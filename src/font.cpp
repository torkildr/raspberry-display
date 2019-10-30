#include <string>
#include <cstring>
#include <vector>

#include "font.h"
#include "font.hpp"

unsigned char* fontCharacter(char c)
{
    char *substr = strchr(charLookup, c);
    if (substr == NULL)
        substr = 0;

    return font_variable[substr - charLookup];
}

std::vector<char> renderChar(char c)
{
    unsigned char *glyph = fontCharacter(c);
    short width = glyph[0];
    std::vector<char> rendered;

    short col;
    for (col = 0; col < width; col++) {
        rendered.push_back(glyph[col + 1]);
    }

    // empty row after glyph
    rendered.push_back(0);

    return rendered;
}

namespace font {

std::vector<char> renderString(std::string text)
{
    std::vector<char> rendered;

    for (auto it = text.begin(); it < text.end(); ++it) {
        auto glyph = renderChar(*it);
        rendered.insert(rendered.end(), glyph.begin(), glyph.end());
    }

    return rendered;
}

}