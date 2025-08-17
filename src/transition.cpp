#include "transition.hpp"
#include <algorithm>
#include <cstring>
#include <chrono>
#include <random>
#include <array>

namespace transition
{

// =============================================================================
// TransitionBase Implementation
// =============================================================================

TransitionBase::TransitionBase(double duration_seconds)
    : duration(duration_seconds), elapsed_time(0.0)
{
}

void TransitionBase::start(const std::array<uint8_t, X_MAX>& from, const std::array<uint8_t, X_MAX>& to)
{
    source_buffer = from;
    target_buffer = to;
    elapsed_time = 0.0;
}

std::array<uint8_t, X_MAX> TransitionBase::update(double delta_time)
{
    elapsed_time += delta_time;
    double progress = std::min(1.0, elapsed_time / duration);
    
    if (progress >= 1.0) {
        return target_buffer;
    }
    
    return animate(progress);
}

void TransitionBase::reset()
{
    elapsed_time = 0.0;
}

// =============================================================================
// WipeTransition Implementation  
// =============================================================================

WipeTransition::WipeTransition(Direction dir, double duration)
    : TransitionBase(duration), direction(dir)
{
}

std::array<uint8_t, X_MAX> WipeTransition::animate(double progress)
{
    std::array<uint8_t, X_MAX> result = source_buffer;

    auto wipe_pos = static_cast<size_t>(round(progress * static_cast<double>(X_MAX)));
    
    for (size_t x = 0; x < X_MAX; ++x) {
        size_t pos = (direction == Direction::LEFT_TO_RIGHT) ? x : (X_MAX - 1 - x);
        bool should_reveal = wipe_pos > x;
        
        if (x == wipe_pos) {
            if (pos - 1 > 0) {
                result[pos - 1] ^= 0xFF;
            }
            result[pos] ^= 0xFF;
       } else if (should_reveal) {
            result[pos] = target_buffer[pos];
       }
    }
    
    return result;
}

// =============================================================================
// DissolveTransition Implementation
// =============================================================================

DissolveTransition::DissolveTransition(double duration, uint32_t seed)
    : TransitionBase(duration)
{
    if (seed == 0) {
        seed = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
    rng.seed(seed);
    generatePixelOrder();
}

void DissolveTransition::generatePixelOrder()
{
    // Generate random thresholds for each pixel bit
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto& threshold : pixel_thresholds) {
        threshold = dist(rng);
    }
}

std::array<uint8_t, X_MAX> DissolveTransition::animate(double progress)
{
    std::array<uint8_t, X_MAX> result = source_buffer;
    
    for (size_t x = 0; x < X_MAX; ++x) {
        uint8_t byte_result = 0;
        uint8_t sparkle_mask = 0;
        
        for (size_t bit = 0; bit < 8; ++bit) {
            size_t pixel_index = x * 8 + bit;
            double threshold = pixel_thresholds[pixel_index];
            
            auto bit_mask = static_cast<uint8_t>(1U << static_cast<unsigned>(bit));
            
            if (progress >= threshold) {
                // Reveal target pixel bit
                byte_result = static_cast<uint8_t>(byte_result | (target_buffer[x] & bit_mask));
            } else {
                // Keep source pixel bit
                byte_result = static_cast<uint8_t>(byte_result | (source_buffer[x] & bit_mask));
                
                // Create sparkle effect for identical content
                if ((source_buffer[x] & bit_mask) == (target_buffer[x] & bit_mask)) {
                    // Create a sparkle zone around the transition threshold
                    double sparkle_zone = 0.1; // 10% of transition time
                    double distance_to_threshold = std::abs(progress - threshold);
                    
                    if (distance_to_threshold < sparkle_zone) {
                        // Use a sine wave pattern for sparkle intensity
                        double sparkle_intensity = std::sin(distance_to_threshold * 3.14159 / sparkle_zone);
                        // Randomly decide if this pixel sparkles based on intensity and threshold
                        if (sparkle_intensity > 0.5 && 
                            (static_cast<uint32_t>(pixel_index * 31) % 100) < static_cast<uint32_t>(sparkle_intensity * 100)) {
                            sparkle_mask |= bit_mask;
                        }
                    }
                }
            }
        }
        
        // Apply sparkle effect (invert sparkle pixels)
        result[x] = static_cast<uint8_t>(byte_result ^ sparkle_mask);
    }
    
    return result;
}

void DissolveTransition::reset()
{
    TransitionBase::reset();
    generatePixelOrder();
}

// =============================================================================
// ScrollTransition Implementation
// =============================================================================

ScrollTransition::ScrollTransition(Direction dir, double duration)
    : TransitionBase(duration), direction(dir)
{
}

std::array<uint8_t, X_MAX> ScrollTransition::animate(double progress)
{
    std::array<uint8_t, X_MAX> result{0};
    
    int pixel_shift = static_cast<int>(
        round(progress * static_cast<double>(DISPLAY_HEIGHT))
    );

    for (size_t x = 0; x < X_MAX; ++x) {
        uint8_t source_byte = source_buffer[x];
        uint8_t target_byte = target_buffer[x];

        if (direction == Direction::UP) {
            result[x] = ((source_byte >> pixel_shift) & 0xFF) | 
                        ((target_byte << (DISPLAY_HEIGHT - pixel_shift)) & 0xFF);
        } else {
            result[x] = ((source_byte << pixel_shift) & 0xFF) | 
                        ((target_byte >> (DISPLAY_HEIGHT - pixel_shift)) & 0xFF);
        }
    }
    
    return result;
}

// =============================================================================
// SplitTransition Implementation  
// =============================================================================

SplitTransition::SplitTransition(Direction dir, double duration)
    : TransitionBase(duration), direction(dir)
{
}

std::array<uint8_t, X_MAX> SplitTransition::animate(double progress)
{
    std::array<uint8_t, X_MAX> result = source_buffer;
    
    auto reveal_width = static_cast<size_t>(progress * static_cast<double>(X_MAX) / 2.0);
    
    for (size_t x = 0; x < X_MAX; ++x) {
        bool should_reveal = false;
        
        if (direction == Direction::CENTER_OUT) {
            // Reveal from center outward
            size_t center = X_MAX / 2;
            should_reveal = (x >= (center > reveal_width ? center - reveal_width : 0)) && 
                           (x < (center + reveal_width));
        } else {
            // Reveal from sides inward
            should_reveal = (x < reveal_width) || (x >= (X_MAX - reveal_width));
        }
        
        if (should_reveal) {
            result[x] = target_buffer[x];
        }
    }
    
    return result;
}

// =============================================================================
// TransitionFactory Implementation
// =============================================================================

std::unique_ptr<TransitionBase> TransitionFactory::create(Type type, double duration)
{
    // Handle RANDOM by selecting a random transition type
    if (type == Type::RANDOM) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // Available transitions (excluding NONE and RANDOM itself)
        static const std::array<Type, 7> available_types = {
            Type::WIPE_LEFT, Type::WIPE_RIGHT, Type::DISSOLVE,
            Type::SCROLL_UP, Type::SCROLL_DOWN, 
            Type::SPLIT_CENTER, Type::SPLIT_SIDES
        };
        
        std::uniform_int_distribution<size_t> dist(0, available_types.size() - 1);
        type = available_types[dist(gen)];
    }
    
