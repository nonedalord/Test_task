#pragma once

#include "../csv_parser/thread_pool_queue.hpp"
#include "serializer.hpp"
#include "algorithm.hpp"

#include <vector>
#include <cstdint>
#include <string>
#include <fstream>
#include <mutex>

template<typename T, typename Compare = std::less<>>
class OutWriter
{
public:
    OutWriter(uint64_t max_elements, std::shared_ptr<ISerializer<T>> serializer, std::shared_ptr<IAlgorithm<T>> algorithm, Compare comp = Compare(), uint32_t max_threads = 4);
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
    std::shared_ptr<IAlgorithm<T>> m_algorithm;
    std::unique_ptr<ThreadPoolQueue> m_queue;
    std::vector<std::string> m_file_to_merge;
    Compare m_comp;
    std::vector<T> m_buff;
    uint64_t m_max_elements;
};

#include "out_writer_impl.hpp"