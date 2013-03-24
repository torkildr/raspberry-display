#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <time.h>
#include "display.h"

void setup_curses()
{
    initscr();
    timeout(0);
    cbreak();
    noecho();

    nonl();
    intrflush(stdscr, FALSE);
    keypad(stdscr, TRUE);
}

char *time_string()
{
    time_t current_time = time(NULL);
    struct tm *local_time = localtime(&current_time);
    return asctime(local_time);
}

char *abc_string()
{
    return " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\xe6\xf8\xe5\xc6\xd8\xc5";
}

int main()
{
    setup_curses();

    int done = 0;
    int offset = 0;
    char * (*get_text)();

    get_text = &time_string;
    timer_enable();

    while (!done) {
        int c;

        if ((c = getch()) != ERR) {
            switch (c) {
                case 'q':
                    done = 1;
                    break;
                case 't':
                    get_text = &time_string;
                    break;
                case 'a':
                    get_text = &abc_string;
                    break;
                case '0':
                case KEY_HOME:
                    offset = 0;
                    break;
                case KEY_LEFT:
                    offset -= 1;
                    break;
                case KEY_RIGHT:
                    offset += 1;
                    break;
                case KEY_NPAGE:
                    offset -= X_MAX;
                    break;
                case KEY_PPAGE:
                    offset += X_MAX;
                    break;
            }
        }

        render_text(get_text(), offset);
        halfdelay(2);
    }

    timer_disable();
    endwin();
    return 0;
}

//
// mocked stuff from display.h
//
//
void display_clear()
{
    clear();
}

void display_update()
{
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

    addstr("\nnavigate with left/right, page up/down\nt: time\na: supported characters\n0: reset offset\nq: exit");
}

