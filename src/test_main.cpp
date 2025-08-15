#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#define CATCH_CONFIG_MAIN

#include <catch2/catch_all.hpp>

#include "transition.hpp"

TEST_CASE("scroll up", "[transition]") {
    transition::ScrollTransition scroll_transition(transition::ScrollTransition::Direction::UP, 1.0);
    std::array<uint8_t, X_MAX> from_buffer = {0x00, 0xFF};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF, 0x00};

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[0] == 0x00);
    REQUIRE(scroll_transition.update(0.5)[0] == 0xF0);
    REQUIRE(scroll_transition.update(1.0)[0] == 0xFF);

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[1] == 0xFF);
    REQUIRE(scroll_transition.update(0.5)[1] == 0x0F);
    REQUIRE(scroll_transition.update(1.0)[1] == 0x00);
}


TEST_CASE("scroll down", "[transition]") {
    transition::ScrollTransition scroll_transition(transition::ScrollTransition::Direction::DOWN, 1.0);
    std::array<uint8_t, X_MAX> from_buffer = {0x00, 0xFF};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF, 0x00};
    
    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[0] == 0x00);
    REQUIRE(scroll_transition.update(0.5)[0] == 0x0F);
    REQUIRE(scroll_transition.update(1.0)[0] == 0xFF);

    scroll_transition.start(from_buffer, to_buffer);
    REQUIRE(scroll_transition.update(0.0)[1] == 0xFF);
    REQUIRE(scroll_transition.update(0.5)[1] == 0xF0);
    REQUIRE(scroll_transition.update(1.0)[1] == 0x00);
}

TEST_CASE("scroll should move without gaps", "[transition]") {
    std::array<transition::ScrollTransition, 2> scroll_directions = {
        transition::ScrollTransition(transition::ScrollTransition::Direction::UP, 1.0),
        transition::ScrollTransition(transition::ScrollTransition::Direction::DOWN, 1.0),
    };
    std::array<uint8_t, X_MAX> from_buffer = {0x00};
    std::array<uint8_t, X_MAX> to_buffer = {0xFF};

    for (size_t i = 0; i < scroll_directions.size(); ++i) {
        auto& scroll_transition = scroll_directions[i];
        scroll_transition.start(from_buffer, to_buffer);
        
        uint8_t previous = from_buffer[0];
        for (double t = 0.0; t <= 1.0; t += 1/100.0) {
            INFO("Scroll direction: " << i << ", time: " << t);
            auto result = scroll_transition.update(t);
            REQUIRE(result[0] >= previous);
            previous = result[0];
        }
    }
}
