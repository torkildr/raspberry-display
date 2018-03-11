#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>

#include "display.h"
#include "ht1632.h"

#define HT1632_WIREPI_LOCK_ID 0

static int cs_pins[] = { HT1632_PANEL_PINS };
static int panel_count = sizeof(cs_pins) / sizeof(cs_pins[0]);

static int spifd = -1;

void select_chip(int pin)
{
    for (int i=0; i < panel_count; ++i) {
        if (pin & HT1632_PANEL_ALL) {
            digitalWrite(cs_pins[i], 1);
        } else if (pin & HT1632_PANEL_NONE) {
            digitalWrite(cs_pins[i], 0);
        } else {
            digitalWrite(cs_pins[i], cs_pins[i] == pin ? 1 : 0);
        }
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

void send_cmd(int pin, uint8_t cmd)
{
    uint16_t data = HT1632_ID_CMD;
    data <<= HT1632_LENGTH_DATA;
    data |= cmd;
    data <<= (16 - HT1632_LENGTH_DATA - HT1632_LENGTH_ID);
    reverse_endian(&data, sizeof(data));

    piLock(HT1632_WIREPI_LOCK_ID);

    select_chip(pin);
    write(spifd, &data, 2);
    select_chip(HT1632_PANEL_NONE);

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

void display_enable()
{
    if (wiringPiSetup() == -1) {
        printf("WiringPi Setup Failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    spifd = wiringPiSPISetup(0, HT1632_SPI_FREQ);
    if (!spifd) {
        printf("SPI Setup Failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (X_MAX != (HT1632_PANEL_WIDTH * panel_count)) {
        printf("Display area (X_MAX) must be equal to total panel columns.\n");
        printf("X_MAX: %d\n", X_MAX);
        printf("Panel columns: %d, (%d * %d)\n",
                (HT1632_PANEL_WIDTH * panel_count),
                HT1632_PANEL_WIDTH,
                panel_count);
        exit(EXIT_FAILURE);
    }

    /* set cs pins to output */
    for (int i = 0; i < panel_count; ++i) {
        pinMode(cs_pins[i], OUTPUT);
    }

    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_DIS);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_COM);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_MASTER);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_RC);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_EN);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_LED_ON);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_BLINK_OFF);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_PWM);

    printf("Display initialized\n");
}

void display_disable()
{
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_LED_OFF);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_DIS);

    close(spifd);
}

void display_clear()
{
    /*
     * There is no need to clear the display, seeing as we render all pixels on
     * every refresh.
     * To clear display, you simply empty the buffer, `display_memory`.
     */
}

void display_update()
{
    piLock(HT1632_WIREPI_LOCK_ID);

    for (int i = 0; i < panel_count; ++i) {
        unsigned char *pos = display_memory + (i * HT1632_PANEL_WIDTH);

        select_chip(i);
        write(spifd, pos, HT1632_PANEL_WIDTH);
        select_chip(HT1632_PANEL_NONE);
    }

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

