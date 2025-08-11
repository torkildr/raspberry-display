#ifndef font_hpp
#define font_hpp

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace font
{

std::vector<uint8_t> renderString(std::string text);

// Optimized font rendering with caching
class FontCache
{
private:
    static std::unordered_map<char, std::vector<uint8_t>> glyphCache;
    static std::unordered_map<std::string, std::vector<uint8_t>> stringCache;
    static bool initialized;
    
    static void initializeCache();
    static const std::vector<uint8_t>& renderCharCached(char c);
    
public:
    static std::vector<uint8_t> renderStringOptimized(const std::string& text);
    static void clearCache();
};

} // namespace font

#endif
