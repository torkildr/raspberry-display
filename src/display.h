#include <stdint.h>

#ifndef DISPLAY_H
#define DISPLAY_H

#define INTERVAL_SEC  0   /* second */
#define INTERVAL_MS   40  /* microseconds */

#define X_MAX 128
#define TIME_WIDTH 29

#define BUFFER_SIZE 1024

/* force reinit of display */
#define DISPLAY_REINIT_EVERY_N_SECONDS 20

enum scrolling {
    SCROLL_DISABLED = 0,
    SCROLL_LEFT = -1,
    SCROLL_RIGHT = 1,
    SCROLL_RESET = -99,
    SCROLL_AUTO = -98,
};

enum format {
    TEXT_DATA,
    TIME_FORMAT,
    TIME_TEXT_COMBINED
};

char text_buffer[BUFFER_SIZE];
char time_format[BUFFER_SIZE];

/* X_MAX columns of 8 bit pixel rows */
unsigned char display_memory[X_MAX];

/* Callback functions */
void (*display_pre_update)();
void (*display_post_update)();

/* These needs device specific implentation */
void display_init();
void display_enable();
void display_disable();
void display_update();
void display_clear();
void display_brightness(uint8_t brightness);

/* Set/manipulate text on display */
void display_scroll(enum scrolling direction);
void display_text(char *text, int includeTime);
void display_time(char *format);

/* Timer stuff */
void timer_enable();
void timer_disable();

#endif

