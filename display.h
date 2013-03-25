#ifndef DISPLAY_H
#define DISPLAY_H

#define INTERVAL_SEC  0   // second
#define INTERVAL_MS   100 // microseconds

#define X_MAX 128
#define BUFFER_SIZE 1024

enum scrolling {
    SCROLL_DISABLED = 0,
    SCROLL_LEFT = -1,
    SCROLL_RIGHT = 1
};

enum format {
    TEXT_DATA,
    TIME_FORMAT
};

char text_buffer[BUFFER_SIZE];

// 128 columns of 8 bit pixel rows
unsigned char display_memory[X_MAX];

void (*display_update_callback)();

void display_enable();
void display_disable();

void display_update();
void display_clear();
void display_scroll(enum scrolling direction);

void display_text(char *text);
void display_time(char *format);

// timer stuff
void timer_enable();
void timer_disable();

#endif

