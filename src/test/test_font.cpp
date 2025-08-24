#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include "utf8_converter.hpp"
#include "font.hpp"

#define CATCH_CONFIG_MAIN

std::string to_hex_string(const std::string& str);
void test_utf8_conversion(const std::string& utf8_input, const std::string& expected_latin1);

std::string to_hex_string(const std::string& str) {
    std::string hex_str;
    for (size_t i = 0; i < str.length(); ++i) {
        char hex_char[8];
        snprintf(hex_char, sizeof(hex_char), "\\x%02x", static_cast<unsigned char>(str[i]));
        hex_str += hex_char;
    }
    return hex_str;
}

void test_utf8_conversion(const std::string& utf8_input, const std::string& expected_latin1) {
    std::string actual = utf8::toLatin1(utf8_input);
    INFO("Expected: " << to_hex_string(expected_latin1));
    INFO("Actual:   " << to_hex_string(actual));
    REQUIRE(actual == expected_latin1);
}

TEST_CASE("UTF-8 to iso8859-1 conversion", "[utf8]") {
    test_utf8_conversion("Hello, World!", "Hello, World!");
    test_utf8_conversion("\xc3\x86\xc3\x98\xc3\x85", "\xc6\xd8\xc5");
    test_utf8_conversion("Café", "Caf\xe9");
    test_utf8_conversion("\xc2\xb0\xc2\xba", "\xb0\xba");
}

TEST_CASE("Character mapping functionality", "[font]") {
    // Test compile-time character mapping - 0xba should map to 0xb0's glyph
    char char_b0 = static_cast<char>(0xb0);
    char char_ba = static_cast<char>(0xba);
    
    // Render both characters and verify they produce identical output
    std::vector<uint8_t> original = font::renderString(std::string(1, char_b0));
    std::vector<uint8_t> mapped = font::renderString(std::string(1, char_ba));
    
    INFO("Original (0xb0): " << to_hex_string(std::string(original.begin(), original.end())));
    INFO("Mapped (0xba):   " << to_hex_string(std::string(mapped.begin(), mapped.end())));
    REQUIRE(original == mapped);
    
    // Test that mapping works in strings
    std::string original_str = "A" + std::string(1, char_b0) + "B";
    std::string mapped_str = "A" + std::string(1, char_ba) + "B";
    std::vector<uint8_t> mixed_original = font::renderString(original_str);
    std::vector<uint8_t> mixed_mapped = font::renderString(mapped_str);
    REQUIRE(mixed_original == mixed_mapped);
}

TEST_CASE("Composite character", "[font]") {
    test_utf8_conversion("\xc3\x9f", "\xdf");
    test_utf8_conversion("\uFB01", "fi");
    test_utf8_conversion("A\u2103", "A?");
    test_utf8_conversion("\u0142", "?");
}
