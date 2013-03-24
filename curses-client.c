#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include "display.h"

char *abc_string()
{
    return " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\xe6\xf8\xe5\xc6\xd8\xc5";
}

void print_help_text()
{
    addstr("\nt: time\na: supported characters\n0: reset offset");
    addstr("\nr/l: scroll left/right\nd: disable scroll");
    addstr("\nq: exit");
}

int main()
{
    int done = 0;

    display_update_callback = &print_help_text;

    display_enable();
    timer_enable();
    display_time("");

    while (!done) {
        int c;

        if ((c = getch()) != ERR) {
            switch (c) {
                case 'q':
                    done = 1;
                    break;
                case 't':
                    display_time("");
                    break;
                case 'a':
                    display_text(abc_string());
                    break;
                case 'l':
                    display_scroll(SCROLL_LEFT);
                    break;
                case 'r':
                    display_scroll(SCROLL_RIGHT);
                    break;
                case 'd':
                case '0':
                case KEY_HOME:
                    display_scroll(SCROLL_DISABLED);
                    break;
            }
        }

        halfdelay(2);
    }

    timer_disable();
    display_disable();

    return 0;
}

