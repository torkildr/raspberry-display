#include <ctime>
#include <iomanip>
#include <sstream>

#include "display.hpp"
#include "timer.hpp"

#include "font.hpp"

namespace display
{

Display::Display(std::function<void()> preUpdate, std::function<void()> postUpdate)
    : preUpdate(preUpdate), postUpdate(postUpdate)
{
}

Display::~Display()
{
    stop();
}

void Display::prepare()
{
    int textSize = renderedText.size();
    auto time = renderTime();
    int textBlock = X_MAX - time.size();

    if (scrollDelay >= SCROLL_DELAY)
    {
        if (scrollOffset != 0)
        {
            scrollDelay = 1;
            scrollOffset = 0;
        }
        else
        {
            scrollDelay = 0;
        }
    }
    else if (scrollDelay)
    {
        scrollDelay++;
    }
    else
    {
        if (scrollDirection == Scrolling::ENABLED && textSize > textBlock)
        {
            ++scrollOffset;
            if (textBlock + scrollOffset >= textSize)
            {
                scrollDelay = 1;
            }
        }
    }

    auto tmp = createDisplayBuffer(std::move(time));
    if (tmp == displayBuffer)
    {
        dirty = false;
    }
    else
    {
        std::copy(tmp.begin(), tmp.end(), displayBuffer.begin());
        dirty = true;
    }
};

void Display::start()
{
    timers.push_back(timer::createTimer([=] {
        if (dirty)
        {
            preUpdate();
            update();
            postUpdate();
        }
        prepare();
    }, REFRESH_RATE));
}

void Display::stop()
{
    for (auto &timer : timers)
    {
        timer->stop();
        delete timer;
    }
    timers.clear();
}

std::string getTime(std::string format)
{
    std::stringstream displayText;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    displayText << std::put_time(&tm, format.c_str());

    return displayText.str();
}

std::vector<char> Display::renderTime()
{
    if (mode == Mode::TIME || mode == Mode::TIME_AND_TEXT)
    {
        return font::renderString(getTime(timeFormat));
    }
    return std::vector<char>();
}

std::array<char, X_MAX> Display::createDisplayBuffer(std::vector<char> time)
{
    std::array<char, X_MAX> rendered = {0};
    size_t pos = 0;

    for (; pos < time.size(); pos++)
    {
        rendered[pos] = time.at(pos);
    }

    if (mode == Mode::TEXT || mode == Mode::TIME_AND_TEXT)
    {
        for (size_t i = scrollOffset; pos < X_MAX && i < renderedText.size(); ++i, ++pos)
        {
            rendered[pos] = renderedText.at(i);
        }
    }

    return rendered;
}

void Display::setScrolling(Scrolling direction)
{
    scrollOffset = 0;
    scrollDelay = 1;

    if (direction == Scrolling::RESET)
    {
        return;
    }

    scrollDirection = direction;
}

void Display::showText(std::string text)
{
    renderedText = font::renderString(text);
    renderedTextSize = this->renderedText.size();

    setScrolling(Scrolling::RESET);
}

void Display::show(std::string text)
{
    mode = Mode::TEXT;

    showText(text);
}

void Display::showTime(std::string timeFormat)
{
    mode = Mode::TIME;

    if (!timeFormat.empty())
    {
        this->timeFormat = timeFormat;
    }
    else
    {
        this->timeFormat = TIME_FORMAT_LONG;
    }
}

void Display::showTime(std::string timeFormat, std::string text)
{
    mode = Mode::TIME_AND_TEXT;

    if (!timeFormat.empty())
    {
        this->timeFormat = timeFormat;
    }
    else
    {
        this->timeFormat = TIME_FORMAT_SHORT;
    }

    showText(text);
}

} // namespace display
