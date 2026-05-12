#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <catch2/catch_all.hpp>

#include "display.hpp"
#include "font.hpp"

namespace display_test
{

class TestDisplay : public display::Display
{
public:
    explicit TestDisplay(display::TextRenderer textRenderer)
        : TestDisplay(nullptr, []() {}, std::move(textRenderer))
    {
    }

    TestDisplay(
        display::DisplayStateCallback stateCallback = nullptr,
        std::function<void()> scrollCompleteCallback = []() {},
        display::TextRenderer textRenderer = nullptr
    )
        : display::Display(
            []() {},
            []() {},
            std::move(stateCallback),
            std::move(scrollCompleteCallback),
            std::move(textRenderer)
        )
    {
    }

    void setBrightness(int brightness) override
    {
        lastBrightness = brightness;
        ++brightnessSetCount;
        currentBrightness = brightness;
    }

    void simulateDisplayCycle(int cycles = 1)
    {
        for (int i = 0; i < cycles; ++i) {
            const bool hasChanges = prepare();
            if (hasChanges || isTransitioning()) {
                update();
            }
        }
    }

    const std::array<uint8_t, X_MAX>& buffer() const
    {
        return displayBuffer;
    }

    int scrollOffsetValue() const
    {
        return scrollOffset;
    }

    display::Scrolling scrollDirectionValue() const
    {
        return scrollDirection;
    }

    size_t renderedTextSizeValue() const
    {
        return renderedTextSize;
    }

    int brightnessSetCountValue() const
    {
        return brightnessSetCount;
    }

    int lastBrightnessValue() const
    {
        return lastBrightness;
    }

    int updateCountValue() const
    {
        return updateCount;
    }

private:
    void update() override
    {
        ++updateCount;
    }

    int brightnessSetCount = 0;
    int updateCount = 0;
    int lastBrightness = DEFAULT_BRIGHTNESS;
};

class RecordingTextRenderer
{
public:
    void willRender(const std::string& text, std::vector<uint8_t> columns)
    {
        renderResults[text] = std::move(columns);
    }

    display::TextRenderer callback()
    {
        return [this](const std::string& text) {
            return render(text);
        };
    }

    const std::vector<std::string>& renderedTexts() const
    {
        return texts;
    }

private:
    std::vector<uint8_t> render(const std::string& text)
    {
        texts.push_back(text);

        const auto configured = renderResults.find(text);
        if (configured != renderResults.end()) {
            return configured->second;
        }

        return font::FontCache::renderStringOptimized(text);
    }

    std::vector<std::string> texts;
    std::unordered_map<std::string, std::vector<uint8_t>> renderResults;
};

inline std::vector<std::string> artLines(std::string_view art)
{
    std::vector<std::string> lines;
    std::string current;

    for (const char ch : art) {
        if (ch == '\n') {
            if (!current.empty() && current.back() == '\r') {
                current.pop_back();
            }
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }

    if (!current.empty()) {
        if (current.back() == '\r') {
            current.pop_back();
        }
        lines.push_back(current);
    }

    while (!lines.empty() && lines.front().empty()) {
        lines.erase(lines.begin());
    }

    while (!lines.empty() && lines.back().empty()) {
        lines.pop_back();
    }

    return lines;
}

inline std::vector<uint8_t> columnsFromArt(std::string_view art)
{
    constexpr size_t displayHeight = 8;
    const auto lines = artLines(art);

    REQUIRE(lines.size() == displayHeight);

    const size_t width = lines.front().size();
    std::vector<uint8_t> columns(width, 0);

    for (size_t y = 0; y < displayHeight; ++y) {
        REQUIRE(lines[y].size() == width);

        for (size_t x = 0; x < width; ++x) {
            const char pixel = lines[y][x];
            REQUIRE((pixel == '.' || pixel == '#'));

            if (pixel == '#') {
                columns[x] = static_cast<uint8_t>(columns[x] | static_cast<uint8_t>(1U << y));
            }
        }
    }

    return columns;
}

inline std::string columnsToArt(const std::vector<uint8_t>& columns)
{
    constexpr size_t displayHeight = 8;
    std::string art;

    for (size_t y = 0; y < displayHeight; ++y) {
        for (const uint8_t column : columns) {
            art.push_back((column & static_cast<uint8_t>(1U << y)) != 0 ? '#' : '.');
        }
        if (y + 1 < displayHeight) {
            art.push_back('\n');
        }
    }

    return art;
}

inline std::vector<uint8_t> bufferPrefix(const std::array<uint8_t, X_MAX>& buffer, size_t width)
{
    REQUIRE(width <= buffer.size());
    return {buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(width)};
}

inline std::string bufferPrefixToArt(const std::array<uint8_t, X_MAX>& buffer, size_t width)
{
    return columnsToArt(bufferPrefix(buffer, width));
}

inline void requireBufferPrefixEquals(const std::array<uint8_t, X_MAX>& buffer, std::string_view expectedArt)
{
    const auto expected = columnsFromArt(expectedArt);
    const auto actual = bufferPrefix(buffer, expected.size());

    INFO("Expected:\n" << columnsToArt(expected));
    INFO("Actual:\n" << columnsToArt(actual));

    REQUIRE(actual == expected);
}

inline std::optional<size_t> firstLitColumn(const std::array<uint8_t, X_MAX>& buffer)
{
    for (size_t i = 0; i < buffer.size(); ++i) {
        if (buffer[i] != 0) {
            return i;
        }
    }

    return std::nullopt;
}

inline std::optional<size_t> lastLitColumn(const std::array<uint8_t, X_MAX>& buffer)
{
    for (size_t i = buffer.size(); i > 0; --i) {
        const size_t index = i - 1;
        if (buffer[index] != 0) {
            return index;
        }
    }

    return std::nullopt;
}

} // namespace display_test
