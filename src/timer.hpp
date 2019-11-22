#ifndef timer_hpp
#define timer_hpp

#include <functional>
#include <thread>
#include <condition_variable>

namespace timer {

class Timer {
    public:
        ~Timer();

        void setInterval(std::function<void()> function, int interval);
        void stop();

    private:
        bool m_clear = false;
        std::condition_variable m_abort;

        std::thread m_thread;
};

std::unique_ptr<Timer> createTimer(std::function<void()> callback, float seconds);

}

#endif