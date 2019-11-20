#include <thread>
#include <atomic>

#include "timer.hpp"

namespace timer {

std::unique_ptr<Timer> createTimer(std::function<void()> callback, float seconds) {
    auto timer = std::make_unique<Timer>();
    int milliseconds = seconds * 1000;

    timer->setInterval([callback] {
        callback();
    }, milliseconds);

    return timer;
}

Timer::~Timer() {
    stop();
}

void Timer::setInterval(std::function<void()> function, int interval) {
    m_clear = false;
    m_thread = std::thread([=]() {
        std::mutex mutex;

        while(true) {
            if(m_clear) return;
            std::unique_lock<std::mutex> lock(mutex);
            m_abort.wait_for(lock, std::chrono::milliseconds(interval));
            if(m_clear) return;
            function();
        }
    });
}

void Timer::stop() {
    if (m_thread.joinable()) {
        m_clear = true;
        m_abort.notify_all();
        m_thread.join();
    }
}

}
