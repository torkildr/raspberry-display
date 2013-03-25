#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "display.h"
#include "font.h"

int scroll_direction = SCROLL_DISABLED;
int scroll_offset = 0;

int format_kind = TEXT_DATA;

char time_format[101];

timer_t update_timer;

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

void display_text(char *text)
{
    format_kind = TEXT_DATA;
    strcpy(text_buffer, text);
}

void display_time(char *format)
{
    format_kind = TIME_FORMAT;
    time_format[100] = '\0';
    strncpy(time_format, format, 100);
}

char *get_time()
{
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);
    return asctime(local_time);
}

void render_text(char *text)
{
    int offset = 0;
    memset(display_memory, '\0', sizeof(display_memory));

    if (scroll_direction != SCROLL_DISABLED) {
        scroll_offset += scroll_direction;
        offset = scroll_offset;
    }

    if (format_kind == TIME_FORMAT)
        strcpy(text, get_time());

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

static void timer_handler( int sig, siginfo_t *si, void *uc )
{
    display_update();
    render_text(text_buffer);
}

void timer_disable()
{
    /* TODO */
}

void timer_enable()
{
    struct sigevent te;
    struct itimerspec its;
    struct sigaction sa;
    int signal_num = SIGALRM;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(signal_num, &sa, NULL);

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = signal_num;
    timer_create(CLOCK_REALTIME, &te, &update_timer);

    its.it_interval.tv_sec = INTERVAL_SEC;
    its.it_interval.tv_nsec = INTERVAL_MS * 1000000;
    its.it_value.tv_sec = INTERVAL_SEC;
    its.it_value.tv_nsec = INTERVAL_MS * 1000000;
    timer_settime(update_timer, 0, &its, NULL);

    text_buffer[0] = '\0';
}

