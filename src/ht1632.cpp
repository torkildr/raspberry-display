#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <vector>
#include <chrono>

#include "display_impl.hpp"
#include "ht1632.hpp"

namespace ht1632
{

int spifd = -1;

int cs_pins[] = {HT1632_PANEL_PINS};
int panel_count = sizeof(cs_pins) / sizeof(cs_pins[0]);

// Stability tracking for display health monitoring
static std::chrono::steady_clock::time_point last_reinit_time = std::chrono::steady_clock::now();

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
    // Increased delay for better signal integrity, especially for 4th panel
    // HT1632 requires minimum 1μs setup time, using 5μs for stability
    delayMicroseconds(5);
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
    // Add small delay before deselecting for better signal integrity
    delayMicroseconds(2);
    select_chip(HT1632_PANEL_NONE);

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

static void initialize_displays()
{
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_SYS_EN);
    delayMicroseconds(50);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_COM);
    delayMicroseconds(50);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_LED_ON);
    delayMicroseconds(50);
    send_cmd(HT1632_PANEL_ALL, HT1632_CMD_BLINK_OFF);
    delayMicroseconds(50);

    last_reinit_time = std::chrono::steady_clock::now();
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
    ht1632::initialize_displays();
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
    currentBrightness = brightness & 0xF;  // Store for restoration after reinitialization
    ht1632::send_cmd(HT1632_PANEL_ALL, HT1632_CMD_PWM + static_cast<uint8_t>(currentBrightness));
}

// Helper function to reverse bits in a byte (for 180-degree flip)
static constexpr uint8_t reverse_bits(uint8_t byte) {
    return ((byte & 0x01) << 7) | ((byte & 0x02) << 5) | ((byte & 0x04) << 3) | ((byte & 0x08) << 1) |
           ((byte & 0x10) >> 1) | ((byte & 0x20) >> 3) | ((byte & 0x40) >> 5) | ((byte & 0x80) >> 7);
}

static std::array<unsigned char, 34> createWriteBuffer(std::array<char, X_MAX> displayBuffer, int panel)
{
    std::array<unsigned char, 34> buffer = {0};
    
    // Header: Write command (3 bits) + Address 0 (7 bits) = 10 bits
    const uint16_t header = (HT1632_ID_WRITE << 7) | 0x00;
    buffer[0] = static_cast<uint8_t>((header >> 2) & 0xFF);        // Upper 8 bits
    buffer[1] = static_cast<uint8_t>((header << 6) & 0xC0);        // Lower 2 bits
    
    const auto offset = static_cast<size_t>(panel * HT1632_PANEL_WIDTH);
    
#ifdef HT1632_FLIP_180
    // Pre-calculate flipped panel position (avoid repeated calculation)
    const size_t flipped_panel_base = static_cast<size_t>(3 - panel) * HT1632_PANEL_WIDTH;
#endif
    
    // Single pass: process all 32 columns + 6 duplicate pixels in one loop
    // Total: 32*8 + 6 = 262 bits starting at bit position 10
    // The last 6 bits are duplicates of the first 6 pixels (column 0, rows 0-5)
    size_t bit_pos = 10;
    
    // Process regular pixels (32 columns, 8 pixels per column)
    for (int col = 0; col < 32; ++col) {
        uint8_t column_pixels = 0;
        const size_t src_index = offset + static_cast<size_t>(col);
        
        if (src_index < X_MAX) {
#ifdef HT1632_FLIP_180
            // Calculate flipped position and get the entire column
            const size_t flipped_col = flipped_panel_base + static_cast<size_t>(HT1632_PANEL_WIDTH - 1 - col);
            if (flipped_col < X_MAX) {
                // Reverse the bit order for 180° flip (bit 0 becomes bit 7, etc.)
                column_pixels = reverse_bits(static_cast<uint8_t>(displayBuffer[flipped_col]));
            }
#else
            column_pixels = static_cast<uint8_t>(displayBuffer[src_index]);
#endif
        }
        
        // Pack each pixel from the column (optimized: only process set bits)
        for (int row = 0; row < 8; ++row) {
            if (column_pixels & (1 << row)) {
                const size_t byte_pos = bit_pos / 8;
                const size_t bit_offset = 7 - (bit_pos % 8);  // MSB first
                if (byte_pos < 34) {
                    buffer[byte_pos] |= static_cast<uint8_t>(1 << bit_offset);
                }
            }
            bit_pos++;
        }
    }
    
    // Duplicate first 6 pixels for SPI alignment (prevents wrap-around corruption)
    uint8_t first_column_pixels = 0;
    const size_t src_index = offset;  // Always use first column (col=0)
    
    if (src_index < X_MAX) {
#ifdef HT1632_FLIP_180
        // Calculate flipped position for first column
        const size_t flipped_col = flipped_panel_base + static_cast<size_t>(HT1632_PANEL_WIDTH - 1);
        if (flipped_col < X_MAX) {
            // Reverse the bit order for 180° flip
            first_column_pixels = reverse_bits(static_cast<uint8_t>(displayBuffer[flipped_col]));
        }
#else
        first_column_pixels = static_cast<uint8_t>(displayBuffer[src_index]);
#endif
    }
    
    // Pack first 6 pixels of the column into buffer
    for (int row = 0; row < 6; ++row) {
        if (first_column_pixels & (1 << row)) {
            const size_t byte_pos = bit_pos / 8;
            const size_t bit_offset = 7 - (bit_pos % 8);  // MSB first
            if (byte_pos < 34) {
                buffer[byte_pos] |= static_cast<uint8_t>(1 << bit_offset);
            }
        }
        bit_pos++;
    }
    
    return buffer;
}

void DisplayImpl::update()
{
    // Time-based periodic reinitialization to prevent state corruption
#ifdef HT1632_ENABLE_HEALTH_MONITORING
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - ht1632::last_reinit_time);
    
    if (elapsed.count() >= HT1632_REINIT_INTERVAL_MINUTES)
    {
        ht1632::initialize_displays();
        // Restore current brightness after reinitialization
        setBrightness(currentBrightness);
    }
#endif
    
    piLock(HT1632_WIREPI_LOCK_ID);

    for (int i = 0; i < ht1632::panel_count; ++i)
    {
        ht1632::select_chip(ht1632::cs_pins[i]);
        delayMicroseconds(2);

        auto buffer = createWriteBuffer(displayBuffer, i);
        ht1632::ht1632_write(buffer.data(), buffer.size());
        delayMicroseconds(2);
    }

    piUnlock(HT1632_WIREPI_LOCK_ID);
}

} // namespace display
