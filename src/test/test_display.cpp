#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>

#include <catch2/catch_all.hpp>

#include "display.hpp"
#include "transition.hpp"
#include "font.hpp"

#define CATCH_CONFIG_MAIN

namespace display {

// TestDisplayImpl - Mock implementation for testing Display class
class TestDisplayImpl : public Display {
public:
    TestDisplayImpl(
        std::function<void()> preUpdate = [](){},
        std::function<void()> postUpdate = [](){},
        DisplayStateCallback stateCallback = nullptr,
        std::function<void()> scrollCompleteCallback = [](){}
    ) : Display(preUpdate, postUpdate, stateCallback, scrollCompleteCallback),
        brightness_set_count(0),
        update_count(0),
        last_brightness(DEFAULT_BRIGHTNESS) {}

    void setBrightness(int brightness) override {
        last_brightness = brightness;
        brightness_set_count++;
        currentBrightness = brightness;
    }

    // Test accessors
    int getBrightnessSetCount() const { return brightness_set_count; }
    int getUpdateCount() const { return update_count; }
    int getLastBrightness() const { return last_brightness; }
    const std::array<uint8_t, X_MAX>& getDisplayBuffer() const { return displayBuffer; }
    int getScrollOffset() const { return scrollOffset; }
    Scrolling getScrollDirection() const { return scrollDirection; }
    size_t getRenderedTextSize() const { return renderedTextSize; }
    
    // Simulate the display update cycle that would normally happen in start()
    void simulateDisplayCycle() {
        // Force the display to update by calling forceUpdate() and then starting/stopping
        // This ensures the display buffer gets updated with current content
        forceUpdate();
        start();
        // Give it a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop();
    }

private:
    void update() override {
        update_count++;
    }

    int brightness_set_count;
    int update_count;
    int last_brightness;
};

} // namespace display

using namespace display;

TEST_CASE("TestDisplayImpl basic functionality", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Initial state") {
        REQUIRE(display.getBrightnessSetCount() == 0);
        REQUIRE(display.getUpdateCount() == 0);
        REQUIRE(display.getLastBrightness() == DEFAULT_BRIGHTNESS);
        REQUIRE(display.getScrollOffset() == 0);
        REQUIRE(display.getScrollDirection() == Scrolling::ENABLED);
        REQUIRE(display.getAlignment() == Alignment::LEFT);
    }
    
    SECTION("setBrightness functionality") {
        display.setBrightness(5);
        REQUIRE(display.getBrightnessSetCount() == 1);
        REQUIRE(display.getLastBrightness() == 5);
        
        display.setBrightness(10);
        REQUIRE(display.getBrightnessSetCount() == 2);
        REQUIRE(display.getLastBrightness() == 10);
    }
}

TEST_CASE("Display text functionality", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Show text only") {
        display.show("Hello World", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        
        // Buffer should contain rendered text
        const auto& buffer = display.getDisplayBuffer();
        bool hasContent = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                hasContent = true;
                break;
            }
        }
        REQUIRE(hasContent);
        REQUIRE(display.getRenderedTextSize() == 44);
    }
    
    SECTION("Show time only") {
        display.show(std::nullopt, "%H:%M");
        display.simulateDisplayCycle(); // Simulate display update cycle
        
        // Buffer should contain rendered time
        const auto& buffer = display.getDisplayBuffer();
        bool hasContent = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                hasContent = true;
                break;
            }
        }
        REQUIRE(hasContent);
    }
    
    SECTION("Show time and text") {
        display.show("Test", "%H:%M");
        display.simulateDisplayCycle(); // Simulate display update cycle
        
        // Buffer should contain both time and text
        const auto& buffer = display.getDisplayBuffer();
        bool hasContent = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                hasContent = true;
                break;
            }
        }
        REQUIRE(hasContent);
    }
}

TEST_CASE("Display alignment functionality", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Left alignment") {
        display.setAlignment(Alignment::LEFT);
        REQUIRE(display.getAlignment() == Alignment::LEFT);
        
        display.show("Test", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        // Content should start from the beginning
        const auto& buffer = display.getDisplayBuffer();
        REQUIRE(buffer[0] != 0); // Should have content at start
    }
    
    SECTION("Center alignment") {
        display.setAlignment(Alignment::CENTER);
        REQUIRE(display.getAlignment() == Alignment::CENTER);
        
        display.show("A", std::nullopt); // Single character should be centered
        display.simulateDisplayCycle(); // Simulate display update cycle
        const auto& buffer = display.getDisplayBuffer();
        
        // Find where content starts (should not be at position 0 for centered short text)
        bool foundContent = false;
        size_t contentStart = 0;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                contentStart = i;
                foundContent = true;
                break;
            }
        }
        REQUIRE(foundContent);
        REQUIRE(contentStart > 0); // Should be offset from start for centering
    }
}

TEST_CASE("Display scrolling functionality", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Scrolling enabled") {
        display.setScrolling(Scrolling::ENABLED);
        REQUIRE(display.getScrollDirection() == Scrolling::ENABLED);
        REQUIRE(display.getScrollOffset() == 0);
    }
    
    SECTION("Scrolling disabled") {
        display.setScrolling(Scrolling::DISABLED);
        REQUIRE(display.getScrollDirection() == Scrolling::DISABLED);
        REQUIRE(display.getScrollOffset() == 0);
    }
    
    SECTION("Scrolling reset") {
        // First set some scroll state
        display.setScrolling(Scrolling::ENABLED);
        display.show("Very long text that should definitely scroll because it exceeds display width", std::nullopt);
        
        // Reset scrolling
        display.setScrolling(Scrolling::RESET);
        REQUIRE(display.getScrollOffset() == 0);
    }
}

