#include <algorithm>
#include <iostream>
#include <optional>

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

void SequenceManager::stopSequence()
{
    m_active = false;
    m_current_element.reset();
    setDefaultContent();
}

void SequenceManager::startSequence()
{
    if (m_sequence.empty()) {
        return;
    }
    
    m_active = true;
    m_current_element = m_sequence.first();
    m_state_start_time = std::chrono::steady_clock::now();
    
    // Execute the first state immediately if element is valid
    if (m_current_element && !m_current_element->isMarkedForDeletion()) {
        processDisplayState(m_current_element->getId(), m_current_element->getData().state);
    } else {
        stopSequence();
    }
}


void SequenceManager::addSequenceState(const std::string& sequence_id, const DisplayState& state, double time, double ttl)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (sequence_id.empty()) {
        ERROR_LOG("Sequence state must have a non-empty sequence_id");
        return;
    }
    
    SequenceState seq_state;
    seq_state.state = state;
    seq_state.time = time;
    seq_state.ttl = ttl;
    seq_state.sequence_id = sequence_id;
    seq_state.created_at = std::chrono::steady_clock::now();

    m_sequence.insert(sequence_id, seq_state);
    
    // If this is the first item and no sequence is running, start it
    if (m_sequence.size() == 1 && !m_active) {
        startSequence();
    }
    
    DEBUG_LOG("Added sequence state with time=" << time << ", ttl=" << ttl << ", id='" << sequence_id << "'");
    DEBUG_LOG("Sequence size = " << m_sequence.size() << ", addSequence");
}

void SequenceManager::setSequence(const std::vector<SequenceState>& sequence)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_sequence.clear();
    m_active = false;
    
    auto now = steady_clock::now();
    for (SequenceState state : sequence) {
        if (state.sequence_id.empty()) {
            ERROR_LOG("Sequence state must have a non-empty sequence_id");
            continue;
        }
        state.created_at = now;
        m_sequence.insert(state.sequence_id, state);
    }
    
    if (!m_sequence.empty()) {
        startSequence();
    }

    DEBUG_LOG("Sequence size = " << m_sequence.size() << ", setSequence");
}

void SequenceManager::setDefaultContent()
{
    DEBUG_LOG("Setting default (empty) content");
    if (m_display) {
        m_active = false;
        m_last_shown_id = std::nullopt;
        
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

    m_sequence.forEach([&ids](const std::string& id, const SequenceState&) {
        ids.push_back(id);
    });
    
    return ids;
}

std::string SequenceManager::getCurrentSequenceId() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current_element ?  m_current_element->getId() : "<none>";
}

size_t SequenceManager::getSequenceCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sequence.size();
}

void SequenceManager::clearSequenceById(const std::string& sequence_id)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (sequence_id.empty()) {
        ERROR_LOG("Sequence ID is empty");
        return;
    }
    
    m_sequence.erase(sequence_id);
    
    if (m_sequence.empty()) {
        stopSequence();
    }
}

void SequenceManager::processSequence(bool skip_current)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_active || m_sequence.empty()) {
        return;
    }

    if (!m_current_element) {
        startSequence();
        return;
    }

    /*
    // Check if current element is marked for deletion or doesn't exist
    if (m_current_element->isMarkedForDeletion()) {
        m_current_element = m_sequence.first();
        if (!m_current_element) {
            stopSequence();
            return;
        }
        
        // Display the first state of the new cycle
        processDisplayState(m_current_element->getId(), m_current_element->getData().state);
        m_state_start_time = steady_clock::now();
        return;
    }
    */

    const auto& sequence_state = m_current_element->getData();

    if (isStateExpired(sequence_state)) {
        DEBUG_LOG("Erasing expired state: " << m_current_element->getId());
        auto id_to_erase = m_current_element->getId();
        m_current_element = m_current_element->next();
        m_sequence.erase(id_to_erase);
        
        if (m_sequence.empty()) {
            stopSequence();
            return;
        }
        
        // Check if current element is null after moving to next
        if (!m_current_element) {
            startSequence();
            return;
        }
        
        skip_current = true;
    }
    
    auto now = steady_clock::now();
    auto state_elapsed = duration<double>(now - m_state_start_time).count();
    
    // Check if it's time to move to the next state
    if (skip_current || state_elapsed >= sequence_state.time) {
        // Check if current element is still valid before calling next()
        if (!m_current_element) {
            startSequence();
            return;
        }
        
        // Move to next state
        m_current_element = m_current_element->next();
        
        // Check if we've looped back or have no valid next element
        if (!m_current_element) {
            // Restart sequence from beginning
            startSequence();
        } else {
            // Display the new state
            processDisplayState(m_current_element->getId(), m_current_element->getData().state);
        }
        
        m_state_start_time = now;
    }
}

void SequenceManager::nextState()
{
    processSequence(true);
}

bool SequenceManager::isStateExpired(const SequenceState& state)
{
    auto now = steady_clock::now();
    auto elapsed = duration<double>(now - state.created_at).count();
    return elapsed >= state.ttl;
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
    stopSequence();
    if (m_display) {
        m_display->stop();
    }
}

void SequenceManager::processDisplayState(const std::optional<std::string> sequence_id, const DisplayState& state)
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

    auto transition_type= state.transition_type;
    if (m_last_shown_id.has_value() && sequence_id.has_value() && m_last_shown_id.value() == sequence_id.value()) {
        transition_type = transition::Type::NONE;
    }
    
    m_last_shown_id = sequence_id;
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
