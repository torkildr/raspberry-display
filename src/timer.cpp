#include <thread>
#include <atomic>

#include "timer.hpp"

#include <iostream>
#include <ostream>

using namespace std::chrono;

namespace timer
{

std::unique_ptr<Timer> createTimer(const std::function<void()>& callback, double seconds)
{
    auto timer = std::make_unique<Timer>();
    auto intervalNs = static_cast<long long>(seconds * 1e9);
    nanoseconds interval(intervalNs);

    timer->setInterval(callback, interval);

    return timer;
}

Timer::~Timer()
{
    stop();
}

void Timer::setInterval(const std::function<void()>& function, nanoseconds interval)
{
    m_clear = false;
    m_thread = std::thread([this, function, interval]() {
        std::mutex mutex;
        auto next_execution = steady_clock::now() + interval;

        while (true)
        {
            if (m_clear)
                return;
            
            // Wait until the scheduled execution time
            std::unique_lock<std::mutex> lock(mutex);
            if (m_abort.wait_until(lock, next_execution) == std::cv_status::no_timeout) {
                // Woken up by abort signal, not timeout
                if (m_clear)
                    return;
            }
            
            if (m_clear)
                return;

            // Execute function
            function();
            
            // Schedule next execution at fixed interval from the original schedule
            next_execution += interval;
            
            // If we're already past the next scheduled time, execute immediately
            auto now = steady_clock::now();
            if (next_execution <= now) {
                next_execution = now;
            }
        }
    });
}

void Timer::stop()
{
    if (m_thread.joinable())
    {
        m_clear = true;
        m_abort.notify_all();
        m_thread.join();
    }
}



} // namespace timer
