#include <algorithm>
#include <iostream>

#include "sequence.hpp"
#include "debug_util.hpp"

using namespace std::chrono;

namespace sequence
{

SequenceManager::SequenceManager(StateCallback callback)
    : m_callback(std::move(callback))
{
    // Start the processing timer
    m_timer = timer::createTimer([this]() { processSequence(); }, TIMER_INTERVAL);
}

SequenceManager::~SequenceManager()
{
    clearSequence();
}

void SequenceManager::addSequenceState(const nlohmann::json& state, double time, double ttl)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = steady_clock::now();
    m_sequence.push_back({state, time, ttl, now});
    
    // If this is the first item and no sequence is running, start it
    if (m_sequence.size() == 1 && !m_active) {
        m_active = true;
        m_current_index = 0;
        m_state_start_time = now;
        
        // Execute the callback for the first state immediately
        if (m_callback) {
            m_callback(m_sequence[0].state);
        }
    }
}

void SequenceManager::setSequence(const std::vector<SequenceState>& sequence)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = steady_clock::now();
    m_sequence.clear();
    
    // Copy sequence and set creation time
    for (const auto& item : sequence) {
        SequenceState state_copy = item;
        state_copy.created_at = now;
        m_sequence.push_back(state_copy);
    }
    
    if (!m_sequence.empty()) {
        m_active = true;
        m_current_index = 0;
        m_state_start_time = now;
        
        // Execute the callback for the first state immediately
        if (m_callback) {
            m_callback(m_sequence[0].state);
        }
    } else {
        m_active = false;
    }
}

void SequenceManager::clearSequence()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sequence.clear();
    m_active = false;
    m_current_index = 0;
}

bool SequenceManager::isActive() const
{
    return m_active;
}

void SequenceManager::processSequence()
{
    if (!m_active) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Remove expired states first
    removeExpiredStates();
    
    if (m_sequence.empty()) {
        m_active = false;
        return;
    }
    
    // If there's only one element, don't transition to itself - just stay put
    if (m_sequence.size() == 1) {
        return;
    }
    
    // Ensure current index is valid after removal
    if (m_current_index >= m_sequence.size()) {
        m_current_index = 0;
        m_state_start_time = steady_clock::now();
        if (m_callback) {
            m_callback(m_sequence[0].state);
        }
        return;
    }
    
    const auto& current_state = m_sequence[m_current_index];
    auto now = steady_clock::now();
    auto state_elapsed = duration<double>(now - m_state_start_time).count();
    
    // Check if it's time to move to the next state
    if (state_elapsed >= current_state.time) {
        // Move to next state
        m_current_index++;
        
        if (m_current_index >= m_sequence.size()) {
            // Loop back to beginning
            m_current_index = 0;
            DEBUG_LOG("Looping sequence back to start");
        }
        
        m_state_start_time = now;
        if (m_callback) {
            m_callback(m_sequence[m_current_index].state);
        }
        DEBUG_LOG("Advanced to sequence state " << m_current_index);
    }
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
    
    // If we removed states and current index is now invalid, reset to start
    if (m_sequence.size() < original_size && m_current_index >= m_sequence.size()) {
        if (!m_sequence.empty()) {
            m_current_index = 0;
            m_state_start_time = now;
            if (m_callback) {
                m_callback(m_sequence[0].state);
            }
            DEBUG_LOG("Reset to first state after TTL cleanup");
        }
    }
}

} // namespace sequence
