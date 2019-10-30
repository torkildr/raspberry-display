#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <vector>

#include "display_impl.hpp"
#include "ht1632.hpp"

namespace ht1632 {

int spifd = -1;

int cs_pins[] = { HT1632_PANEL_PINS };
int panel_count = sizeof(cs_pins) / sizeof(cs_pins[0]);

void select_chip(int pin)
{
    for (int i=0; i < panel_count; ++i) {
        if (pin == HT1632_PANEL_ALL) {
            digitalWrite(cs_pins[i], LOW);
        } else if (pin == HT1632_PANEL_NONE) {
            digitalWrite(cs_pins[i], HIGH);
        } else {
            digitalWrite(cs_pins[i], cs_pins[i] == pin ? LOW : HIGH);
        }
        delayMicroseconds(10);
    }
}

void *reverse_endian(void *p, size_t size) {
    char *head = (char *)p;
    char *tail = head + size -1;

    for(; tail > head; --tail, ++head) {
        char temp = *head;
        *head = *tail;
        *tail = temp;
    }
    return p;
}

void ht1632_write(const void *buffer, size_t size)
{
    ssize_t length = write(spifd, buffer, size);

    if (length == -1) {
        perror("Device write failed");
        exit(EXIT_FAILURE);
    }
}

void send_cmd(int pin, uint8_t cmd)
{
    uint16_t data = HT1632_ID_CMD;
    data <<= HT1632_LENGTH_DATA;
    data |= cmd;
    data <<= (16 - HT1632_LENGTH_DATA - HT1632_LENGTH_ID);
    reverse_endian(&data, sizeof(data));

    piLock(HT1632_WIREPI_LOCK_ID);

    select_chip(pin);
    ht1632_write(&data, 2);
    select_chip(HT1632_PANEL_NONE);

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

}

namespace display {

void init()
{
    /* set cs pins to output */
    for (int i = 0; i < ht1632::panel_count; ++i) {
        pinMode(ht1632::cs_pins[i], OUTPUT);
    }

    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_DIS);
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_EN);
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_COM);
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_LED_ON);
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_BLINK_OFF);
}

DisplayImpl::DisplayImpl(std::function<void()> preUpdate, std::function<void()> postUpdate)
    : Display(preUpdate, postUpdate)
{
    if (wiringPiSetup() == -1) {
        perror("WiringPi Setup Failed");
        exit(EXIT_FAILURE);
    }

    ht1632::spifd = wiringPiSPISetup(0, HT1632_SPI_FREQ);
    if (!ht1632::spifd) {
        perror("SPI Setup Failed");
        exit(EXIT_FAILURE);
    }

    if (X_MAX != (HT1632_PANEL_WIDTH * ht1632::panel_count)) {
        printf("Display area (X_MAX) must be equal to total panel columns.\n");
        printf("X_MAX: %d\n", X_MAX);
        printf("Panel columns: %d, (%d * %d)\n",
                (HT1632_PANEL_WIDTH * ht1632::panel_count),
                HT1632_PANEL_WIDTH,
                ht1632::panel_count);
        exit(EXIT_FAILURE);
    }

    init();
    setBrightness(8);

    printf("Display enabled\n");
}

DisplayImpl::~DisplayImpl()
{
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_LED_OFF);
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_DIS);

    close(ht1632::spifd);
}

void DisplayImpl::setBrightness(int brightness)
{
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_PWM + (brightness & 0xF));
}

std::array<unsigned char, X_MAX+2> createWriteBuffer(std::array<char, X_MAX> displayBuffer, int panel)
{
    const int bufferSize = X_MAX + 2;
    std::array<unsigned char, bufferSize> buffer = { 0 };

    uint8_t offset = panel * HT1632_PANEL_WIDTH;

    /* start buffer with write command and zero address */
    buffer[0] = HT1632_ID_WRITE << (8 - HT1632_LENGTH_ID);

    /* start bitcount after initial command */
    int n = HT1632_LENGTH_ID + HT1632_LENGTH_ADDR;
    int excess_bits = ((bufferSize * 8) - n) % 8;

    for(int i=0; i < HT1632_PANEL_WIDTH*8; ++i, ++n) {
        uint8_t src_pos = i / 8;
        uint8_t src_bit = i % 8;
        uint8_t dst_pos = n / 8;
        uint8_t dst_bit = 7 - (n % 8);

        #ifdef HT1632_FLIP_180
            uint8_t src_val = (displayBuffer[X_MAX - 1 - src_pos - offset] >> (7 - src_bit)) & 1;
        #else
            uint8_t src_val = (displayBuffer[src_pos + offset] >> src_bit) & 1;
        #endif

        buffer[dst_pos] |= src_val << dst_bit;

        /*
         * Hack to work around Raspberry Pi's 8-bit SPI write.
         *
         * Since the Pi will write SPI data in chunks of 8 bits, and the ht1632
         * write command and address is 10 bits wide, we will in most cases end up
         * with excess bits at the end of the buffer. It tourns out that these
         * bits wrap around and are written to the starting address. To work around this, we
         * simple write the first couple of bits both at the start and at the end
         * of the SPI data. No biggie...
         */
        if (i < excess_bits) {
            buffer[bufferSize - 1] |= src_val << (excess_bits - i - 1);
        }
    }

    return buffer;
}

void DisplayImpl::update()
{
    piLock(HT1632_WIREPI_LOCK_ID);

    for (int i = 0; i < ht1632::panel_count; ++i) {
        ht1632::select_chip(ht1632::cs_pins[i]);

        auto buffer = createWriteBuffer(displayBuffer, i);
        ht1632::ht1632_write(&buffer, buffer.size());

        ht1632::select_chip(HT1632_PANEL_NONE);
    }

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

}
