#include <string>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <iostream>

#include "font_generated.hpp"
#include "font.hpp"


static const font::GlyphData* fontCharacter(char c)
{
    return font::getGlyph(c);
}

static std::vector<uint8_t> renderChar(char c)
{
    const auto* glyph = fontCharacter(c);
    if (!glyph) {
        return {0}; // Return single empty column if glyph not found
    }
    
    std::vector<uint8_t> rendered;
    
    // Copy the actual glyph columns
    for (uint8_t column : *glyph) {
        rendered.push_back(column);
    }

    // empty row after glyph
    rendered.push_back(0);

    return rendered;
}

namespace font
{

std::vector<uint8_t> renderString(std::string text)
{
    std::vector<uint8_t> rendered;

    for (auto it = text.begin(); it < text.end(); ++it)
    {
        auto glyph = renderChar(*it);
        rendered.insert(rendered.end(), glyph.begin(), glyph.end());
    }

    return rendered;
}

// Static member definitions
std::unordered_map<char, std::vector<uint8_t>> FontCache::glyphCache;
std::unordered_map<std::string, std::vector<uint8_t>> FontCache::stringCache;
bool FontCache::initialized = false;

void FontCache::initializeCache()
{
    if (initialized) return;
    
    // Pre-cache common characters to avoid runtime lookups
    const char* commonChars = " 0123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,!?-";
    
    for (const char* c = commonChars; *c; ++c)
    {
        renderCharCached(*c);  // This will populate the cache
    }
    
    initialized = true;
}

const std::vector<uint8_t>& FontCache::renderCharCached(char c)
{
    auto it = glyphCache.find(c);
    if (it != glyphCache.end())
    {
        return it->second;  // Cache hit
    }
    
    // Cache miss - render and store
    std::vector<uint8_t> rendered = renderChar(c);
    auto result = glyphCache.emplace(c, std::move(rendered));
    return result.first->second;
}

std::vector<uint8_t> FontCache::renderStringOptimized(const std::string& text)
{
    if (!initialized)
    {
        initializeCache();
    }
    
    // Check string cache first for common strings (like time formats)
    auto stringIt = stringCache.find(text);
    if (stringIt != stringCache.end())
    {
        return stringIt->second;  // Full string cache hit
    }
    
    // Render using cached characters
    std::vector<uint8_t> rendered;
    rendered.reserve(text.length() * 6);  // Estimate: ~6 pixels per character average
    
    for (char c : text)
    {
        const auto& glyph = renderCharCached(c);
        rendered.insert(rendered.end(), glyph.begin(), glyph.end());
    }
    
    // Cache the result if it's a reasonable size (avoid caching huge strings)
    if (text.length() <= 32 && stringCache.size() < 100)
    {
        stringCache[text] = rendered;
    }
    
    return rendered;
}

void FontCache::clearCache()
{
    glyphCache.clear();
    stringCache.clear();
    initialized = false;
}

} // namespace font