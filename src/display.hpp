#ifndef display_hpp
#define display_hpp

#include <array>
#include <string>
#include <vector>
#include <functional>

#include "timer.hpp"

#define X_MAX 128

#define SCROLL_DELAY 10
#define REFRESH_RATE 0.1

#define TIME_FORMAT_LONG "%A, %b %d %H:%M"
#define TIME_FORMAT_SHORT "%H:%M|"

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

class Display
{
public:
    Display(std::function<void()> preUpdate, std::function<void()> postUpdate);
    virtual ~Display();

    virtual void setBrightness(int brightness) = 0;
    void setScrolling(Scrolling direction);

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
    std::vector<char> renderTime();

    Mode mode = Mode::TIME;
    std::vector<char> renderedText;
    std::string timeFormat = TIME_FORMAT_LONG;
    int scrollDelay = 0;
    bool dirty = true;

    std::vector<timer::Timer*> timers;

    std::function<void()> preUpdate;
    std::function<void()> postUpdate;
};

} // namespace display

#endif