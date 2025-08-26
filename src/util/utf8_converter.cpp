#include <iconv.h>
#include <errno.h>
#include <cstring>
#include <iostream>

#include "utf8_converter.hpp"

namespace utf8 {

// Internal helper function to perform iconv conversion
static std::string convertEncoding(const std::string& input, const char* to_encoding, const char* from_encoding, size_t buffer_multiplier = 4) {
    if (input.empty()) {
        return input;
    }

    iconv_t cd = iconv_open(to_encoding, from_encoding);
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        std::cerr << "Failed to open iconv descriptor for " << from_encoding << " to " << to_encoding << std::endl;
        return input;
    }

    // Input buffer
    const char* input_ptr = input.c_str();
    size_t input_len = input.length();
    
    // Output buffer - allocate with multiplier for potential expansion
    size_t output_len = input.length() * buffer_multiplier;
    std::string result(output_len, '\0');
    char* output = &result[0];
    char* output_ptr = output;
    
    // Perform conversion
    size_t conv_result = iconv(cd, const_cast<char**>(&input_ptr), &input_len, &output_ptr, &output_len);
    
    // Close conversion descriptor
    iconv_close(cd);
    
    if (conv_result == static_cast<size_t>(-1)) {
        // Conversion error - return best effort result
        std::cerr << "Failed to convert from " << from_encoding << " to " << to_encoding 
                  << ", errno: " << errno << " (" << strerror(errno) << ")" << std::endl;
        result.resize(static_cast<size_t>(output_ptr - output));
        return result;
    }
    
    // Resize to actual converted length
    result.resize(static_cast<size_t>(output_ptr - output));
    return result;
}

std::string toLatin1(const std::string& utf8_string) {
    return convertEncoding(utf8_string, "ISO-8859-1//TRANSLIT", "UTF-8", 2);
}

std::string toUtf8(const std::string& latin1_string) {
    return convertEncoding(latin1_string, "UTF-8", "ISO-8859-1", 4);
}

} // namespace utf8
