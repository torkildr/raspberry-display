#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "display.h"
#include "font.h"

int scroll_direction = SCROLL_DISABLED;
int scroll_offset = 0;

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
    memset(display_memory, '\0', sizeof(display_memory));

    if (scroll_direction != SCROLL_DISABLED) {
        scroll_offset += scroll_direction;
        offset = scroll_offset;
    }

    int length = strlen(text);
    int i;
    for (i=0; i < length; i++) {
        int width = render_char(text[i], offset);
        offset += width + 1;
    }
}

void display_scroll(enum scrolling direction)
{
    scroll_direction = direction;
    scroll_offset = 0;
}

void timer_disable()
{
}

static void timerHandler( int sig, siginfo_t *si, void *uc )
{
    display_update();
}

timer_t update_timer;

void timer_enable()
{
    struct sigevent         te;
    struct itimerspec       its;
    struct sigaction        sa;
    int                     sigNo = SIGALRM;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(sigNo, &sa, NULL) == -1)
    {
        exit(-1);
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sigNo;
    timer_create(CLOCK_REALTIME, &te, &update_timer);

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 50 * 1000000;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 50 * 1000000;
    timer_settime(update_timer, 0, &its, NULL);
}

