#include <bits/chrono.h>
#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>

#include "timer.hpp"
#include "display.hpp"
#include "cyclic_list.hpp"

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
    std::string sequence_id;        // Optional sequence identifier for replacement
    steady_clock::time_point created_at;  // When this state was created
    double time;                    // Display time in seconds
    double ttl;                     // Time-to-live in seconds
    DisplayState state;             // The structured display state
};

typedef CyclicList<std::string, SequenceState> SequenceList;

class SequenceManager
{
public:
    // Constructor takes ownership of Display instance
    SequenceManager(std::unique_ptr<display::Display> display);
    ~SequenceManager();
    
    // Sequence management methods
    void addSequenceState(const std::string& sequence_id, const DisplayState& state, double time, double ttl = 0.0);
    void setSequence(const std::vector<SequenceState>& sequence);
    void clearSequence(bool set_default_content = false);
    void clearSequenceById(const std::string& sequence_id);
    bool isActive() const;
    
    // Sequence information methods
    std::vector<std::string> getActiveSequenceIds() const;
    std::string getCurrentSequenceId() const;
    size_t getSequenceCount() const;
    
    // Display control methods (global state management)
    void setBrightness(int brightness);
    void setScrolling(display::Scrolling direction);
    void setAlignment(display::Alignment alignment);
    display::Alignment getAlignment() const;
    void setDefaultTransition(transition::Type type, double duration = 0.0);
    
    // Process a display state directly (for immediate display)
    void processDisplayState(const std::optional<std::string> sequence_id, const DisplayState& state);
    void nextState();
    void onScrollComplete(); // Called when scrolling completes - respects timing

    // Display lifecycle methods
    void start();
    void stop();
    
    // Pong game control methods (independent of sequence system)
    void togglePongGame(); // Toggle pong on/off without affecting sequence
    void setPongPlayerControl(int control); // 0=none, -1=up, 1=down
    
    // Sequence state checking
    bool isSequenceActive() const;
        
private:
    void processSequence(bool skip_current = false);
    bool isStateExpired(const SequenceState& state);
    void setDefaultContent();
    void onPongStop(); // Called when pong stops to refresh display

    void stopSequence();
    void startSequence();
    
    SequenceList m_sequence;
    std::shared_ptr<SequenceList::Element> m_current_element;
    
    std::unique_ptr<display::Display> m_display;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_active{false};
    steady_clock::time_point m_state_start_time;
    std::optional<std::string> m_last_shown_id;
    
    // Default transition settings
    transition::Type m_default_transition_type = transition::Type::NONE;
    double m_default_transition_duration = 0.0;
    
    std::unique_ptr<timer::Timer> m_timer;
    static constexpr milliseconds TIMER_INTERVAL = 10ms;
    
    // Current display state tracking
    int m_current_brightness = DEFAULT_BRIGHTNESS; // Default brightness
};

// Utility function to parse JSON into DisplayState (for clients)
DisplayState parseDisplayStateFromJSON(const nlohmann::json& json);

} // namespace sequence
