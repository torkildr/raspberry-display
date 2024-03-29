#ifndef timer_hpp
#define timer_hpp

#include <functional>
#include <thread>
#include <condition_variable>

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

Timer* createTimer(std::function<void()> callback, double seconds);

} // namespace timer

#endif
