#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPoolQueue {
public:
    ThreadPoolQueue() : m_pending_tasks(0), m_is_running(true) {};

    void start_async(unsigned int max_threads)
    {
        for (int i = 0; i < max_threads; ++i)
        {
            m_threads_vec.emplace_back([this]{
                while (m_is_running.load(std::memory_order_acquire))
                {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(m_mutex);
                        m_cv.wait(lock, [this] { 
                            return !m_is_running.load(std::memory_order_acquire) || !m_tasks_queue.empty(); 
                        });

                        if (m_tasks_queue.empty())
                        {
                            continue;
                        }
                        task = std::move(m_tasks_queue.front());
                        m_tasks_queue.pop();
                    }
                    try
                    {
                        task();
                    }
                    catch (const std::exception& err)
                    {

                    }
                    m_pending_tasks.fetch_sub(1, std::memory_order_release);
                    if (m_pending_tasks.load(std::memory_order_acquire) == 0)
                    {
                        std::lock_guard<std::mutex> lock(m_wait_task_mutex);
                        m_wait_task_cv.notify_all();
                    }
                }
            });
        }
    }

    ~ThreadPoolQueue() 
    {
        stop();
    }

    void stop()
    {
        m_is_running.store(false, std::memory_order_release);
        m_cv.notify_all();
        m_pending_tasks.store(0, std::memory_order_release);
        m_wait_task_cv.notify_all();
        for (auto& task : m_threads_vec)
        {
            if (task.joinable())
            {
                task.join();
            }
        }
    }

    template<class T>
    void push(T&& task) 
    {
        if (!m_is_running.load(std::memory_order_acquire))
        {
            return;
        }
        m_pending_tasks.fetch_add(1, std::memory_order_relaxed);
        {
            std::unique_lock lock(m_mutex);
            m_tasks_queue.emplace(std::forward<T>(task));
        }
        m_cv.notify_one();
    }

    void wait_for_pending()
    {
        std::unique_lock lock(m_wait_task_mutex);
        m_wait_task_cv.wait(lock, [this] {
            return m_pending_tasks.load(std::memory_order_acquire) == 0;
        });
    }
private:
    std::vector<std::thread> m_threads_vec;
    std::queue<std::function<void()>> m_tasks_queue;
    std::mutex m_mutex;
    std::mutex m_wait_task_mutex;
    std::condition_variable m_wait_task_cv;
    std::condition_variable m_cv;
    std::atomic<uint64_t> m_pending_tasks;  
    std::atomic<bool> m_is_running;
};