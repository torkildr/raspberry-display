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
    
    // Check if time needs update BEFORE calling renderTimeOptimized (which resets the flag)
    bool timeChanged = timeNeedsUpdate;
    if (!timeChanged && (mode == Mode::TIME || mode == Mode::TIME_AND_TEXT)) {
        auto currentTime = std::time(nullptr);
        timeChanged = (currentTime != lastTimeRendered || timeFormat != lastTimeFormat);
    }
    
    const auto& time = renderTimeOptimized();  // Use cached version
    auto textBlock = static_cast<int>(X_MAX - time.size());

    bool scrollChanged = false;
    
    // Determine if scrolling should actually be active
    bool shouldScroll = false;
    
    if (mode == Mode::TIME) {
        // TIME mode scrolls only when content is longer than display width AND scrolling is enabled
        shouldScroll = (scrollDirection == Scrolling::ENABLED) && 
                      (static_cast<int>(time.size()) > X_MAX);
    } else if (mode == Mode::TEXT || mode == Mode::TIME_AND_TEXT) {
        int availableSpace = (mode == Mode::TEXT) ? X_MAX : textBlock;
        // Only scroll when content is longer than available space AND scrolling is enabled
        shouldScroll = (scrollDirection == Scrolling::ENABLED) && 
                      (textSize > availableSpace);
    }
    
    if (scrollDelay >= SCROLL_DELAY)
    {
        if (scrollOffset != 0)
        {
            // Reset to beginning and start delay before next scroll cycle
            scrollDelay = 1;
            scrollOffset = 0;
            scrollChanged = true;
        }
        else
        {
            // Delay finished, start scrolling
            scrollDelay = 0;
        }
    }
    else if (scrollDelay)
    {
        // Counting delay before starting/restarting scroll
        scrollDelay++;
    }
    else
    {
        if (shouldScroll)
        {
            ++scrollOffset;
            scrollChanged = true;
            
            // Check if we've reached the end (content fully visible on right edge)
            bool reachedEnd = false;
            if (mode == Mode::TIME) {
                // Stop when end of time content is at right edge of display
                reachedEnd = (scrollOffset + X_MAX >= static_cast<int>(time.size()));
            } else if (mode == Mode::TEXT) {
                // Stop when end of text content is at right edge of display
                reachedEnd = (scrollOffset + X_MAX >= textSize);
            } else if (mode == Mode::TIME_AND_TEXT) {
                // Stop when end of text content is at right edge of available text area
                reachedEnd = (scrollOffset + textBlock >= textSize);
            }
            
            if (reachedEnd) {
                scrollDelay = 1;  // Start delay before restarting
            }
        }
        else if (scrollOffset != 0)
        {
            // Reset scroll when scrolling is disabled
            scrollOffset = 0;
            scrollChanged = true;
        }
    }

    // Only recreate buffer if something actually changed
    if (dirty || scrollChanged || timeChanged)
    {
        auto tmp = createDisplayBufferOptimized(time);  // Use optimized version with const reference
        
        if (tmp != displayBuffer)
        {
            std::copy(tmp.begin(), tmp.end(), displayBuffer.begin());
            dirty = true;
        }
        else
        {
            dirty = false;
        }
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

const std::vector<char>& Display::renderTimeOptimized()
{
    if (mode != Mode::TIME && mode != Mode::TIME_AND_TEXT)
    {
        static std::vector<char> empty;
        return empty;
    }
    
    auto currentTime = std::time(nullptr);
    
    // Check if we need to update the cached time
    if (timeNeedsUpdate || 
        currentTime != lastTimeRendered || 
        timeFormat != lastTimeFormat)
    {
        cachedRenderedTime = font::FontCache::renderStringOptimized(getTime(timeFormat));
        lastTimeRendered = currentTime;
        lastTimeFormat = timeFormat;
        timeNeedsUpdate = false;
    }
    
    return cachedRenderedTime;
}

std::array<char, X_MAX> Display::createDisplayBuffer(std::vector<char> time)
{
    std::array<char, X_MAX> rendered = {0};
    
    // Handle different display modes with unified logic
    switch (mode) {
        case Mode::TIME:
            return createTimeOnlyBuffer(rendered, time);
            
        case Mode::TEXT:
            return createTextOnlyBuffer(rendered);
            
        case Mode::TIME_AND_TEXT:
            return createTimeAndTextBuffer(rendered, time);
    }
    
    return rendered;
}

std::array<char, X_MAX> Display::createDisplayBufferOptimized(const std::vector<char>& time)
{
    std::array<char, X_MAX> rendered = {0};
    
    // Handle different display modes with unified logic (using const reference)
    switch (mode) {
        case Mode::TIME:
            return createTimeOnlyBuffer(rendered, time);
            
        case Mode::TEXT:
            return createTextOnlyBuffer(rendered);
            
        case Mode::TIME_AND_TEXT:
            return createTimeAndTextBuffer(rendered, time);
    }
    
    return rendered;
}

std::array<char, X_MAX> Display::createTimeOnlyBuffer(std::array<char, X_MAX>& rendered, const std::vector<char>& time)
{
    if (alignment == Alignment::CENTER && static_cast<int>(time.size()) <= X_MAX) {
        // Center the time display when content fits and is center-aligned
        size_t centerOffset = calculateCenterOffset(time.size(), X_MAX);
        for (size_t i = 0; i < time.size(); i++) {
            rendered[centerOffset + i] = time.at(i);
        }
    } else {
        // Left alignment or scrolling for long content
        renderContentToBuffer(rendered, time, 0, X_MAX);
    }
    return rendered;
}

std::array<char, X_MAX> Display::createTextOnlyBuffer(std::array<char, X_MAX>& rendered)
{
    if (alignment == Alignment::CENTER && renderedText.size() < X_MAX && scrollDirection == Scrolling::DISABLED) {
        // Center the text display
        size_t centerOffset = calculateCenterOffset(renderedText.size(), X_MAX);
        for (size_t i = 0; i < renderedText.size(); i++) {
            rendered[centerOffset + i] = renderedText.at(i);
        }
    } else {
        // Left alignment or fallback for long content/scrolling
        renderContentToBuffer(rendered, renderedText, 0, X_MAX);
    }
    return rendered;
}

std::array<char, X_MAX> Display::createTimeAndTextBuffer(std::array<char, X_MAX>& rendered, const std::vector<char>& time)
{
    size_t pos = 0;
    
    // Render time first
    for (; pos < time.size() && pos < X_MAX; pos++) {
        rendered[pos] = time.at(pos);
    }
    
    // Add divider if needed
    if (time.size() > 0) {
        pos = addTimeDivider(rendered, pos);
    }
    
    // Handle text alignment
    if (alignment == Alignment::CENTER && scrollDirection == Scrolling::DISABLED) {
        size_t availableSpace = X_MAX - pos;
        if (renderedText.size() < availableSpace) {
            pos += calculateCenterOffset(renderedText.size(), availableSpace);
        }
    }
    
    // Render text
    renderContentToBuffer(rendered, renderedText, pos, X_MAX);
    
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
    dirty = true;  // Trigger buffer recreation to apply alignment change
}

Alignment Display::getAlignment() const
{
    return alignment;
}

void Display::forceUpdate()
{
    dirty = true;  // Force buffer recreation on next prepare() call
}

size_t Display::calculateCenterOffset(size_t contentSize, size_t availableSpace) const
{
    if (contentSize >= availableSpace) {
        return 0;  // No centering possible
    }
    return (availableSpace - contentSize) / 2;
}

void Display::renderContentToBuffer(std::array<char, X_MAX>& buffer, const std::vector<char>& content, 
                                   size_t startPos, size_t maxPos) const
{
    size_t pos = startPos;
    for (size_t i = static_cast<size_t>(scrollOffset); pos < maxPos && i < content.size(); ++i, ++pos)
    {
        buffer[pos] = content.at(i);
    }
}

size_t Display::addTimeDivider(std::array<char, X_MAX>& buffer, size_t pos) const
{
    if (show_time_divider && pos < X_MAX) {
        // Add vertical divider line (all bits set for full height)
        buffer[pos] = 0xFF;  // Vertical divider
        pos++;
        
        // Add 1 pixel gap before text when not actively scrolling
        if (pos < X_MAX && scrollOffset == 0) {
            buffer[pos] = 0;  // 1 pixel gap before text starts
            pos++;
        }
    }
    return pos;
}

void Display::showText(std::string text)
{
    renderedText = font::FontCache::renderStringOptimized(text);
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
    renderedText.clear();  // Clear any previous text content

    if (!timeFormat.empty())
    {
        this->timeFormat = timeFormat;
    }
    else
    {
        this->timeFormat = TIME_FORMAT_LONG;
    }
    
    timeNeedsUpdate = true;  // Invalidate time cache
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

    timeNeedsUpdate = true;  // Invalidate time cache
    showText(text);
}

}
