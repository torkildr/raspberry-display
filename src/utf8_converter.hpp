#pragma once

#include <string>

namespace utf8 {
    /**
     * @brief Convert UTF-8 encoded string to Latin-1 (ISO-8859-1) encoding
     * 
     * This function converts UTF-8 input to Latin-1, attempting to preserve
     * as much information as possible by:
     * 1. Direct mapping for characters 0-255 that exist in Latin-1
     * 2. Transliteration for common accented characters (e.g., é → e)
     * 3. Fallback to '?' for unsupported characters
     * 
     * @param utf8_string Input UTF-8 encoded string
     * @return Latin-1 encoded string compatible with display system
     */
    std::string toLatin1(const std::string& utf8_string);
}
