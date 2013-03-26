#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "display.h"
#include "font.h"

/* Text/time display stuff */

int scroll_direction = SCROLL_DISABLED;
int scroll_offset = 0;
int format_kind = TEXT_DATA;

unsigned char *text_block;
int text_block_width;
int autoscroll = 0;

/* Font stuff  */

int char_index(char c)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        return 0;

    return substr - charLookup;
}

int text_width(char* text)
{
    int width = 0, i = 0;
    int length = strlen(text);

    for(i = 0; i < length; i++){
        int index = char_index(text[i]);
        width += font_variable[index][0] + 1;
    }

    return width;
}

/* Text display stuff */

void display_text(char *text, int includeTime)
{
    if (!includeTime) {
        format_kind = TEXT_DATA;
        text_block = display_memory;
        text_block_width = X_MAX;
    } else {
        format_kind = TIME_TEXT_COMBINED;
        text_block = display_memory + TIME_WIDTH;
        text_block_width = X_MAX - TIME_WIDTH;
    }

    /* Calculate scroll/autoscroll */
    if (!autoscroll) {
        display_scroll(scroll_direction);
    } else {
        if (text_width(text) > text_block_width) {
            scroll_direction = SCROLL_LEFT;
            scroll_offset = text_block_width;
        } else {
            scroll_direction = SCROLL_DISABLED;
            scroll_offset = 0;
        }
    }

    strncpy(text_buffer, text, BUFFER_SIZE - 1);
}

void display_time(char *format)
{
    format_kind = TIME_FORMAT;
    text_block = display_memory;
    text_block_width = X_MAX;

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

/* Render display memory */

int render_char(unsigned char *display_buffer, int display_width, char c, int x)
{
    int index = char_index(c);
    int width = font_variable[index][0];

    int col;
    for (col = 0; col < width; col++) {
        /* Don't write to the display buffer if the location is out of range */
        if((x + col) >= 0 && (x + col) < display_width) {
            /* reads entire column of the glyph, jams it into memory */
            display_buffer[x+col] = font_variable[index][col+1];
        }
    }

    return width;
}

void render_string(unsigned char *display_buffer, int display_width, char *text, int offset)
{
    int length = strlen(text);
    int i;
    for (i=0; i < length; i++) {
        int width = render_char(display_buffer, display_width, text[i], offset);
        offset += width + 1;
    }
}

void make_text(char *text)
{
    int offset = 0;
    memset(display_memory, '\0', sizeof(display_memory));

    /* Prepare time stuff */
    if (format_kind == TIME_FORMAT)
        make_time(text_buffer, time_format);
    if (format_kind == TIME_TEXT_COMBINED) {
        char left_text[10];
        make_time(left_text, "%H:%M");
        render_string(display_memory, X_MAX, left_text, 0);
        display_memory[TIME_WIDTH-2] = 0xff;
    }

    /* Prepare scroll stuff */
    if (scroll_direction != SCROLL_DISABLED) {
        scroll_offset += scroll_direction;
        offset = scroll_offset;

        /* Text is totally outside visible area, reset offset */
        int width = text_width(text);
        if (scroll_direction == SCROLL_LEFT && offset < (0 - width))
            scroll_offset = text_block_width;
        if (scroll_direction == SCROLL_RIGHT && offset > X_MAX)
            scroll_offset = 0 - width;
    }

    render_string(text_block, text_block_width, text_buffer, offset);
}

void display_scroll(enum scrolling direction)
{
    autoscroll = 0;

    if (direction != SCROLL_RESET) {
        scroll_direction = direction;
    }

    if (scroll_direction == SCROLL_LEFT)
        scroll_offset = text_block_width;
    if (scroll_direction == SCROLL_RIGHT)
        scroll_offset = 0 - text_width(text_buffer);
    if (scroll_direction == SCROLL_DISABLED || scroll_direction == SCROLL_RESET)
        scroll_offset = 0;
    if (scroll_direction == SCROLL_AUTO) {
        scroll_offset = 0;
        scroll_direction = SCROLL_DISABLED;
        autoscroll = 1;
    }
}

/* Timer */

timer_t update_timer;

static void timer_handler(int sig, siginfo_t *si, void *uc)
{
    if (display_pre_update != NULL)
        display_pre_update();

    display_update();

    if (display_post_update != NULL)
        display_post_update();

    make_text(text_buffer);
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

