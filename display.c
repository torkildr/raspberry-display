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

timer_t update_timer;

int char_index(char c)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        return 0;

    return substr - charLookup;
}

int text_width(char* text) {
    int width = 0, i = 0;
    int length = strlen(text);

    for(i = 0; i < length; i++){
        int index = char_index(text[i]);
        width += font_variable[index][0] + 1;
    }

    return width;
}

int render_char(char c, int x)
{
    int index = char_index(c);
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
    strncpy(text_buffer, text, BUFFER_SIZE - 1);
}

void display_time(char *format)
{
    format_kind = TIME_FORMAT;

    if (!strcmp("", format))
        strcpy(time_format, "%A, %b %-d %H:%M");
    else
        strncpy(time_format, format, BUFFER_SIZE - 1);
}

void make_time(char *buffer, char *format)
{
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);

    strftime(buffer, BUFFER_SIZE, format, local_time);
}

void render_text(char *text)
{
    int offset = 0;
    memset(display_memory, '\0', sizeof(display_memory));

    if (scroll_direction != SCROLL_DISABLED) {
        scroll_offset += scroll_direction;
        offset = scroll_offset;

        // text is totally outside visible area, reset offset
        int width = text_width(text);
        if (scroll_direction == SCROLL_LEFT && offset < (0- width))
            display_scroll(SCROLL_RESET);
        if (scroll_direction == SCROLL_RIGHT && offset > X_MAX)
            display_scroll(SCROLL_RESET);
    }

    if (format_kind == TIME_FORMAT)
        make_time(text_buffer, time_format);

    int length = strlen(text);
    int i;
    for (i=0; i < length; i++) {
        int width = render_char(text[i], offset);
        offset += width + 1;
    }
}

void display_scroll(enum scrolling direction)
{
    if (direction != SCROLL_RESET) {
        scroll_direction = direction;
    }

    if (scroll_direction == SCROLL_LEFT)
        scroll_offset = X_MAX;
    if (scroll_direction == SCROLL_RIGHT)
        scroll_offset = 0 - text_width(text_buffer);
    if (scroll_direction == SCROLL_DISABLED)
        scroll_offset = 0;
}

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
    if (display_pre_update != NULL)
        display_pre_update();

    display_update();

    if (display_post_update != NULL)
        display_post_update();

    render_text(text_buffer);
}

void timer_disable()
{
    timer_delete(update_timer);
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

