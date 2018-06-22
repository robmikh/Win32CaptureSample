#pragma once

#include <chrono>
#include <mutex>
#include <condition_variable>

// taken from https://stackoverflow.com/questions/14920725/waiting-for-an-atomic-bool#14921115
struct safe_flag
{
    safe_flag(std::nullptr_t) {}
    safe_flag() : m_flag(false) {}

    bool is_set() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_flag;
    }

    void set()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_flag = true;
        }
        m_cv.notify_all();
    }

    void reset()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_flag = true;
        }
        m_cv.notify_all();
    }

    void wait() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return m_flag; });
    }

    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& rel_time) const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(llock, rel_time, [this] { return m_flag; });
    }

    template <typename Rep, typename Period>
    bool wait_until(const std::chrono::duration<Rep, Period>& rel_time) const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_until(lock, rel_time, [this] { return m_flag; });
    }

private:
    bool m_flag;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
};