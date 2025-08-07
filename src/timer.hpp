#ifndef timer_hpp
#define timer_hpp

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

    void setInterval(std::function<void()> function, std::chrono::nanoseconds interval);
    void stop();

private:
    bool m_clear = false;
    std::condition_variable m_abort;
    std::thread m_thread;
};

std::unique_ptr<Timer> createTimer(std::function<void()> callback, double seconds);

} // namespace timer

#endif
