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

namespace ht1632
{

int spifd = -1;

int cs_pins[] = {HT1632_PANEL_PINS};
int panel_count = sizeof(cs_pins) / sizeof(cs_pins[0]);

static void select_chip(int pin)
{
    for (int i = 0; i < panel_count; ++i)
    {
        if (pin == HT1632_PANEL_ALL)
        {
            digitalWrite(cs_pins[i], LOW);
        }
        else if (pin == HT1632_PANEL_NONE)
        {
            digitalWrite(cs_pins[i], HIGH);
        }
        else
        {
            digitalWrite(cs_pins[i], cs_pins[i] == pin ? LOW : HIGH);
        }
    }
    // Single delay after all pins are set, rather than per-pin
    // HT1632 requires minimum 1μs setup time, 10μs is very conservative
    delayMicroseconds(2);
}



static void ht1632_write(const void *buffer, size_t size)
{
    // Create a copy since wiringPiSPIDataRW modifies the buffer
    std::vector<unsigned char> data(static_cast<const unsigned char*>(buffer), 
                                   static_cast<const unsigned char*>(buffer) + size);
    
    int result = wiringPiSPIDataRW(0, data.data(), static_cast<int>(size));
    
    if (result == -1)
    {
        perror("SPI write failed");
        exit(EXIT_FAILURE);
    }
}

static void send_cmd(int pin, uint8_t cmd)
{
    // HT1632 Command Protocol: 12 bits total
    // Format: [3-bit ID][8-bit command][1 padding bit] = 12 bits
    uint16_t data = ((static_cast<uint16_t>(HT1632_ID_CMD) << 8) | cmd) << 1;
    
    // Convert to bytes for SPI (12 bits = 1.5 bytes, so use 2 bytes)
    uint8_t spi_data[2];
    spi_data[0] = static_cast<uint8_t>((data >> 4) & 0xFF);    // Upper 8 bits
    spi_data[1] = static_cast<uint8_t>((data << 4) & 0xF0);    // Lower 4 bits, left-aligned

    piLock(HT1632_WIREPI_LOCK_ID);

    select_chip(pin);
    ht1632_write(spi_data, 2);
    select_chip(HT1632_PANEL_NONE);

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

} // namespace ht1632

namespace display
{

static void init()
{
    /* set cs pins to output */
    for (int i = 0; i < ht1632::panel_count; ++i)
    {
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
    if (wiringPiSetup() == -1)
    {
        perror("WiringPi Setup Failed");
        exit(EXIT_FAILURE);
    }

    ht1632::spifd = wiringPiSPISetup(0, HT1632_SPI_FREQ);
    if (!ht1632::spifd)
    {
        perror("SPI Setup Failed");
        exit(EXIT_FAILURE);
    }

    if (X_MAX != (HT1632_PANEL_WIDTH * ht1632::panel_count))
    {
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

static std::array<unsigned char, 34> createWriteBuffer(std::array<char, X_MAX> displayBuffer, int panel)
{
    std::array<unsigned char, 34> buffer = {0};
    
    auto offset = static_cast<uint8_t>(panel * HT1632_PANEL_WIDTH);
    
    // Header: Write command (3 bits) + Address 0 (7 bits) = 10 bits
    uint16_t header = (HT1632_ID_WRITE << 7) | 0x00;
    buffer[0] = static_cast<uint8_t>((header >> 2) & 0xFF);        // Upper 8 bits
    buffer[1] = static_cast<uint8_t>((header << 6) & 0xC0);        // Lower 2 bits
    
    // Pack display data using correct column-major layout
    int bit_pos = 10;  // Start after header
    
    // Store first few pixels for duplication at end (SPI alignment fix)
    std::array<uint8_t, 8> first_pixels = {0};
    int first_pixel_count = 0;
    
    // HT1632 expects: Column 0 (8 bits), Column 1 (8 bits), ..., Column 31 (8 bits)
    for (int col = 0; col < 32; ++col) {        // 32 columns
        for (int row = 0; row < 8; ++row) {     // 8 rows per column
            uint8_t pixel = 0;
            
            // Get pixel from our display buffer
            size_t src_index = static_cast<size_t>(offset + col);
            
            if (src_index < X_MAX) {
#ifdef HT1632_FLIP_180
                // Flip entire 128x8 display for 180-degree rotation
                // Map (panel, col) to (3-panel, 31-col) globally
                int flipped_panel = 3 - panel;
                int flipped_col_in_panel = HT1632_PANEL_WIDTH - 1 - col;
                size_t global_flipped_col = static_cast<size_t>(flipped_panel * HT1632_PANEL_WIDTH + flipped_col_in_panel);
                int flipped_row = 7 - row;
                
                if (global_flipped_col < X_MAX && displayBuffer[global_flipped_col] & (1 << flipped_row)) {
                    pixel = 1;
                }
#else
                // Our display buffer: each byte is a column, bit 0 = top pixel
                if (displayBuffer[src_index] & (1 << row)) {
                    pixel = 1;
                }
#endif
            }
            
            // Store first few pixels for duplication (SPI alignment fix)
            if (first_pixel_count < 8) {
                first_pixels[static_cast<size_t>(first_pixel_count)] = pixel;
                first_pixel_count++;
            }
            
            // Pack pixel into SPI buffer
            if (pixel) {
                int byte_pos = bit_pos / 8;
                int bit_offset = 7 - (bit_pos % 8);  // MSB first
                if (byte_pos < 34) {
                    buffer[static_cast<size_t>(byte_pos)] |= (1 << bit_offset);
                }
            }
            bit_pos++;
        }
    }
    
    // Duplicate first few pixels at the end to handle SPI bit wrap-around
    // The excess bits from the 266-bit total (33.25 bytes) wrap around to address 0
    for (int i = 0; i < 6 && i < first_pixel_count; ++i) {
        if (first_pixels[static_cast<size_t>(i)]) {
            int byte_pos = bit_pos / 8;
            int bit_offset = 7 - (bit_pos % 8);  // MSB first
            if (byte_pos < 34) {
                buffer[static_cast<size_t>(byte_pos)] |= (1 << bit_offset);
            }
        }
        bit_pos++;
    }
    
    return buffer;
}

void DisplayImpl::update()
{
    
    piLock(HT1632_WIREPI_LOCK_ID);

    for (int i = 0; i < ht1632::panel_count; ++i)
    {
        ht1632::select_chip(ht1632::cs_pins[i]);

        auto buffer = createWriteBuffer(displayBuffer, i);
        // Send the entire buffer - it contains multiple write commands
        ht1632::ht1632_write(buffer.data(), buffer.size());

        // Deselect immediately after write to minimize hold time
        ht1632::select_chip(HT1632_PANEL_NONE);
    }

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

} // namespace display
