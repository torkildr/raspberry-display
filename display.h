#ifndef DISPLAY_H
#define DISPLAY_H

#define INTERVAL_SEC  0      // second
#define INTERVAL_USEC 100000 // microseconds

#define X_MAX 128

// 128 columns of 8 bit pixel rows
unsigned char display_memory[X_MAX];

// render display memory on device
void display_update();

// clear device
void display_clear();

// put text in memory
void render_text(char *text, int offset);

// timer stuff
void timer_enable();
void timer_disable();

#endif