    // Set default durations if not specified
    if (duration <= 0.0) {
        switch (type) {
            case Type::NONE: duration = 0.0; break;
            case Type::WIPE_LEFT:
            case Type::WIPE_RIGHT: duration = 1.0; break;
            case Type::DISSOLVE: duration = 1.5; break;
            case Type::SCROLL_UP:
            case Type::SCROLL_DOWN: duration = 0.8; break;
            case Type::SPLIT_CENTER:
            case Type::SPLIT_SIDES: duration = 1.0; break;
            case Type::RANDOM: duration = 1.0; break; // Default for random (but should be overridden above)
        }
    }
    
    switch (type) {
        case Type::NONE:
        case Type::RANDOM: // Should not reach here due to random selection above
            return nullptr;
            
        case Type::WIPE_LEFT:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::LEFT_TO_RIGHT, duration);
            
        case Type::WIPE_RIGHT:
            return std::make_unique<WipeTransition>(WipeTransition::Direction::RIGHT_TO_LEFT, duration);
            
        case Type::DISSOLVE:
            return std::make_unique<DissolveTransition>(duration);
            
        case Type::SCROLL_UP:
            return std::make_unique<ScrollTransition>(ScrollTransition::Direction::UP, duration);
            
        case Type::SCROLL_DOWN:
            return std::make_unique<ScrollTransition>(ScrollTransition::Direction::DOWN, duration);
            
        case Type::SPLIT_CENTER:
            return std::make_unique<SplitTransition>(SplitTransition::Direction::CENTER_OUT, duration);
            
        case Type::SPLIT_SIDES:
            return std::make_unique<SplitTransition>(SplitTransition::Direction::SIDES_IN, duration);
    }
    
    return nullptr;
}

Type TransitionFactory::parseType(const std::string& type_str)
{
    std::string lower_str = type_str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    
    if (lower_str == "none") return Type::NONE;
    if (lower_str == "wipe_left" || lower_str == "wipe-left") return Type::WIPE_LEFT;
    if (lower_str == "wipe_right" || lower_str == "wipe-right") return Type::WIPE_RIGHT;
    if (lower_str == "dissolve") return Type::DISSOLVE;
    if (lower_str == "scroll_up" || lower_str == "scroll-up") return Type::SCROLL_UP;
    if (lower_str == "scroll_down" || lower_str == "scroll-down") return Type::SCROLL_DOWN;
    if (lower_str == "split_center" || lower_str == "split-center") return Type::SPLIT_CENTER;
    if (lower_str == "split_sides" || lower_str == "split-sides") return Type::SPLIT_SIDES;
    if (lower_str == "random") return Type::RANDOM;
    
    return Type::NONE;
}

// =============================================================================
// TransitionManager Implementation
// =============================================================================

TransitionManager::TransitionManager(std::function<void(const std::array<uint8_t, X_MAX>&)> display_callback)
    : display_callback(display_callback)
{
}

void TransitionManager::startTransition(const std::array<uint8_t, X_MAX>& to_buffer, 
                                       Type type, 
                                       double duration)
{
    if (type == Type::NONE) {
        // No transition - instant switch
        current_buffer = to_buffer;
        display_callback(current_buffer);
        current_transition.reset();
        return;
    }
    
    current_transition = TransitionFactory::create(type, duration);
    if (current_transition) {
        current_transition->start(current_buffer, to_buffer);
    } else {
        // Fallback to instant switch
        current_buffer = to_buffer;
        display_callback(current_buffer);
    }
}

bool TransitionManager::update(double delta_time)
{
    if (!current_transition) {
        return false;
    }
    
    if (current_transition->isComplete()) {
        current_buffer = current_transition->getTargetState();
        display_callback(current_buffer);
        current_transition.reset();
        return false;
    }
    
    auto transition_buffer = current_transition->update(delta_time);
    display_callback(transition_buffer);
    
    return true;
}

void TransitionManager::setCurrentBuffer(const std::array<uint8_t, X_MAX>& buffer)
{
    current_buffer = buffer;
}

} // namespace transition
