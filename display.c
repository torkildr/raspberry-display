#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "display.h"
#include "font.h"

int render_char(char c, int x)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        return 0;

    int index = substr - charLookup;
    int width = font_variable[index][0];

    int col;
    for (col = 0; col < width; col++) {
        // dont write to the display buffer if the location is out of range
        if((x + col) >= 0 && (x + col) < X_MAX) {
            // reads entire column of the glyph, jams it into memory
            display_memory[x+col] = font_variable[index][col+1];
        }
    }

    return width;
}

void render_text(char *text, int offset)
{
    memset(display_memory, 0, sizeof(display_memory));

    int length = strlen(text);
    int i;
    for (i=0; i < length; i++) {
        int width = render_char(text[i], offset);
        offset += width + 1;
    }
}

void display_redraw()
{
    display_clear();
    display_update();
}

void alarm_wakeup(int i)
{
    signal(SIGALRM, alarm_wakeup);
    display_redraw();
}

void timer_disable()
{
    struct itimerval timer_val = {
        .it_interval.tv_sec = 0,
        .it_interval.tv_usec = 0,
        .it_value.tv_sec = 0,
        .it_value.tv_usec = 0
    };

    setitimer(ITIMER_REAL, &timer_val, NULL);
}

void timer_enable()
{
    struct itimerval timer_val = {
        .it_interval.tv_sec = INTERVAL_SEC,
        .it_interval.tv_usec = INTERVAL_USEC,
        .it_value.tv_sec = INTERVAL_SEC,
        .it_value.tv_usec = INTERVAL_USEC
    };

    signal(SIGALRM, alarm_wakeup);
    setitimer(ITIMER_REAL, &timer_val, 0);
}

