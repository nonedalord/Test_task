#pragma once

#include "thread_pool_queue.hpp"
#include "thread_queue.hpp"

#include <thread>

class CsvParser
{
public:
    explicit CsvParser(int max_threads = 4, unsigned long total_space_to_use);
    void add_file_to_parse(const std::string& file_path);
    struct ParserData
    {
        uint64_t receive_ts;
        double median;
    };
private:
    void parse_csv_data(const std::string& file_name);
    bool check_empty_file(const std::string& file_path);

    std::unique_ptr<ThreadQueue<std::vector<ParserData>>> m_ready_data_queue;
    std::unique_ptr<ThreadPoolQueue> m_queue;
    unsigned long m_vec_size;
    int m_max_threads;
};