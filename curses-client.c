#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <time.h>
#include "display.h"

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

void print_help_text()
{
    addstr("\nnavigate with left/right, page up/down\nt: time\na: supported characters\n0: reset offset");
    addstr("\nr/l: scroll left/right\nd: disable scroll");
    addstr("\nq: exit");
}

int main()
{
    int done = 0;
    int offset = 0;
    char * (*get_text)();

    get_text = &time_string;

    display_update_callback = &print_help_text;

    display_enable();
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
                case 'l':
                    display_scroll(SCROLL_LEFT);
                    break;
                case 'r':
                    display_scroll(SCROLL_RIGHT);
                    break;
                case 'd':
                    display_scroll(SCROLL_DISABLED);
                    break;
                case '0':
                case KEY_HOME:
                    display_scroll(SCROLL_DISABLED);
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
    display_disable();

    return 0;
}

