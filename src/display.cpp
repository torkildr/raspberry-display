#include <ctime>
#include <iomanip>
#include <sstream>

#include "display.hpp"
#include "timer.hpp"

#include "font.hpp"

namespace display
{

static constexpr bool show_time_divider = true;

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
    auto textSize = static_cast<int>(renderedText.size());
    auto time = renderTime();
    auto textBlock = static_cast<int>(X_MAX - time.size());

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
    timers.push_back(timer::createTimer([this] {
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
    }
    timers.clear();
}

static std::string getTime(std::string format)
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

    // Handle center alignment for TIME mode
    if (mode == Mode::TIME && alignment == Alignment::CENTER)
    {
        // Center the time display
        size_t timeSize = time.size();
        if (timeSize < X_MAX)
        {
            size_t centerOffset = (X_MAX - timeSize) / 2;
            for (size_t i = 0; i < timeSize; i++)
            {
                rendered[centerOffset + i] = time.at(i);
            }
        }
        else
        {
            // If time is too long, fall back to left alignment
            for (size_t i = 0; i < X_MAX && i < timeSize; i++)
            {
                rendered[i] = time.at(i);
            }
        }
        return rendered;
    }

    // Handle center alignment for TEXT mode
    if (mode == Mode::TEXT && alignment == Alignment::CENTER)
    {
        // Center the text display
        size_t textSize = renderedText.size();
        if (textSize < X_MAX && scrollDirection == Scrolling::DISABLED)
        {
            size_t centerOffset = (X_MAX - textSize) / 2;
            for (size_t i = 0; i < textSize; i++)
            {
                rendered[centerOffset + i] = renderedText.at(i);
            }
        }
        else
        {
            // If text is too long or scrolling is enabled, fall back to left alignment with scrolling
            for (size_t i = static_cast<size_t>(scrollOffset); pos < X_MAX && i < textSize; ++i, ++pos)
            {
                rendered[pos] = renderedText.at(i);
            }
        }
        return rendered;
    }

    // Default left alignment behavior (existing logic)
    // Render time first
    for (; pos < time.size(); pos++)
    {
        rendered[pos] = time.at(pos);
    }

    if (mode == Mode::TEXT || mode == Mode::TIME_AND_TEXT)
    {
        if (show_time_divider) {
            // Add horizontal divider in TIME_AND_TEXT mode
            if (mode == Mode::TIME_AND_TEXT && time.size() > 0)
            {
                // Add vertical divider line (all bits set for full height)
                if (pos < X_MAX)
                {
                    rendered[pos] = 0xFF;  // Vertical divider
                    pos++;
                }
                
                // Add 1 pixel gap before text when not actively scrolling
                if (pos < X_MAX && scrollOffset == 0)
                {
                    rendered[pos] = 0;  // 1 pixel gap before text starts
                    pos++;
                }
            }
        }
        
        // Handle center alignment for TIME_AND_TEXT mode
        if (mode == Mode::TIME_AND_TEXT && alignment == Alignment::CENTER)
        {
            // For TIME_AND_TEXT mode with center alignment, center the text portion only
            size_t availableSpace = X_MAX - pos;
            size_t textSize = renderedText.size();
            
            if (textSize < availableSpace && scrollDirection == Scrolling::DISABLED)
            {
                size_t centerOffset = (availableSpace - textSize) / 2;
                pos += centerOffset;
            }
        }
        
        // Render text
        for (size_t i = static_cast<size_t>(scrollOffset); pos < X_MAX && i < renderedText.size(); ++i, ++pos)
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

void Display::setAlignment(Alignment alignment)
{
    this->alignment = alignment;
}

Alignment Display::getAlignment() const
{
    return alignment;
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

}
