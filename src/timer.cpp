#include <thread>

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
        while(true) {
            if(m_clear) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if(m_clear) return;
            function();
        }
    });
}

void Timer::stop() {
    if (m_thread.joinable()) {
        m_clear = true;
        m_thread.join();
    }
}

}
