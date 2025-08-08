#ifndef display_hpp
#define display_hpp

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "timer.hpp"

#define X_MAX 128

#define SCROLL_DELAY 10
#define REFRESH_RATE 0.1

#define TIME_FORMAT_LONG "%A, %b %d %H:%M"
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

class Display
{
public:
    Display(std::function<void()> preUpdate, std::function<void()> postUpdate);
    virtual ~Display();

    virtual void setBrightness(int brightness) = 0;
    void setScrolling(Scrolling direction);
    void setAlignment(Alignment alignment);
    Alignment getAlignment() const;

    void show(std::string text);
    void showTime(std::string timeFormat);
    void showTime(std::string timeFormat, std::string text);

    void start();
    void stop();

protected:
    std::array<char, X_MAX> displayBuffer{0};
    size_t renderedTextSize = 0;
    int scrollOffset = 0;
    Scrolling scrollDirection = Scrolling::ENABLED;

private:
    virtual void update() = 0;

    void showText(std::string text);
    void prepare();
    std::array<char, X_MAX> createDisplayBuffer(std::vector<char> time);
    std::array<char, X_MAX> createDisplayBufferOptimized(const std::vector<char>& time);
    std::vector<char> renderTime();
    const std::vector<char>& renderTimeOptimized();
    
    // Helper methods for cleaner buffer creation
    size_t calculateCenterOffset(size_t contentSize, size_t availableSpace) const;
    void renderContentToBuffer(std::array<char, X_MAX>& buffer, const std::vector<char>& content, 
                              size_t startPos, size_t maxPos) const;
    size_t addTimeDivider(std::array<char, X_MAX>& buffer, size_t pos) const;
    
    // Mode-specific buffer creation methods
    std::array<char, X_MAX> createTimeOnlyBuffer(std::array<char, X_MAX>& rendered, const std::vector<char>& time);
    std::array<char, X_MAX> createTextOnlyBuffer(std::array<char, X_MAX>& rendered);
    std::array<char, X_MAX> createTimeAndTextBuffer(std::array<char, X_MAX>& rendered, const std::vector<char>& time);

    Mode mode = Mode::TIME;
    std::vector<char> renderedText;
    std::string timeFormat = TIME_FORMAT_LONG;
    int scrollDelay = 0;
    bool dirty = true;
    Alignment alignment = Alignment::LEFT;
    
    // Caching for performance optimization
    std::vector<char> cachedRenderedTime;
    std::time_t lastTimeRendered = 0;
    std::string lastTimeFormat;
    bool timeNeedsUpdate = true;

    std::vector<std::unique_ptr<timer::Timer>> timers;

    std::function<void()> preUpdate;
    std::function<void()> postUpdate;
};

}

#endif