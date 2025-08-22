#include <algorithm>
#include <iostream>

#include "display.hpp"
#include "sequence.hpp"
#include "log_util.hpp"
#include "transition.hpp"
#include "utf8_converter.hpp"

using namespace std::chrono;

namespace sequence
{

SequenceManager::SequenceManager(std::unique_ptr<display::Display> display)
    : m_display(std::move(display))
{
    setDefaultContent();
   
    m_timer = timer::createTimer(TIMER_INTERVAL, [this]() {
        processSequence();
    });
    
    if (m_display) {
        m_display->start();
        m_display->setBrightness(DEFAULT_BRIGHTNESS);
        m_current_brightness = DEFAULT_BRIGHTNESS;
    } else {
        throw std::runtime_error("Display not initialized");
    }
}

SequenceManager::~SequenceManager()
{
    clearSequence();
    if (m_display) {
        m_display->stop();
    }
}

void SequenceManager::addSequenceState(const DisplayState& state, double time, double ttl, const std::string& sequence_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove existing states with the same sequence_id if specified
    if (!sequence_id.empty()) {
        m_sequence.erase(
            std::remove_if(m_sequence.begin(), m_sequence.end(),
                [&sequence_id](const SequenceState& s) {
                    return s.sequence_id == sequence_id;
                }),
            m_sequence.end()
        );
    }
    
    // Add the new sequence state
    SequenceState seq_state;
    seq_state.state = state;
    seq_state.time = time;
    seq_state.ttl = ttl;
    seq_state.sequence_id = sequence_id;
    seq_state.created_at = std::chrono::steady_clock::now();
    
    m_sequence.push_back(seq_state);
    
    // If this is the first item and no sequence is running, start it
    if (m_sequence.size() == 1 && !m_active) {
        m_active = true;
        m_current_index = 0;
        m_state_start_time = seq_state.created_at;
        
        // Execute the first state immediately
        processDisplayState(m_sequence[0].state);
    }
    
    DEBUG_LOG("Added sequence state with time=" << time << ", ttl=" << ttl << ", id='" << sequence_id << "'");
    DEBUG_LOG("Sequence size = " << m_sequence.size() << ", addSequence");
}

void SequenceManager::setSequence(const std::vector<SequenceState>& sequence)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_sequence.clear();
    m_active = false;
    m_current_index = 0;
    
    // Copy all sequence states
    m_sequence = sequence;
    
    // Update created_at timestamps to now for all states
    auto now = steady_clock::now();
    for (auto& state : m_sequence) {
        state.created_at = now;
    }
    
    // If we have states, start the sequence
    if (!m_sequence.empty()) {
        m_active = true;
        m_current_index = 0;
        m_state_start_time = now;
        
        // Execute the first state immediately
        processDisplayState(m_sequence[0].state);
    }

    DEBUG_LOG("Sequence size = " << m_sequence.size() << ", setSequence");
}

void SequenceManager::setDefaultContent()
{
    DEBUG_LOG("Setting default (empty) content");
    if (m_display) {
        m_active = false;
        m_current_index = 0;
        
        // TODO: enable this when we know how to stop transition after the first
        // m_display->setTransition(transition::Type::DISSOLVE, 2.0);
        m_display->setAlignment(display::Alignment::CENTER);
        m_display->show(std::nullopt, "");
    }
}

void SequenceManager::clearSequence(bool set_default_content)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sequence.clear();
    if (set_default_content) {
        setDefaultContent();
    }
    DEBUG_LOG("Sequence size = " << m_sequence.size() << ", clearSequence");
}

bool SequenceManager::isActive() const
{
    return m_active;
}

std::vector<std::string> SequenceManager::getActiveSequenceIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    
    for (const auto& state : m_sequence) {
        if (!state.sequence_id.empty()) {
            // Only add if not already in the list (avoid duplicates)
            if (std::find(ids.begin(), ids.end(), state.sequence_id) == ids.end()) {
                ids.push_back(state.sequence_id);
            }
        }
    }
    
    return ids;
}

