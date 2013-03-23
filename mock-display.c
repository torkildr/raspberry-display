#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <time.h>
#include "font.h"

#define X_MAX 128

// 128 columns of 8 bit pixel rows
unsigned char display_memory[X_MAX];

void render_memory()
{
    int row;
    for (row = 0; row < 8; row++)
    {
        int col;
        for (col = 0; col < X_MAX; col++)
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

int render_char(char c, int x)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        return 0;

    int index = substr - charLookup;
    int width = font_variable[index][0];

    int col;
    for (col = 0; col <= width; col++) {
        // dont write to the display buffer if the location is out of range
        if((x + col) >= 0 && (x + col) < X_MAX){
            // reads entire row of glyph, jams it into memory
            if (col != width)
                display_memory[x+col] = font_variable[index][col+1];
            else
                display_memory[x+col] = 0;
        }
    }

    return width;
}

void render_string(char * text, int x)
{
    int length = strlen(text);
    int i;
    for (i=0; i < length; i++)
    {
        int width = render_char(text[i], x);
        x += width + 1;
    }
}

int main()
{
    setup_curses();

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

        memset(display_memory, 0, sizeof(display_memory));

        time_t current_time = time(NULL);
        struct tm *local_time = localtime(&current_time);
        char *time_string = asctime(local_time);

        render_string(time_string, 0);

        clear();
        render_memory();
        halfdelay(1);
    }

    endwin();
    return 0;
}