TEST_CASE("Display state callback functionality", "[display]") {
    std::string callback_text;
    std::string callback_time_format;
    int callback_brightness = -1;
    bool callback_called = false;
    
    DisplayStateCallback callback = [&](const std::string& text, const std::string& time_format, int brightness) {
        callback_text = text;
        callback_time_format = time_format;
        callback_brightness = brightness;
        callback_called = true;
    };
    
    TestDisplayImpl display([](){}, [](){}, callback);
    
    SECTION("Callback triggered on show") {
        display.show("Test Message", "%H:%M:%S");
        
        REQUIRE(callback_called);
        REQUIRE(callback_text == "Test Message");
        REQUIRE(callback_time_format == "%H:%M:%S");
        REQUIRE(callback_brightness == DEFAULT_BRIGHTNESS);
    }
}

TEST_CASE("Display transition functionality", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Set default transition") {
        display.setTransition(transition::Type::SCROLL_UP, 2.0);
        REQUIRE_FALSE(display.isTransitioning()); // Should not be transitioning initially
    }
    
    SECTION("Transition with show") {
        display.show("Initial", std::nullopt);
        display.show("New Text", std::nullopt, transition::Type::SCROLL_UP, 1.0);
        
        // Should be transitioning immediately after starting transition
        REQUIRE(display.isTransitioning());
    }
}

TEST_CASE("Display edge cases", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Empty text") {
        display.show("", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        // Should handle empty text gracefully
        REQUIRE(display.getRenderedTextSize() == 0); // Buffer size should still be X_MAX
    }
    
    SECTION("Null optional values") {
        display.show(std::nullopt, std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        // Should handle null values gracefully - defaults to text mode with empty text
        REQUIRE(display.getRenderedTextSize() == 0);
    }
    
    SECTION("Empty time format") {
        display.show("Text", "");
        display.simulateDisplayCycle(); // Simulate display update cycle
        // Empty time format should use default
        const auto& buffer = display.getDisplayBuffer();
        bool hasContent = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                hasContent = true;
                break;
            }
        }
        REQUIRE(hasContent);
    }
    
    SECTION("Very long text") {
        std::string longText(500, 'A'); // Very long text
        display.show(longText, std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        
        // Should handle long text without crashing
        REQUIRE(display.getRenderedTextSize() == 2500);
        
        // Buffer should be filled
        const auto& buffer = display.getDisplayBuffer();
        bool hasContent = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer[i] != 0) {
                hasContent = true;
                break;
            }
        }
        REQUIRE(hasContent);
    }
    
    SECTION("Invalid brightness values") {
        // Test extreme brightness values
        display.setBrightness(-1);
        REQUIRE(display.getLastBrightness() == -1); // Should accept but may be clamped by implementation
        
        display.setBrightness(1000);
        REQUIRE(display.getLastBrightness() == 1000); // Should accept but may be clamped by implementation
    }
    
    SECTION("Multiple force updates") {
        display.forceUpdate();
        display.forceUpdate();
        display.forceUpdate();
        // Should handle multiple force updates without issues
    }
    
    SECTION("Start and stop multiple times") {
        display.start();
        display.stop();
        display.start();
        display.stop();
        // Should handle multiple start/stop cycles gracefully
    }
}

TEST_CASE("Display callback edge cases", "[display]") {
    int pre_update_count = 0;
    int post_update_count = 0;
    int scroll_complete_count = 0;
    
    auto preUpdate = [&]() { pre_update_count++; };
    auto postUpdate = [&]() { post_update_count++; };
    auto scrollComplete = [&]() { scroll_complete_count++; };
    
    TestDisplayImpl display(preUpdate, postUpdate, nullptr, scrollComplete);
    
    SECTION("Callbacks with null state callback") {
        display.show("Test", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        // Should handle null state callback gracefully
        REQUIRE(display.getRenderedTextSize() == 20);
    }
}

TEST_CASE("Display buffer consistency", "[display]") {
    TestDisplayImpl display;
    
    SECTION("Buffer size consistency") {
        display.show("Short", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        REQUIRE(display.getRenderedTextSize() == 24);
        
        display.show("Much longer text that exceeds normal display width", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        REQUIRE(display.getRenderedTextSize() == 214);
        
        display.show("", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        REQUIRE(display.getRenderedTextSize() == 0);
    }
    
    SECTION("Buffer content changes") {
        display.show("First", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        auto buffer1 = display.getDisplayBuffer();
        
        display.show("Second", std::nullopt);
        display.simulateDisplayCycle(); // Simulate display update cycle
        auto buffer2 = display.getDisplayBuffer();
        
        // Buffers should be different for different content
        bool different = false;
        for (size_t i = 0; i < X_MAX; i++) {
            if (buffer1[i] != buffer2[i]) {
                different = true;
                break;
            }
        }
        REQUIRE(different);
    }
}
