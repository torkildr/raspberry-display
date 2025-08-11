#ifndef sequence_hpp
#define sequence_hpp

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

namespace sequence
{

// Sequence state structure
struct SequenceState {
    nlohmann::json state;           // The display state JSON
    double time;                    // Display time in seconds
    double ttl;                     // Time-to-live in seconds
    std::chrono::steady_clock::time_point created_at;  // When this state was created
};

class SequenceManager
{
public:
    using StateCallback = std::function<void(const nlohmann::json&)>;
    
    SequenceManager(StateCallback callback);
    ~SequenceManager();
    
    void addSequenceState(const nlohmann::json& state, double time, double ttl);
    void setSequence(const std::vector<SequenceState>& sequence);
    void clearSequence();
    bool isActive() const;
    
private:
    void processSequence();
    void removeExpiredStates();
    
    StateCallback m_callback;
    std::vector<SequenceState> m_sequence;
    mutable std::mutex m_mutex;
    std::atomic<size_t> m_current_index{0};
    std::atomic<bool> m_active{false};
    std::chrono::steady_clock::time_point m_state_start_time;
    
    std::unique_ptr<timer::Timer> m_timer;
    static constexpr double TIMER_INTERVAL = 0.01; // 10ms timer resolution
};

} // namespace sequence

#endif
