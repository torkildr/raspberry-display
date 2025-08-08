#ifndef font_hpp
#define font_hpp

#include <string>
#include <vector>
#include <unordered_map>

namespace font
{

std::vector<char> renderString(std::string text);

// Optimized font rendering with caching
class FontCache
{
private:
    static std::unordered_map<char, std::vector<char>> glyphCache;
    static std::unordered_map<std::string, std::vector<char>> stringCache;
    static bool initialized;
    
    static void initializeCache();
    static const std::vector<char>& renderCharCached(char c);
    
public:
    static std::vector<char> renderStringOptimized(const std::string& text);
    static void clearCache();
};

} // namespace font

#endif
