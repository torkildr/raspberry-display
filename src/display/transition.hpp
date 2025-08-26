#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <random>
#include <functional>

#define X_MAX 128

namespace transition
{

/**
 * @brief Enumeration of available transition types
 */
enum class Type
{
    NONE,        // No transition - instant switch
    WIPE_LEFT,   // Wipe from left to right
    WIPE_RIGHT,  // Wipe from right to left  
    DISSOLVE,    // Random pixel dissolve effect
    SCROLL_UP,   // Scroll entire screen up
    SCROLL_DOWN, // Scroll entire screen down
    SPLIT_CENTER,// Split from center outward
    SPLIT_SIDES, // Split from sides inward
    RANDOM       // Randomly select from available transitions
};

/**
 * @brief Base class for all display transitions
 * 
 * Transitions animate from a previous display state to a new display state
 * over a specified duration using frame-by-frame animation.
 */
class TransitionBase
{
public:
    TransitionBase(double duration_seconds = 1.0);
    virtual ~TransitionBase() = default;

    /**
     * @brief Start the transition with source and target states
     * @param from Previous display buffer state
     * @param to Target display buffer state  
     */
    void start(const std::array<uint8_t, X_MAX>& from, const std::array<uint8_t, X_MAX>& to);
    
    /**
     * @brief Update the transition by one frame
     * @param delta_time Time elapsed since last update (seconds)
     * @return Current transition buffer state
     */
    std::array<uint8_t, X_MAX> update(double delta_time);
    
    /**
     * @brief Check if transition is complete
     */
    bool isComplete() const { return elapsed_time >= duration; }
    
    /**
     * @brief Get the final target state
     */
    const std::array<uint8_t, X_MAX>& getTargetState() const { return target_buffer; }
    
    /**
     * @brief Reset transition to initial state
     */
    virtual void reset();

protected:
    /**
     * @brief Pure virtual method for transition-specific animation logic
     * @param progress Normalized progress from 0.0 to 1.0
     * @return Current frame buffer
     */
    virtual std::array<uint8_t, X_MAX> animate(double progress) = 0;

    std::array<uint8_t, X_MAX> source_buffer{0};
    std::array<uint8_t, X_MAX> target_buffer{0};
    double duration;
    double elapsed_time;
};

/**
 * @brief Wipe transition - reveals new content with a moving pattern
 */
class WipeTransition : public TransitionBase
{
public:
    enum class Direction { LEFT_TO_RIGHT, RIGHT_TO_LEFT };
    WipeTransition(Direction dir = Direction::LEFT_TO_RIGHT, double duration = 1.0);

protected:
    std::array<uint8_t, X_MAX> animate(double progress) override;

private:
    Direction direction;
    void set_wipe_pattern(std::array<uint8_t, X_MAX>& result, uint8_t pattern, size_t pos, short offset = 0);
};

/**
 * @brief Dissolve transition - random pixel-by-pixel reveal
 */
class DissolveTransition : public TransitionBase
{
public:
    DissolveTransition(double duration = 1.5, uint32_t seed = 0);

protected:
    std::array<uint8_t, X_MAX> animate(double progress) override;
    void reset() override;

private:
    std::mt19937 rng;
    std::array<double, X_MAX * 8> pixel_thresholds; // 8 bits per byte
    void generatePixelOrder();
};

/**
 * @brief Scroll transition - entire screen scrolls to reveal new content
 */
class ScrollTransition : public TransitionBase
{
public:
    enum class Direction { UP, DOWN };
    
    ScrollTransition(Direction dir = Direction::UP, double duration = 1.0);

protected:
    std::array<uint8_t, X_MAX> animate(double progress) override;

private:
    Direction direction;
    static constexpr int DISPLAY_HEIGHT = 8; // 8 rows of pixels
};

/**
 * @brief Split transition - reveals from center outward or sides inward
 */
class SplitTransition : public TransitionBase
{
public:
    enum class Direction { CENTER_OUT, SIDES_IN };
    
    SplitTransition(Direction dir = Direction::CENTER_OUT, double duration = 1.0);

protected:
    std::array<uint8_t, X_MAX> animate(double progress) override;

private:
    Direction direction;
};

/**
 * @brief Factory class for creating transitions
 */
class TransitionFactory
{
public:
    /**
     * @brief Create a transition by type
     * @param type The transition type to create
     * @param duration Duration in seconds (default varies by type)
     * @return Unique pointer to the transition
     */
    static std::unique_ptr<TransitionBase> create(Type type, double duration = 0.0);
    
    /**
     * @brief Parse transition type from string
     * @param type_str String representation of transition type
     * @return Corresponding Type enum value
     */
    static Type parseType(const std::string& type_str);

};

/**
 * @brief Manager class for handling display transitions
 * 
 * Integrates with display system to provide smooth animated transitions
 * between display states.
 */
class TransitionManager
{
public:
    TransitionManager(std::function<void(const std::array<uint8_t, X_MAX>&)> display_callback);
    
    /**
     * @brief Start a transition to a new display state
     * @param to_buffer Target display buffer
     * @param type Transition type to use
     * @param duration Duration in seconds (0 = use default)
     */
    void startTransition(const std::array<uint8_t, X_MAX>& to_buffer, 
                        Type type, 
                        double duration = 0.0);
    
    /**
     * @brief Update the current transition (call from display loop)
     * @param delta_time Time elapsed since last update
     * @return True if transition is active, false if complete
     */
    bool update(double delta_time);
    
    /**
     * @brief Check if a transition is currently active
     */
    bool isTransitioning() const { return current_transition != nullptr; }
    
    /**
     * @brief Set the current display buffer (for transition source)
     */
    void setCurrentBuffer(const std::array<uint8_t, X_MAX>& buffer);

private:
    std::unique_ptr<TransitionBase> current_transition;
    std::array<uint8_t, X_MAX> current_buffer{0};
    std::function<void(const std::array<uint8_t, X_MAX>&)> display_callback;
};

} // namespace transition
