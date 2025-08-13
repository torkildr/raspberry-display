#include "utf8_converter.hpp"
#include <unordered_map>
#include <cstdint>

namespace utf8 {

namespace {
    // Transliteration map for common Unicode characters to Latin-1 equivalents
    const std::unordered_map<uint32_t, char> transliteration_map = {
        // Latin Extended-A (U+0100-U+017F)
        {0x0100, 'A'}, {0x0101, 'a'}, // Ā, ā
        {0x0102, 'A'}, {0x0103, 'a'}, // Ă, ă
        {0x0104, 'A'}, {0x0105, 'a'}, // Ą, ą
        {0x0106, 'C'}, {0x0107, 'c'}, // Ć, ć
        {0x0108, 'C'}, {0x0109, 'c'}, // Ĉ, ĉ
        {0x010A, 'C'}, {0x010B, 'c'}, // Ċ, ċ
        {0x010C, 'C'}, {0x010D, 'c'}, // Č, č
        {0x010E, 'D'}, {0x010F, 'd'}, // Ď, ď
        {0x0110, 'D'}, {0x0111, 'd'}, // Đ, đ
        {0x0112, 'E'}, {0x0113, 'e'}, // Ē, ē
        {0x0114, 'E'}, {0x0115, 'e'}, // Ĕ, ĕ
        {0x0116, 'E'}, {0x0117, 'e'}, // Ė, ė
        {0x0118, 'E'}, {0x0119, 'e'}, // Ę, ę
        {0x011A, 'E'}, {0x011B, 'e'}, // Ě, ě
        {0x011C, 'G'}, {0x011D, 'g'}, // Ĝ, ĝ
        {0x011E, 'G'}, {0x011F, 'g'}, // Ğ, ğ
        {0x0120, 'G'}, {0x0121, 'g'}, // Ġ, ġ
        {0x0122, 'G'}, {0x0123, 'g'}, // Ģ, ģ
        {0x0124, 'H'}, {0x0125, 'h'}, // Ĥ, ĥ
        {0x0126, 'H'}, {0x0127, 'h'}, // Ħ, ħ
        {0x0128, 'I'}, {0x0129, 'i'}, // Ĩ, ĩ
        {0x012A, 'I'}, {0x012B, 'i'}, // Ī, ī
        {0x012C, 'I'}, {0x012D, 'i'}, // Ĭ, ĭ
        {0x012E, 'I'}, {0x012F, 'i'}, // Į, į
        {0x0130, 'I'}, {0x0131, 'i'}, // İ, ı
        {0x0134, 'J'}, {0x0135, 'j'}, // Ĵ, ĵ
        {0x0136, 'K'}, {0x0137, 'k'}, // Ķ, ķ
        {0x0139, 'L'}, {0x013A, 'l'}, // Ĺ, ĺ
        {0x013B, 'L'}, {0x013C, 'l'}, // Ļ, ļ
        {0x013D, 'L'}, {0x013E, 'l'}, // Ľ, ľ
        {0x0141, 'L'}, {0x0142, 'l'}, // Ł, ł
        {0x0143, 'N'}, {0x0144, 'n'}, // Ń, ń
        {0x0145, 'N'}, {0x0146, 'n'}, // Ņ, ņ
        {0x0147, 'N'}, {0x0148, 'n'}, // Ň, ň
        {0x014C, 'O'}, {0x014D, 'o'}, // Ō, ō
        {0x014E, 'O'}, {0x014F, 'o'}, // Ŏ, ŏ
        {0x0150, 'O'}, {0x0151, 'o'}, // Ő, ő
        {0x0154, 'R'}, {0x0155, 'r'}, // Ŕ, ŕ
        {0x0156, 'R'}, {0x0157, 'r'}, // Ř, ř
        {0x0158, 'R'}, {0x0159, 'r'}, // Ř, ř
        {0x015A, 'S'}, {0x015B, 's'}, // Ś, ś
        {0x015C, 'S'}, {0x015D, 's'}, // Ŝ, ŝ
        {0x015E, 'S'}, {0x015F, 's'}, // Ş, ş
        {0x0160, 'S'}, {0x0161, 's'}, // Š, š
        {0x0162, 'T'}, {0x0163, 't'}, // Ţ, ţ
        {0x0164, 'T'}, {0x0165, 't'}, // Ť, ť
        {0x0166, 'T'}, {0x0167, 't'}, // Ŧ, ŧ
        {0x0168, 'U'}, {0x0169, 'u'}, // Ũ, ũ
        {0x016A, 'U'}, {0x016B, 'u'}, // Ū, ū
        {0x016C, 'U'}, {0x016D, 'u'}, // Ŭ, ŭ
        {0x016E, 'U'}, {0x016F, 'u'}, // Ů, ů
        {0x0170, 'U'}, {0x0171, 'u'}, // Ű, ű
        {0x0172, 'U'}, {0x0173, 'u'}, // Ų, ų
        {0x0174, 'W'}, {0x0175, 'w'}, // Ŵ, ŵ
        {0x0176, 'Y'}, {0x0177, 'y'}, // Ŷ, ŷ
        {0x0178, 'Y'},                 // Ÿ
        {0x0179, 'Z'}, {0x017A, 'z'}, // Ź, ź
        {0x017B, 'Z'}, {0x017C, 'z'}, // Ż, ż
        {0x017D, 'Z'}, {0x017E, 'z'}, // Ž, ž
        
        // Common punctuation and symbols
        {0x2010, '-'}, {0x2011, '-'}, {0x2012, '-'}, {0x2013, '-'}, {0x2014, '-'}, // Various dashes
        {0x2018, '\''}, {0x2019, '\''}, // Left/right single quotation marks
        {0x201C, '"'}, {0x201D, '"'},   // Left/right double quotation marks
        {0x2022, '*'},                  // Bullet
        {0x2026, '.'},                  // Horizontal ellipsis
        {0x20AC, 'E'},                  // Euro sign
        
        // Mathematical symbols
        {0x2212, '-'},                  // Minus sign
        {0x00D7, 'x'},                  // Multiplication sign (already in Latin-1, but for completeness)
        
        // Some Cyrillic basics (transliteration)
        {0x0410, 'A'}, {0x0430, 'a'}, // А, а
        {0x0411, 'B'}, {0x0431, 'b'}, // Б, б
        {0x0412, 'V'}, {0x0432, 'v'}, // В, в
        {0x0413, 'G'}, {0x0433, 'g'}, // Г, г
        {0x0414, 'D'}, {0x0434, 'd'}, // Д, д
        {0x0415, 'E'}, {0x0435, 'e'}, // Е, е
        {0x041A, 'K'}, {0x043A, 'k'}, // К, к
        {0x041C, 'M'}, {0x043C, 'm'}, // М, м
        {0x041D, 'N'}, {0x043D, 'n'}, // Н, н
        {0x041E, 'O'}, {0x043E, 'o'}, // О, о
        {0x041F, 'P'}, {0x043F, 'p'}, // П, п
        {0x0420, 'R'}, {0x0440, 'r'}, // Р, р
        {0x0421, 'S'}, {0x0441, 's'}, // С, с
        {0x0422, 'T'}, {0x0442, 't'}, // Т, т
        {0x0423, 'U'}, {0x0443, 'u'}, // У, у
        {0x0425, 'X'}, {0x0445, 'x'}, // Х, х
    };

