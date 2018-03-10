#include <string.h>
#include <curses.h>
#include <sys/time.h>
#include "display.h"

/*
 * Mocked stuff from display.h
 */

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

void show_elapsed_time(long elapsed)
{
    char buffer[X_MAX+5];
    memset(buffer, ' ', X_MAX+4);
    buffer[X_MAX+3] = '\n';
    buffer[X_MAX+4] = '\0';
    snprintf(buffer+X_MAX+4-9, 9, "%.2f sec", (float) elapsed / 1000000);
    addstr(buffer);
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

    refresh();
}

