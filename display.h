#ifndef DISPLAY_H
#define DISPLAY_H

#define INTERVAL_SEC  0      // second
#define INTERVAL_USEC 100000 // microseconds

#define X_MAX 128

enum scrolling {
    SCROLL_DISABLED = 0,
    SCROLL_LEFT = -1,
    SCROLL_RIGHT = 1
};

// 128 columns of 8 bit pixel rows
unsigned char display_memory[X_MAX];

void display_update();
void display_clear();
void display_scroll(enum scrolling direction);

// put text in memory
void render_text(char *text, int offset);

// timer stuff
void timer_enable();
void timer_disable();

#endif

