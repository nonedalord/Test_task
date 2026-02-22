#pragma once

#include "thread_pool_queue.hpp"
#include "thread_queue.hpp"

#include <thread>
#include <vector>
#include <optional>
#include <condition_variable>
#include <atomic>
#include <cstdint>

class CsvParser
{
public:
    explicit CsvParser(u_int64_t total_space_to_use, u_int32_t max_threads = 4);
    ~CsvParser();
    void add_file_to_parse(const std::string& file_path);
    struct ParserData
    {
        uint64_t receive_ts;
        double price;
    };
    std::optional<std::vector<ParserData>> get_ready_data();
    inline uint32_t get_max_elements()
    {
        return m_max_elements;
    }
private:
    void parse_csv_data(const std::string& file_name);
    bool check_empty_file(const std::string& file_path);
    void wait_task_done();
    void notify_task();

    std::unique_ptr<ThreadQueue<std::vector<ParserData>>> m_ready_data_queue;
    std::unique_ptr<ThreadPoolQueue> m_queue;
    std::thread m_task_wait_thread;
    std::condition_variable m_cv_wait_task;
    std::mutex m_wait_task_mutex;
    u_int64_t m_vec_size {};
    u_int64_t m_max_elements {};
    std::atomic<uint32_t> m_total_task;
    uint32_t m_max_threads {};
};