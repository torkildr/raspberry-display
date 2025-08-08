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

        nanoseconds nextInterval = interval;

        while (true)
        {
            if (m_clear)
                return;
            std::unique_lock<std::mutex> lock(mutex);
            m_abort.wait_for(lock, nextInterval);
            if (m_clear)
                return;

            time_point<high_resolution_clock> before = high_resolution_clock::now();
            function();
            time_point<high_resolution_clock> after = high_resolution_clock::now();

            duration<double> duration = after - before;
            nextInterval = interval - duration_cast<nanoseconds>(duration);

            if (nextInterval.count() < 0)
                nextInterval = nanoseconds(0);
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
