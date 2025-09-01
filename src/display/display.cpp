#include <chrono>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>

#include "display.hpp"
#include "timer.hpp"
#include "transition.hpp"
#include "log_util.hpp"

#include "font.hpp"
#include "pong.hpp"
namespace display
{

using namespace std::chrono_literals;

static constexpr bool show_time_divider = true;

Display::Display(
    std::function<void()> preUpdate,
    std::function<void()> postUpdate,
    DisplayStateCallback stateCallback,
    std::function<void()> scrollCompleteCallback
)
    : preUpdate(preUpdate),
      postUpdate(postUpdate),
      displayStateCallback(stateCallback),
      scrollCompleteCallback(scrollCompleteCallback)
{
    // Initialize transition manager with display buffer update callback
    transition_manager = std::make_unique<transition::TransitionManager>(
        [this](const std::array<uint8_t, X_MAX>& buffer) {
            this->displayBuffer = buffer;
        }
    );
}

Display::~Display()
{
    stop();
}

bool Display::prepare()
{
    // Let sequence processing continue normally even during pong
    // This preserves sequence state and allows new sequence elements
    
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

    // Hack to fix floating point comparison
    if (std::round(scrollDelayTimer*100) >= std::round(SCROLL_DELAY*100) && !transition_manager->isTransitioning())
    {
        if (scrollOffset != 0)
        {
            scrollCompleteCallback();
            // Reset to beginning and start delay before next scroll cycle
            scrollDelayTimer = 1.0 / REFRESH_RATE;  // Start timing from next cycle
            scrollOffset = 0;
            scrollChanged = true;
        }
        else
        {
            // Delay finished, start scrolling
            scrollDelayTimer = -1;
        }
    }
    else if (scrollDelayTimer >= 0.0)
    {
        // Accumulate time for delay before starting/restarting scroll
        scrollDelayTimer += 1.0 / REFRESH_RATE;
    }
    
    if (scrollDelayTimer == -1)
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
                scrollDelayTimer = 0;  // Start delay timer before restarting
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
    bool hasChanges = (dirty || scrollChanged || timeChanged);
    
    if (hasChanges)
    {
        auto newBuffer = createDisplayBufferOptimized(time);
        
        // If we have a default transition and buffer changed, use it
        if (default_transition_type != transition::Type::NONE && 
            newBuffer != displayBuffer && 
            !transition_manager->isTransitioning()) {
            
            transition_manager->setCurrentBuffer(displayBuffer);
            transition_manager->startTransition(newBuffer, default_transition_type, default_transition_duration);
        } else {
            // Normal update path - set display buffer and keep transition manager in sync
            displayBuffer = newBuffer;
            transition_manager->setCurrentBuffer(displayBuffer);
        }
        dirty = false;
    }
    
    // Update any active transitions
    if (transition_manager->isTransitioning()) {
        // Use a fixed delta time based on refresh rate for consistent animation
        double delta_time = 1.0 / REFRESH_RATE;
        transition_manager->update(delta_time);
    }
    
    // Handle pong overlay independently of sequence processing
    if (isPongActive()) {
        // Check if pong game should auto-exit after game over
        if (pong_game->shouldExit()) {
            stopPongGame();
            dirty = true;
            return true; // Update to clear pong display
        }
        
        // Overlay pong on top of current display buffer (preserving sequence state)
        pong_game->renderToBuffer(displayBuffer);
        return true; // Always update when pong is active
    }
    
    return hasChanges;
}

void Display::start()
{
    auto frame_time = std::chrono::duration<double, std::milli>(1000.0 / REFRESH_RATE);
    auto timer = std::make_unique<timer::Timer>();
    timer->setInterval([this] {
        // Check if prepare() detected any changes (including time updates)
        bool hasChanges = prepare(); // Handle transitions and buffer updates
        
        // Update display if changes detected OR if transition is active
        if (hasChanges || transition_manager->isTransitioning())
        {
            preUpdate();
            update();
            postUpdate();
        }
    }, std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time));
    
