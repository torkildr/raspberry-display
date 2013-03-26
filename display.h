#ifndef DISPLAY_H
#define DISPLAY_H

#define INTERVAL_SEC  0   /* second */
#define INTERVAL_MS   100 /* microseconds */

#define X_MAX 128
#define TIME_WIDTH 29

#define BUFFER_SIZE 1024

enum scrolling {
    SCROLL_DISABLED = 0,
    SCROLL_LEFT = -1,
    SCROLL_RIGHT = 1,
    SCROLL_RESET = -99
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
void display_enable();
void display_disable();
void display_update();
void display_clear();

/* Set/manipulate text on display */
void display_scroll(enum scrolling direction);
void display_text(char *text, int includeTime);
void display_time(char *format);

/* Timer stuff */
void timer_enable();
void timer_disable();

#endif

