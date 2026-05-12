#define CATCH_CONFIG_MAIN

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_all.hpp>

#include "display_test_helpers.hpp"
#include "display.hpp"

using display_test::RecordingTextRenderer;
using display_test::TestDisplay;

TEST_CASE("Display bitmap helper verifies exact font output", "[display][bitmap]")
{
    TestDisplay display;

    display.show("A", std::nullopt);
    display.simulateDisplayCycle();

    display_test::requireBufferPrefixEquals(display.buffer(), R"(
.##..
#..#.
#..#.
####.
#..#.
#..#.
#..#.
.....
)");
}

TEST_CASE("Display sends text to renderer before bitmap layout", "[display][renderer]")
{
    RecordingTextRenderer renderer;
    renderer.willRender("HELLO", display_test::columnsFromArt(R"(
##..
.#..
..#.
...#
....
....
....
....
)"));

    TestDisplay display(renderer.callback());

    display.show("HELLO", std::nullopt);
    display.simulateDisplayCycle();

    REQUIRE(renderer.renderedTexts() == std::vector<std::string>{"HELLO"});
    display_test::requireBufferPrefixEquals(display.buffer(), R"(
##..
.#..
..#.
...#
....
....
....
....
)");
}

TEST_CASE("Display sends formatted time text to renderer", "[display][renderer]")
{
    RecordingTextRenderer renderer;
    renderer.willRender("CLOCK", display_test::columnsFromArt(R"(
#.
##
#.
#.
#.
#.
#.
#.
)"));

    TestDisplay display(renderer.callback());

    display.show(std::nullopt, "CLOCK");
    display.simulateDisplayCycle();

    REQUIRE(renderer.renderedTexts() == std::vector<std::string>{"CLOCK"});
    display_test::requireBufferPrefixEquals(display.buffer(), R"(
#.
##
#.
#.
#.
#.
#.
#.
)");
}

TEST_CASE("Display centers deterministic rendered content", "[display][bitmap][alignment]")
{
    RecordingTextRenderer renderer;
    renderer.willRender("DOTS", std::vector<uint8_t>{0x01, 0x01, 0x01, 0x01});
    TestDisplay display(renderer.callback());

    display.setAlignment(display::Alignment::CENTER);
    display.show("DOTS", std::nullopt);
    display.simulateDisplayCycle();

    REQUIRE(display_test::firstLitColumn(display.buffer()) == 62);
    REQUIRE(display_test::lastLitColumn(display.buffer()) == 65);
}

TEST_CASE("Display scrolls deterministic content by bitmap column", "[display][bitmap][scroll]")
{
    RecordingTextRenderer renderer;
    std::vector<uint8_t> rendered(130, 0x10);
    rendered[0] = 0x01;
    rendered[1] = 0x02;
    rendered[127] = 0x40;
    rendered[128] = 0x80;
    renderer.willRender("SCROLL", rendered);

    TestDisplay display(renderer.callback());

    display.setScrolling(display::Scrolling::ENABLED);
    display.show("SCROLL", std::nullopt);
    display.simulateDisplayCycle();

    REQUIRE(display.scrollOffsetValue() == 0);
    REQUIRE(display.buffer()[0] == 0x01);
    REQUIRE(display.buffer()[127] == 0x40);

    display.simulateDisplayCycle(static_cast<int>(std::ceil(REFRESH_RATE * SCROLL_DELAY)));

    REQUIRE(display.scrollOffsetValue() == 1);
    REQUIRE(display.buffer()[0] == 0x02);
    REQUIRE(display.buffer()[127] == 0x80);
}

TEST_CASE("Display reports scroll completion after visible content reaches the end", "[display][scroll]")
{
    int scrollCompleteCount = 0;
    RecordingTextRenderer renderer;
    renderer.willRender("END", std::vector<uint8_t>(129, 0x01));
    TestDisplay display(nullptr, [&scrollCompleteCount]() { ++scrollCompleteCount; }, renderer.callback());

    display.setScrolling(display::Scrolling::ENABLED);
    display.show("END", std::nullopt);
    display.simulateDisplayCycle();

    display.simulateDisplayCycle(static_cast<int>(std::ceil(REFRESH_RATE * SCROLL_DELAY)));
    REQUIRE(display.scrollOffsetValue() == 1);
    REQUIRE(scrollCompleteCount == 0);

    display.simulateDisplayCycle(static_cast<int>(std::ceil(REFRESH_RATE * SCROLL_DELAY)));
    display.simulateDisplayCycle();

    REQUIRE(display.scrollOffsetValue() == 0);
    REQUIRE(scrollCompleteCount == 1);
}
