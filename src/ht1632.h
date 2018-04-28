#ifndef HT1632_H
#define HT1632_H

#include "display.h"

/* Display settings */
#define HT1632_PANEL_WIDTH      X_MAX / 4       /* column/pixel width of each panel */
#define HT1632_PANEL_PINS       8, 9, 15, 16    /* wiringPi pins */
#define HT1632_SPI_FREQ         200000          /* Hz */

/* Define HT1632_FLIP_180 to mount the display upside down */
#define HT1632_FLIP_180

#define HT1632_PANEL_NONE       -1
#define HT1632_PANEL_ALL        0xff

/* HT1632 interface settings */
#define HT1632_ID_CMD           0b100
#define HT1632_ID_READ          0b110
#define HT1632_ID_WRITE         0b101

#define HT1632_CMD_SYS_DIS      0b00000000
#define HT1632_CMD_SYS_EN       0b00000001
#define HT1632_CMD_LED_OFF      0b00000010
#define HT1632_CMD_LED_ON       0b00000011
#define HT1632_CMD_BLINK_OFF    0b00001000
#define HT1632_CMD_BLINK_ON     0b00001001
#define HT1632_CMD_SLAVE        0b00010000
#define HT1632_CMD_MASTER       0b00010100
#define HT1632_CMD_RC           0b00011000
#define HT1632_CMD_EXT_CLK      0b00011100
#define HT1632_CMD_COM          0b00100000
#define HT1632_CMD_PWM          0b10100000

/* length in bits */
#define HT1632_LENGTH_ID        3
#define HT1632_LENGTH_CMD       8
#define HT1632_LENGTH_DATA      8
#define HT1632_LENGTH_ADDR      7

/* write buffer, needs extra space for id + address bits */
unsigned char ht1632_write_buffer[HT1632_PANEL_WIDTH + 2];

void update_write_buffer();

void *reverse_endian(void *p, size_t size);

#endif

