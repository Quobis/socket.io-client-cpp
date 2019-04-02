#ifndef SIO_TIMER_H
#define SIO_TIMER_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace sio {
namespace quobis {

class timer {
    public:
        template <class Rep, class Period>
        timer(const std::function<void()> job,
              const std::chrono::duration<Rep, Period>& time)
            : m_job{job},
              m_end_time{std::chrono::steady_clock::now() + time},
              m_wait_thread{std::bind(&timer::wait, this)} {}

        void stop() {
            {
                std::lock_guard<std::mutex> lock{m_mt};
                if (m_stopped) {
                    return;
                }
                m_stopped = true;
            }
            m_cv.notify_one();
            m_wait_thread.join();
        }

        ~timer() {
            timer::stop();
        }

    private:
        void wait() {
            std::cv_status status{std::cv_status::no_timeout};
            std::unique_lock<std::mutex> lock{m_mt};
            while (m_stopped == false && status == std::cv_status::no_timeout) {
                status = m_cv.wait_until(lock, m_end_time);
            }
            if (m_stopped == false && status == std::cv_status::timeout) {
                m_job();
            }
        }

    private:
        bool m_stopped{false};
        std::mutex m_mt;
        std::condition_variable m_cv;
        std::function<void()> m_job;
        decltype(std::chrono::steady_clock::now()) m_end_time;
        std::thread m_wait_thread;
};

}
}

#endif

