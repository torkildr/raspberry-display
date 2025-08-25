#ifndef display_hpp
#define display_hpp

#include <array>
#include <string>
#include <optional>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

#include "timer.hpp"
#include "transition.hpp"

#define X_MAX 128

#define SCROLL_DELAY 2.0   // Seconds
#define REFRESH_RATE 15  // Hz

#define DEFAULT_BRIGHTNESS 8

#define TIME_FORMAT_LONG "%A, %b %d %H:%M:%S"
#define TIME_FORMAT_SHORT "%H:%M"

namespace display
{

enum Scrolling
{
    DISABLED,
    ENABLED,
    RESET,
};

enum Mode
{
    TIME,
    TIME_AND_TEXT,
    TEXT,
};

enum Alignment
{
    LEFT,
    CENTER,
};

using DisplayStateCallback = std::function<void(const std::string& text, const std::string& time_format, int brightness)>;

class Display
{
public:
    Display(
        std::function<void()> preUpdate,
        std::function<void()> postUpdate,
        DisplayStateCallback stateCallback = nullptr,
        std::function<void()> scrollCompleteCallback = nullptr
    );
    virtual ~Display();

    virtual void setBrightness(int brightness) = 0;
    void setScrolling(Scrolling direction);
    void setAlignment(Alignment alignment);
    Alignment getAlignment() const;
    void forceUpdate();

    void show(
        std::optional<std::string> text,
        std::optional<std::string> timeFormat,
        transition::Type transition_type = transition::Type::NONE,
        double duration = 1.0
    );

    void start();
    void stop();
    
    // Transition support
    void setTransition(transition::Type type, double duration = 0.0);
    bool isTransitioning() const;

protected:
    std::array<uint8_t, X_MAX> displayBuffer{0};
    size_t renderedTextSize = 0;
    int scrollOffset = 0;
    Scrolling scrollDirection = Scrolling::ENABLED;
    int currentBrightness = DEFAULT_BRIGHTNESS;
    
    bool prepare();

private:
    virtual void update() = 0;

    void showText(std::string text);
    std::array<uint8_t, X_MAX> createDisplayBuffer(std::vector<uint8_t> time);
    std::array<uint8_t, X_MAX> createDisplayBufferOptimized(const std::vector<uint8_t>& time);
    std::vector<uint8_t> renderTime();
    const std::vector<uint8_t>& renderTimeOptimized();
    
    // Helper methods for cleaner buffer creation
    size_t calculateCenterOffset(size_t contentSize, size_t availableSpace) const;
    void renderContentToBuffer(std::array<uint8_t, X_MAX>& buffer, const std::vector<uint8_t>& content, 
                              size_t startPos, size_t maxPos) const;
    size_t addTimeDivider(std::array<uint8_t, X_MAX>& buffer, size_t pos) const;
    
    // Mode-specific buffer creation methods
    std::array<uint8_t, X_MAX> createTimeOnlyBuffer(std::array<uint8_t, X_MAX>& rendered, const std::vector<uint8_t>& time);
    std::array<uint8_t, X_MAX> createTextOnlyBuffer(std::array<uint8_t, X_MAX>& rendered);
    std::array<uint8_t, X_MAX> createTimeAndTextBuffer(std::array<uint8_t, X_MAX>& rendered, const std::vector<uint8_t>& time);
    
    // Consolidated buffer creation method
    std::array<uint8_t, X_MAX> createBufferWithContent(std::array<uint8_t, X_MAX>& rendered, 
                                                    const std::vector<uint8_t>* timeContent,
                                                    const std::vector<uint8_t>* textContent,
                                                    bool addDivider);

    Mode mode = Mode::TIME;
    std::vector<uint8_t> renderedText;
    std::string timeFormat = TIME_FORMAT_LONG;
    double scrollDelayTimer = 0.0;
    bool dirty = true;
    Alignment alignment = Alignment::LEFT;
    
    // Caching for performance optimization
    std::vector<uint8_t> cachedRenderedTime;
    std::time_t lastTimeRendered = 0;
    std::string lastTimeFormat;
    bool timeNeedsUpdate = true;

    std::vector<std::unique_ptr<timer::Timer>> timers;

    std::function<void()> preUpdate;
    std::function<void()> postUpdate;
    
    DisplayStateCallback displayStateCallback;
    std::function<void()> scrollCompleteCallback;
    
    // Transition system
    std::unique_ptr<transition::TransitionManager> transition_manager;
    transition::Type default_transition_type = transition::Type::NONE;
    double default_transition_duration = 1.0;
};

} // namespace display

#endif