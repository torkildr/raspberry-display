#include <curses.h>
#include "display.h"

//
// mocked stuff from display.h
//

void display_clear()
{
    clear();
}

void display_update()
{
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

    addstr("\nnavigate with left/right, page up/down\nt: time\na: supported characters\n0: reset offset");
    addstr("\nr/l: scroll left/right\nd: disable scroll");
    addstr("\nq: exit");
}

