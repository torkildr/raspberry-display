#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "font.h"

// 128 columns of 8 bit pixel rows
unsigned char display_memory[128];

void render_memory()
{
    int row;
    for (row = 0; row < 8; row++)
    {
        int col;
        for (col = 0; col < 128; col++)
        {
            if (display_memory[col] & (1 << row))
                addch('x');
            else
                addch(' ');
        }
        addch('\n');
    }
}

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

void hardcoded_pixels()
{
    display_memory[0] = 0xff;
    display_memory[127] = 0x01;

    // $
    display_memory[23] = 0x24;
    display_memory[24] = 0x2a;
    display_memory[25] = 0x7f;
    display_memory[26] = 0x2a;
    display_memory[27] = 0x12;

    // a
    display_memory[100] = 0x20;
    display_memory[101] = 0x54;
    display_memory[102] = 0x54;
    display_memory[103] = 0x78;
}

int main()
{
    setup_curses();
    hardcoded_pixels();

    int done = 0;

    while (!done)
    {
        int c;

        if ((c = getch()) != ERR)
        {
            switch (c)
            {
                case 'q':
                    done = 1;
                    break;
            }
        }

        clear();
        render_memory();
        halfdelay(2);
    }

    endwin();
    return 0;
}

