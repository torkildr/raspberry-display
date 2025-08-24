#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include "display.hpp"
#include "transition.hpp"

#define CATCH_CONFIG_MAIN

std::string to_binary(long long);

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
        from_buffer[i] = static_cast<uint8_t>(0x0F);
        to_buffer[i] = static_cast<uint8_t>(0xF0);
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

    step_and_verify(0.0, {
        {0, 0x00},
        {1, 0x0F & 0b00100100},
        {63, 0x0F},
        {127, 0x0F},
    });
    step_and_verify(0.5, {
        {0, 0xF0},
        {61, 0xF0},
        {62, 0xF0 & 0b11011011},
        {63, 0xF0 & 0b00100100},
        {64, 0x00},
        {65, 0x0F & 0b00100100},
        {66, 0x0F & 0b11011011},
        {127, 0x0F},
    });
    step_and_verify(0.5, {
        {0, 0xF0},
        {1, 0xF0},
        {2, 0xF0},
        {63, 0xF0},
        {127, 0xF0},
    });
}