    // Decode a single UTF-8 character and return codepoint + bytes consumed
    std::pair<uint32_t, int> decodeUTF8Char(const char* str, size_t len) {
        if (len == 0) return {0, 0};
        
        unsigned char first = static_cast<unsigned char>(str[0]);
        
        // ASCII (0xxxxxxx)
        if (first < 0x80) {
            return {static_cast<uint32_t>(first), 1};
        }
        
        // 2-byte sequence (110xxxxx 10xxxxxx)
        if ((first & 0xE0) == 0xC0 && len >= 2) {
            unsigned char second = static_cast<unsigned char>(str[1]);
            if ((second & 0xC0) == 0x80) {
                uint32_t codepoint = ((static_cast<uint32_t>(first) & 0x1F) << 6) | (static_cast<uint32_t>(second) & 0x3F);
                return {codepoint, 2};
            }
        }
        
        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        if ((first & 0xF0) == 0xE0 && len >= 3) {
            unsigned char second = static_cast<unsigned char>(str[1]);
            unsigned char third = static_cast<unsigned char>(str[2]);
            if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80) {
                uint32_t codepoint = ((static_cast<uint32_t>(first) & 0x0F) << 12) | 
                                   ((static_cast<uint32_t>(second) & 0x3F) << 6) | 
                                   (static_cast<uint32_t>(third) & 0x3F);
                return {codepoint, 3};
            }
        }
        
        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((first & 0xF8) == 0xF0 && len >= 4) {
            unsigned char second = static_cast<unsigned char>(str[1]);
            unsigned char third = static_cast<unsigned char>(str[2]);
            unsigned char fourth = static_cast<unsigned char>(str[3]);
            if ((second & 0xC0) == 0x80 && (third & 0xC0) == 0x80 && (fourth & 0xC0) == 0x80) {
                uint32_t codepoint = ((static_cast<uint32_t>(first) & 0x07) << 18) | 
                                   ((static_cast<uint32_t>(second) & 0x3F) << 12) |
                                   ((static_cast<uint32_t>(third) & 0x3F) << 6) | 
                                   (static_cast<uint32_t>(fourth) & 0x3F);
                return {codepoint, 4};
            }
        }
        
        // Invalid UTF-8 sequence - consume 1 byte and return replacement
        return {0xFFFD, 1}; // Unicode replacement character
    }
}

std::string toLatin1(const std::string& utf8_string) {
    std::string result;
    result.reserve(utf8_string.size()); // Reserve at least input size
    
    const char* str = utf8_string.c_str();
    size_t len = utf8_string.length();
    size_t pos = 0;
    
    while (pos < len) {
        auto [codepoint, bytes] = decodeUTF8Char(str + pos, len - pos);
        pos += static_cast<size_t>(bytes);
        
        if (bytes == 0) {
            // Should not happen, but safety fallback
            break;
        }
        
        // Handle the decoded codepoint
        if (codepoint <= 0xFF) {
            // Direct mapping to Latin-1 (0-255)
            result += static_cast<char>(codepoint);
        } else {
            // Try transliteration map
            auto it = transliteration_map.find(codepoint);
            if (it != transliteration_map.end()) {
                result += it->second;
            } else {
                // Fallback to replacement character
                result += '?';
            }
        }
    }
    
    return result;
}

} // namespace utf8