std::string SequenceManager::getCurrentSequenceId() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_active || m_sequence.empty() || m_current_index >= m_sequence.size()) {
        return "";
    }
    
    return m_sequence[m_current_index].sequence_id;
}

size_t SequenceManager::getCurrentSequenceIndex() const
{
    return m_current_index;
}

size_t SequenceManager::getSequenceCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sequence.size();
}

void SequenceManager::clearSequenceById(const std::string& sequence_id)
{
    if (sequence_id.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove all states with the specified sequence_id
    m_sequence.erase(
        std::remove_if(m_sequence.begin(), m_sequence.end(),
            [&sequence_id](const SequenceState& s) {
                return s.sequence_id == sequence_id;
            }),
        m_sequence.end()
    );
    
    // If sequence is now empty, deactivate
    if (m_sequence.empty()) {
        m_active = false;
        m_current_index = 0;
    }
    // If current index is out of bounds, reset to 0
    else if (m_current_index >= m_sequence.size()) {
        m_current_index = 0;
        m_state_start_time = std::chrono::steady_clock::now();
        
        // Execute new current state
        processDisplayState(m_sequence[0].state);
    }
}

void SequenceManager::processSequence(bool skip_current)
{
    if (!m_active) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    removeExpiredStates();
    
    if (m_sequence.empty()) {
        m_active = false;
        return;
    }
    
    // Ensure current index is valid after removal
    if (m_current_index >= m_sequence.size()) {
        m_current_index = 0;
        m_state_start_time = steady_clock::now();
        processDisplayState(m_sequence[0].state);
        return;
    }

    const auto& current_state = m_sequence[m_current_index];
    auto now = steady_clock::now();
    auto state_elapsed = duration<double>(now - m_state_start_time).count();
    
    // Check if it's time to move to the next state
    if (skip_current || state_elapsed >= current_state.time) {
        // Move to next state
        m_current_index++;
        
        if (m_current_index >= m_sequence.size()) {
            // Loop back to beginning
            m_current_index = 0;
        }
        
        m_state_start_time = now;
        processDisplayState(m_sequence[m_current_index].state);
    }
}

void SequenceManager::nextState()
{
    processSequence(true);
}

void SequenceManager::removeExpiredStates()
{
    auto now = steady_clock::now();
    size_t original_size = m_sequence.size();
    
    // Remove states that have exceeded their TTL
    m_sequence.erase(
        std::remove_if(m_sequence.begin(), m_sequence.end(),
            [now](const SequenceState& state) {
                if (state.ttl <= 0) {
                    return false; // No TTL means never expire
                }
                auto elapsed = duration<double>(now - state.created_at).count();
                return elapsed >= state.ttl;
            }),
        m_sequence.end());
    
    // Handle sequence state changes after TTL cleanup
    if (m_sequence.size() < original_size) {
        if (m_sequence.empty()) {
            setDefaultContent();
            DEBUG_LOG("All sequence items expired - cleared screen and went inactive");
        }
        else if (m_current_index >= m_sequence.size()) {
            // Some items expired but sequence not empty - reset to first remaining item
            m_current_index = 0;
            m_state_start_time = now;
            processDisplayState(m_sequence[0].state);
            DEBUG_LOG("Reset to first state after TTL cleanup");
        }
    }
}

// Display control methods (global state management)
void SequenceManager::setBrightness(int brightness)
{
    if (m_display) {
        m_display->setBrightness(brightness);
        m_current_brightness = brightness;
    }
}

void SequenceManager::setScrolling(display::Scrolling direction)
{
    if (m_display) {
        m_display->setScrolling(direction);
    }
}

void SequenceManager::setAlignment(display::Alignment alignment)
{
    if (m_display) {
        m_display->setAlignment(alignment);
    }
}

display::Alignment SequenceManager::getAlignment() const
{
    if (m_display) {
        return m_display->getAlignment();
    }
    return display::Alignment::LEFT;
}

void SequenceManager::setDefaultTransition(transition::Type type, double duration)
{
    m_default_transition_type = type;
    m_default_transition_duration = duration;
    if (m_display) {
        m_display->setTransition(type, duration);
    }
}

// Display lifecycle methods
void SequenceManager::start()
{
    if (m_display) {
        m_display->start();
    }
}

void SequenceManager::stop()
{
    m_active = false;
    if (m_display) {
        m_display->stop();
    }
}

void SequenceManager::processDisplayState(const DisplayState& state)
{
    if (!m_display) {
        return;
    }
    
    if (state.alignment.has_value()) {
        m_display->setAlignment(state.alignment.value());
        if (state.alignment.value() == display::Alignment::LEFT && !state.scrolling.has_value()) {
            m_display->setScrolling(display::Scrolling::DISABLED);
        }
    }
    
    if (state.scrolling.has_value()) {
        m_display->setScrolling(state.scrolling.value());
    }
    
    if (state.brightness.has_value()) {
        int brightness = state.brightness.value();
        if (brightness >= 0 && brightness <= 15) {
            m_display->setBrightness(brightness);
            m_current_brightness = brightness;
        }
    }

    auto transition_type= (m_sequence.size() > 1)
        ? state.transition_type
        : transition::Type::NONE;
    
    m_display->show(state.text, state.time_format, transition_type, state.transition_duration);
}

// Utility function to parse JSON into DisplayState (for clients)
DisplayState parseDisplayStateFromJSON(const nlohmann::json& json)
{
    DisplayState state;
    
    try {
        // Handle main content
        if (json.contains("text")) {
            state.text = utf8::toLatin1(json["text"].get<std::string>());
            
            if (json.contains("show_time") && json["show_time"].get<bool>()) {
                state.time_format = json.contains("time_format")
                    ? utf8::toLatin1(json["time_format"].get<std::string>())
                    : "";
            }
        } else if (json.contains("show_time") && json["show_time"].get<bool>()) {
            state.time_format = json.contains("time_format")
                ? utf8::toLatin1(json["time_format"].get<std::string>())
                : "";
        }

        // Parse visual properties
        if (json.contains("alignment")) {
            std::string alignment = json["alignment"];
            if (alignment == "center" || alignment == "centre") {
                state.alignment = display::Alignment::CENTER;
            } else if (alignment == "left") {
                state.alignment = display::Alignment::LEFT;
            }
        }
        
        if (json.contains("scroll")) {
            std::string scroll = json["scroll"];
            if (scroll == "enabled" || scroll == "true") {
                state.scrolling = display::Scrolling::ENABLED;
            } else if (scroll == "disabled" || scroll == "false") {
                state.scrolling = display::Scrolling::DISABLED;
            } else if (scroll == "reset") {
                state.scrolling = display::Scrolling::RESET;
            }
        }
        
        if (json.contains("brightness")) {
            int brightness = json["brightness"];
            if (brightness >= 0 && brightness <= 15) {
                state.brightness = brightness;
            }
        }
        
        // Parse transition
        if (json.contains("transition")) {
            auto trans_data = json["transition"];
            
            if (trans_data.is_string()) {
                // Simple string format: "wipe_left", "dissolve", etc.
                state.transition_type = transition::TransitionFactory::parseType(trans_data.get<std::string>());
            } else if (trans_data.is_object()) {
                // Object format: {"type": "wipe_left", "duration": 1.5}
                if (trans_data.contains("type")) {
                    state.transition_type = transition::TransitionFactory::parseType(trans_data["type"].get<std::string>());
                }
                if (trans_data.contains("duration")) {
                    state.transition_duration = trans_data["duration"].get<double>();
                }
            }
        } else {
            state.transition_type = transition::Type::NONE;
            state.transition_duration = 0.0;
        }
        
    } catch (const std::exception& e) {
        LOG("Error parsing JSON to DisplayState: " << e.what());
    }
    
    return state;
}

} // namespace sequence
