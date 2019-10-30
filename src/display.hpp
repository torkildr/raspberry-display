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

namespace display {

enum Scrolling {
    DISABLED,
    ENABLED,
    RESET,
};

enum Mode {
    TIME,
    TIME_AND_TEXT,
    TEXT,
};

class Display {
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
        std::array<char, X_MAX> displayBuffer { 0 };
        size_t renderedTextSize;

        int scrollOffset = 0;
        Scrolling scrollDirection = Scrolling::DISABLED;

    private:
        virtual void update() = 0;

        void showText(std::string text);
        void prepare();
        std::array<char, X_MAX> createDisplayBuffer(std::vector<char> time);
        std::vector<char> renderTime();

        Mode mode;
        std::vector<char> renderedText;
        std::string timeFormat;
        int scrollDelay = 0;

        std::vector<std::unique_ptr<timer::Timer>> timers;

        std::function<void()> preUpdate;
        std::function<void()> postUpdate;

};

}

#endif
