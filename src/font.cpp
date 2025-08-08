#include <string>
#include <cstring>
#include <vector>

#include "font.h"
#include "font.hpp"

static unsigned char *fontCharacter(char c)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
    {
        // Return default character (space ' ') for unknown characters
        // This prevents null pointer dereference and segmentation faults
        substr = strchr(charLookup, ' ');
        if (substr == NULL)
        {
            // Fallback: return first character if even space is not found
            return font_variable[0];
        }
    }

    return font_variable[substr - charLookup];
}

static std::vector<char> renderChar(char c)
{
    unsigned char *glyph = fontCharacter(c);
    short width = glyph[0];
    std::vector<char> rendered;

    short col;
    for (col = 0; col < width; col++)
    {
        rendered.push_back(glyph[col + 1]);
    }

    // empty row after glyph
    rendered.push_back(0);

    return rendered;
}

namespace font
{

std::vector<char> renderString(std::string text)
{
    std::vector<char> rendered;

    for (auto it = text.begin(); it < text.end(); ++it)
    {
        auto glyph = renderChar(*it);
        rendered.insert(rendered.end(), glyph.begin(), glyph.end());
    }

    return rendered;
}

}