#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include "display.hpp"
#include "transition.hpp"
#include "utf8_converter.hpp"
#include "font.hpp"

#define CATCH_CONFIG_MAIN

std::string to_binary(long long);
std::string to_hex_string(const std::string& str);
void test_utf8_conversion(const std::string& utf8_input, const std::string& expected_latin1);

std::string to_binary(long long value) {
    std::string binary_string;
    while (value > 0) {
        binary_string.insert(binary_string.begin(), (value % 2) ? '1' : '0');
        value /= 2;
    }
    return binary_string.empty() ? "0" : binary_string;
}

TEST_CASE("scroll up", "[transition]") {
    transition::ScrollTransition scroll_transition(transition::ScrollTransition::Direction::UP, 1.0);
    std::array<uint8_t, X_MAX> from_buffer = {0x00, 0xFF};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF, 0x00};

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[0] == 0x00);
    REQUIRE(scroll_transition.update(0.5)[0] == 0xF0);
    REQUIRE(scroll_transition.update(0.5)[0] == 0xFF);

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[1] == 0xFF);
    REQUIRE(scroll_transition.update(0.5)[1] == 0x0F);
    REQUIRE(scroll_transition.update(0.5)[1] == 0x00);
}

TEST_CASE("scroll down", "[transition]") {
    transition::ScrollTransition scroll_transition(transition::ScrollTransition::Direction::DOWN, 1.0);
    std::array<uint8_t, X_MAX> from_buffer = {0x00, 0xFF};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF, 0x00};
    
    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[0] == 0x00);
    REQUIRE(scroll_transition.update(0.5)[0] == 0x0F);
    REQUIRE(scroll_transition.update(0.5)[0] == 0xFF);

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[1] == 0xFF);
    REQUIRE(scroll_transition.update(0.5)[1] == 0xF0);
    REQUIRE(scroll_transition.update(0.5)[1] == 0x00);
}

TEST_CASE("scroll should move without gaps", "[transition]") {
    double duration = 3.0;
    std::array<transition::ScrollTransition, 2> scroll_directions = {
        transition::ScrollTransition(transition::ScrollTransition::Direction::UP, duration),
        transition::ScrollTransition(transition::ScrollTransition::Direction::DOWN, duration),
    };
    std::array<uint8_t, X_MAX> from_buffer = {0x00};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF};

    for (size_t i = 0; i < scroll_directions.size(); ++i) {
        auto& scroll_transition = scroll_directions[i];
        scroll_transition.start(from_buffer, to_buffer);
        
        uint8_t previous = from_buffer[0];
        double step = duration * (1.0 / REFRESH_RATE);
        for (double t = 0.0; t < duration; t += step) {
            INFO("Scroll direction: " << ", time: " << t);
            auto result = scroll_transition.update(step);
            
            REQUIRE(result[0] >= previous);
            previous = result[0];
        }
    }
}

TEST_CASE("wipe right", "[transition]") {
    transition::WipeTransition wipe_transition(transition::WipeTransition::Direction::LEFT_TO_RIGHT, 1.0);
    std::array<uint8_t, X_MAX> from_buffer = {{}};
    std::array<uint8_t, X_MAX> to_buffer = {{}};
    for (size_t i = 0; i < X_MAX; ++i) {
        from_buffer[i] = static_cast<uint8_t>(0x00);
        to_buffer[i] = static_cast<uint8_t>(0xFF);
    }

    wipe_transition.start(from_buffer, to_buffer);

    struct VerificationData {
        unsigned long x_cord;
        uint8_t expected;
    };

    auto step_and_verify = [&](double step, std::vector<VerificationData> expected)   {
        auto buffer = wipe_transition.update(step);
        for (const auto& data : expected) {
            INFO("Step: " << step);
            INFO("X: " << data.x_cord);
            REQUIRE_THAT(
                to_binary(buffer[data.x_cord]),
                Catch::Matchers::Equals(to_binary(data.expected))
            );
        }
    };

    INFO("T = 0.0");
    step_and_verify(0.0, {
        {0, 0xFF},
        {1, 0x00},
        {63, 0x00},
        {127, 0x00},
    });
    INFO("T = 0.5");
    step_and_verify(0.5, {
        {0, 0xFF},
        {61, 0xFF},
        {62, 0xFF},
        {63, 0x00},
        {63, 0x00},
        {127, 0x00},
    });
    INFO("T = 1.0");
    step_and_verify(0.5, {
        {0, 0xFF},
        {63, 0xFF},
        {127, 0xFF},
    });
}

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
