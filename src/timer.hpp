#ifndef timer_hpp
#define timer_hpp

#include <chrono>
#include <functional>
#include <thread>
#include <condition_variable>
#include <memory>

namespace timer
{

class Timer
{
public:
    ~Timer();

    void setInterval(const std::function<void()>& function, std::chrono::nanoseconds interval);
    void stop();

private:
    bool m_clear = false;
    std::condition_variable m_abort;
    std::thread m_thread;
};

std::unique_ptr<Timer> createTimer(
    std::chrono::milliseconds interval,
    const std::function<void()>& callback
);

} // namespace timer

#endif
