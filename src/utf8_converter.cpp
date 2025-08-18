#include "utf8_converter.hpp"
#include <iconv.h>
#include <errno.h>
#include <cstring>
#include <iostream>

namespace utf8 {

std::string toLatin1(const std::string& utf8_string) {
    if (utf8_string.empty()) {
        return utf8_string;
    }

    iconv_t cd = iconv_open("ISO-8859-1//TRANSLIT", "UTF-8");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        std::cerr << "Failed to open iconv descriptor" << std::endl;
        return utf8_string;
    }

    // Input and output buffers
    const char* input = utf8_string.c_str();
    size_t input_len = utf8_string.length();
    
    // Allocate output buffer (UTF-8 can be longer than Latin-1)
    size_t output_len = utf8_string.length() * 2;
    std::string result(output_len, '\0');
    char* output = &result[0];
    char* output_ptr = output;
    const char* input_ptr = input;
    
    // Perform conversion
    size_t conv_result = iconv(cd, const_cast<char**>(&input_ptr), &input_len, &output_ptr, &output_len);
    
    // Close conversion descriptor
    iconv_close(cd);
    
    if (conv_result == static_cast<size_t>(-1)) {
        // Conversion error - return best effort result
        std::cerr << "Failed to convert string, errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        result.resize(static_cast<size_t>(output_ptr - output));
        return result;
    }
    
    // Resize to actual converted length
    result.resize(static_cast<size_t>(output_ptr - output));
    return result;
}

} // namespace utf8
