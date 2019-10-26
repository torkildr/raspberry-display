#include <string.h>
#include <curses.h>
#include <sys/time.h>
#include "display.h"

/*
 * Mocked stuff from display.h
 */

static uint8_t brightness = 0;

void display_init()
{
}

void display_enable()
{
    initscr();
    timeout(0);
    cbreak();
    noecho();

    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
}

void display_disable()
{
    endwin();
}

void display_clear()
{
    clear();
}

struct timeval last_updated;

long get_elapsed_time()
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    long elapsed = (current_time.tv_sec - last_updated.tv_sec) * 1000000 + \
                   current_time.tv_usec - last_updated.tv_usec;

    gettimeofday(&last_updated, NULL);

    return elapsed;
}

void left_aligned_text(char *text)
{
    int width = X_MAX + 4;
    char buffer[width + 1];
    memset(buffer, ' ', width);

    int length = strlen(text) + 1;
    snprintf(buffer + width - length, length + 1, "%s\n", text);
    addstr(buffer);
}

void show_elapsed_time(long elapsed)
{
    char buffer[50];
    snprintf(buffer, 49, "Update frequency: %.2f sec", (float) elapsed / 1000000);
    left_aligned_text(buffer);
}

void show_brightness()
{
    char buffer[50];
    snprintf(buffer, 49, "Brightness: %d", brightness & 0xF);
    left_aligned_text(buffer);
}

void display_brightness(uint8_t new_brightness)
{
    brightness = new_brightness;
}

void display_update()
{
    long elapsed = get_elapsed_time();
    clear();

    int row, i;
    for (row = -1; row <= 8; row++)
    {
        if (row == -1)
        {
            addch(ACS_ULCORNER);
            for(i = 0; i < X_MAX + 2; i++) addch(ACS_HLINE);
            addch(ACS_URCORNER);
            addch('\n');
            continue;
        }

        if (row == 8)
        {
            addch(ACS_LLCORNER);
            for(i = 0; i < X_MAX + 2; i++) addch(ACS_HLINE);
            addch(ACS_LRCORNER);
            addch('\n');
            continue;
            continue;
        }

        addch(ACS_VLINE);
        addch(' ');

        int col;
        for (col = 0; col < X_MAX; col++)
        {
            if (display_memory[col] & (1 << row))
                addch(ACS_CKBOARD);
            else
                addch(' ');
        }

        addch(' ');
        addch(ACS_VLINE);
        addch('\n');
    }

    show_elapsed_time(elapsed);
    show_brightness();

    refresh();
}

