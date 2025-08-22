#ifndef sequence_hpp
#define sequence_hpp

#include <bits/chrono.h>
#include <functional>
#include <thread>
#include <condition_variable>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

#include "timer.hpp"
#include "display.hpp"

namespace sequence
{

using namespace std::chrono;

// Structured display state - no JSON parsing in SequenceManager
struct DisplayState {
    // Content
    std::optional<std::string> text;
    std::optional<std::string> time_format;
    
    // Visual properties
    std::optional<display::Alignment> alignment;
    std::optional<display::Scrolling> scrolling;
    std::optional<int> brightness;  // 0-15
    
    // Transition
    transition::Type transition_type = transition::Type::NONE;
    double transition_duration = 1.0;
};

// Sequence state structure
struct SequenceState {
    DisplayState state;             // The structured display state
    double time;                    // Display time in seconds
    double ttl;                     // Time-to-live in seconds
    steady_clock::time_point created_at;  // When this state was created
    std::string sequence_id;        // Optional sequence identifier for replacement
};

class SequenceManager
{
public:
    // Constructor takes ownership of Display instance
    SequenceManager(std::unique_ptr<display::Display> display);
    ~SequenceManager();
    
    // Sequence management methods
    void addSequenceState(const DisplayState& state, double time, double ttl = 0.0, const std::string& sequence_id = "");
    void setSequence(const std::vector<SequenceState>& sequence);
    void clearSequence(bool set_default_content = false);
    void clearSequenceById(const std::string& sequence_id);
    bool isActive() const;
    
    // Sequence information methods
    std::vector<std::string> getActiveSequenceIds() const;
    std::string getCurrentSequenceId() const;
    size_t getCurrentSequenceIndex() const;
    size_t getSequenceCount() const;
    
    // Display control methods (global state management)
    void setBrightness(int brightness);
    void setScrolling(display::Scrolling direction);
    void setAlignment(display::Alignment alignment);
    display::Alignment getAlignment() const;
    void setDefaultTransition(transition::Type type, double duration = 0.0);
    
    // Process a display state directly (for immediate display)
    void processDisplayState(const DisplayState& state);
    void nextState();

    // Display lifecycle methods
    void start();
    void stop();
        
private:
    void processSequence(bool skip_current = false);
    void removeExpiredStates();
    void setDefaultContent();
    
    std::unique_ptr<display::Display> m_display;
    std::vector<SequenceState> m_sequence;
    mutable std::mutex m_mutex;
    std::atomic<size_t> m_current_index{0};
    std::atomic<bool> m_active{false};
    steady_clock::time_point m_state_start_time;
    
    // Default transition settings
    transition::Type m_default_transition_type = transition::Type::NONE;
    double m_default_transition_duration = 0.0;
    
    std::unique_ptr<timer::Timer> m_timer;
    static constexpr seconds TIMER_INTERVAL = duration_cast<seconds>(10ms); // 10ms timer resolution
    
    // Current display state tracking
    int m_current_brightness = DEFAULT_BRIGHTNESS; // Default brightness
};

// Utility function to parse JSON into DisplayState (for clients)
DisplayState parseDisplayStateFromJSON(const nlohmann::json& json);

} // namespace sequence

#endif
