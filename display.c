#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "font.h"

int render_char(char c, int x)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        return 0;

    int index = substr - charLookup;
    int width = font_variable[index][0];

    int col;
    for (col = 0; col < width; col++) {
        // dont write to the display buffer if the location is out of range
        if((x + col) >= 0 && (x + col) < X_MAX){
            // reads entire column of the glyph, jams it into memory
            display_memory[x+col] = font_variable[index][col+1];
        }
    }

    return width;
}

void render_string(char *text, int x)
{
    int length = strlen(text);
    int i;
    for (i=0; i < length; i++)
    {
        int width = render_char(text[i], x);
        x += width + 1;
    }
}

void render_text(char *text, int offset)
{
   memset(display_memory, 0, sizeof(display_memory));

   render_string(text, offset);

   clear_display();
   update_display();
}