    timers.push_back(std::move(timer));
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

std::vector<uint8_t> Display::renderTime()
{
    if (mode == Mode::TIME || mode == Mode::TIME_AND_TEXT)
    {
        return font::renderString(getTime(timeFormat));
    }
    return std::vector<uint8_t>();
}

const std::vector<uint8_t>& Display::renderTimeOptimized()
{
    if (mode != Mode::TIME && mode != Mode::TIME_AND_TEXT)
    {
        static std::vector<uint8_t> empty;
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

std::array<uint8_t, X_MAX> Display::createDisplayBuffer(std::vector<uint8_t> time)
{
    std::array<uint8_t, X_MAX> rendered = {0};
    
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

std::array<uint8_t, X_MAX> Display::createDisplayBufferOptimized(const std::vector<uint8_t>& time)
{
    std::array<uint8_t, X_MAX> rendered = {0};
    
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

std::array<uint8_t, X_MAX> Display::createTimeOnlyBuffer(std::array<uint8_t, X_MAX>& rendered, const std::vector<uint8_t>& time)
{
    return createBufferWithContent(rendered, &time, nullptr, false);
}

std::array<uint8_t, X_MAX> Display::createTextOnlyBuffer(std::array<uint8_t, X_MAX>& rendered)
{
    return createBufferWithContent(rendered, nullptr, &renderedText, false);
}

std::array<uint8_t, X_MAX> Display::createTimeAndTextBuffer(std::array<uint8_t, X_MAX>& rendered, const std::vector<uint8_t>& time)
{
    return createBufferWithContent(rendered, &time, &renderedText, true);
}

std::array<uint8_t, X_MAX> Display::createBufferWithContent(std::array<uint8_t, X_MAX>& rendered, 
                                                        const std::vector<uint8_t>* timeContent,
                                                        const std::vector<uint8_t>* textContent,
                                                        bool addDivider)
{
    size_t pos = 0;
    
    // Render time content if provided
    if (timeContent && !timeContent->empty()) {
        if (alignment == Alignment::CENTER && static_cast<int>(timeContent->size()) <= X_MAX && !textContent) {
            // Center time-only content
            size_t centerOffset = calculateCenterOffset(timeContent->size(), X_MAX);
            for (size_t i = 0; i < timeContent->size() && centerOffset + i < X_MAX; i++) {
                rendered[centerOffset + i] = timeContent->at(i);
            }
            return rendered;
        } else {
            // Left-align time content or part of time+text
            for (; pos < timeContent->size() && pos < X_MAX; pos++) {
                rendered[pos] = timeContent->at(pos);
            }
            
            // Add divider between time and text if requested
            if (addDivider && textContent) {
                pos = addTimeDivider(rendered, pos);
            }
        }
    }
    
    // Render text content if provided
    if (textContent && !textContent->empty()) {
        // Handle centering for text content
        if (alignment == Alignment::CENTER) {
            size_t availableSpace = X_MAX - pos;
            if (textContent->size() < availableSpace) {
                pos += calculateCenterOffset(textContent->size(), availableSpace);
            }
        }
        
        // Render the text content
        renderContentToBuffer(rendered, *textContent, pos, X_MAX);
    }
    
    return rendered;
}

void Display::setScrolling(Scrolling direction)
{
    scrollOffset = 0;
    scrollDelayTimer = 0;

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

void Display::renderContentToBuffer(std::array<uint8_t, X_MAX>& buffer, const std::vector<uint8_t>& content, 
                                   size_t startPos, size_t maxPos) const
{
    size_t pos = startPos;
    for (size_t i = static_cast<size_t>(scrollOffset); pos < maxPos && i < content.size(); ++i, ++pos)
    {
        buffer[pos] = content.at(i);
    }
}

size_t Display::addTimeDivider(std::array<uint8_t, X_MAX>& buffer, size_t pos) const
{
    if (show_time_divider && pos < X_MAX) {
        // Add vertical divider line (all bits set for full height)
        buffer[pos++] = 0xFF;  // Vertical divider
        
        // Add 1 pixel gap before text when not actively scrolling
        if (pos < X_MAX && scrollOffset == 0) {
            buffer[pos++] = 0;  // 1 pixel gap before text starts
        }
    }
    return pos;
}

void Display::showText(std::string text)
{
    renderedText = font::FontCache::renderStringOptimized(text);
    renderedTextSize = this->renderedText.size();

    setScrolling(Scrolling::RESET);
    dirty = true;
}

void Display::show(
    std::optional<std::string> text,
    std::optional<std::string> timeFormat,
    transition::Type transition_type,
    double duration)
{
    // Ignore if pong is active
    if (pong_mode) {
        return;
    }
    
    if (timeFormat.has_value()) {
        timeNeedsUpdate = true;
        
        if (text.has_value()) {
            mode = Mode::TIME_AND_TEXT;
            this->timeFormat = timeFormat.value().empty() ? TIME_FORMAT_SHORT : timeFormat.value();
            showText(text.value());
        } else {
            mode = Mode::TIME;
            this->timeFormat = timeFormat.value().empty() ? TIME_FORMAT_LONG : timeFormat.value();
            renderedText.clear();
            renderedTextSize = 0;
        }
    } else {
        mode = Mode::TEXT;
        showText(text.has_value() ? text.value() : "");
    }

    if (transition_type != transition::Type::NONE) {
        auto newBuffer = createDisplayBufferOptimized(renderTimeOptimized());
        transition_manager->setCurrentBuffer(displayBuffer);
        transition_manager->startTransition(newBuffer, transition_type, duration);
    }
    
    // Invoke display state callback if set
    if (displayStateCallback) {
        std::string textValue = text.value_or("");
        std::string timeFormatValue = timeFormat.value_or("");
        displayStateCallback(textValue, timeFormatValue, currentBrightness);
    }
}

// Transition support methods
void Display::setTransition(transition::Type type, double duration)
{
    default_transition_type = type;
    if (duration > 0.0) {
        default_transition_duration = duration;
    }
}

bool Display::isTransitioning() const
{
    return transition_manager->isTransitioning();
}

// Pong game support methods
void Display::startPongGame()
{
    if (!pong_game) {
        pong_game = std::make_unique<pong::PongGame>();
    }
    pong_game->start();
    pong_mode = true;
    dirty = true;
    DEBUG_LOG("Pong game started");
}

void Display::stopPongGame()
{
    if (pong_game) {
        pong_game->stop();
    }
    pong_mode = false;
    dirty = true;
    DEBUG_LOG("Pong game stopped");
    
    // Notify sequence system that pong stopped so it can refresh display
    if (pong_stop_callback) {
        pong_stop_callback();
    }
}

bool Display::isPongActive() const
{
    return pong_mode && pong_game && pong_game->isRunning();
}

void Display::togglePongGame()
{
    if (isPongActive()) {
        stopPongGame();
    } else {
        startPongGame();
    }
}

void Display::setPongStopCallback(std::function<void()> callback)
{
    pong_stop_callback = std::move(callback);
}

void Display::setPongPlayerControl(int control)
{
    if (pong_game) {
        pong::PaddleControl paddleControl = pong::PaddleControl::NONE;
        if (control == -1) {
            paddleControl = pong::PaddleControl::UP;
        } else if (control == 1) {
            paddleControl = pong::PaddleControl::DOWN;
        }
        pong_game->setPlayerControl(paddleControl);
    }
}

} // namespace display
