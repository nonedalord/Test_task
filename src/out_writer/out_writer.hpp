#pragma once

#include "./csv_parser/thread_pool_queue.hpp"
#include "serializer.hpp"
#include "algorithm.hpp"

#include <vector>
#include <cstdint>
#include <string>

template<typename T, typename Compare = std::less<>>
class OutWriter
{
public:
    OutWriter(u_int64_t max_elements, const std::string& out_file_name, std::shared_ptr<ISerializer<T>> serializer, std::unique_ptr<IAlgorithm<T>> algorithm, Compare comp = Compare());
    ~OutWriter();
    void collect_data(std::vector<T>&& data);
    void write_data(const std::string& file_name);
private:
    struct FileStream 
    {
        std::ifstream stream;
        uint64_t remaining;
        T current;
    };

    void write_to_temporary(std::vector<T>&& data);
    std::string merge_sort();
    std::shared_ptr<ISerializer<T>>  m_serializer;
    std::unique_ptr<IAlgorithm<T>> m_algorithm;
    std::unique_ptr<ThreadPoolQueue> m_queue;
    std::vector<std::string> m_file_to_merge;
    Compare m_comp;
    std::vector<T> m_buff;
    std::string out_file_name;
    u_int64_t m_max_elements;
};