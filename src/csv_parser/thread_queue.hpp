#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <type_traits>

template<typename T>
class ThreadQueue {
public:
    static_assert(std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>,"ThreadQueue requires T to be either copy constructible or move constructible");

    ThreadQueue() = default;
    ~ThreadQueue()
    {
        delete_queue();
    }

    void delete_queue()
    {
        m_stop_flag.store(true, std::memory_order_release);
        m_cv.notify_all();
        while (!m_queue.empty())
        {
            m_queue.pop();
        }
    }

    void push(T&& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_finished.load(std::memory_order_acquire))
        {
            return;
        }
        m_queue.push(std::move(value));
        m_cv.notify_one();
    }

    bool front(T& value)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] {
            return m_finished.load(std::memory_order_acquire) || !m_queue.empty() || m_stop_flag.load(std::memory_order_acquire);
        });

        if (m_queue.empty() || m_stop_flag.load(std::memory_order_acquire))
        {
            return false;
        }

        value = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void stop()
    {
        m_finished.store(true, std::memory_order_release);
        m_cv.notify_all();
    }

    bool empty() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_finished{ false };
    std::atomic<bool> m_stop_flag{ false };
};