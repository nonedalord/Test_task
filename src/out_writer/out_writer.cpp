#include "out_writer.hpp"
#include "../logger/logger.hpp"

#include <ranges>
#include <fstream>
#include <queue>
#include <string_view>

namespace
{
    constexpr uint64_t binary_hash = 0x12345678;

    std::string generate_file_name() 
    {
        static std::string base_name = "binary_data";
        static int counter = 0;
        return base_name + "_" + std::to_string(binary_hash) + "_" + std::to_string(counter++) + ".bin";
    }
} //anonymous namespace

template<typename T, typename Compare>
OutWriter<T, Compare>::OutWriter(u_int64_t max_elements, const std::string& out_file_name, std::shared_ptr<ISerializer<T>> serializer, std::unique_ptr<IAlgorithm<T>> algorithm, 
    Compare comp = Compare()) : 
m_serializer(serializer), m_algorithm(std::move(algorithm)), comp(comp), m_out_file_name(out_file_name), m_max_elements(max_elements) 
{
    m_buff.reserve(m_max_elements);
    m_queue->start_async(4);
}

template <typename T, typename Compare>
OutWriter<T, Compare>::~OutWriter()
{
    m_queue->stop();
}

template<typename T, typename Compare>
void OutWriter<T, Compare>::collect_data(std::vector<T>&& data)
{
    if (m_buff.size() + data.size() > m_max_elements)
    {
        u_int64_t offset = m_max_elements - m_buff.size();
        m_buff.insert(m_buff.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.begin() + offset));
        std::ranges::sort(m_buff, m_comp);
        write_to_temporary(std::move(m_buff));
        m_buff.insert(m_buff.end(), std::make_move_iterator(data.begin() + offset), std::make_move_iterator(data.end()));
        return;
    }
    m_buff.insert(m_buff.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
}

template<typename T, typename Compare>
void OutWriter<T, Compare>::write_data(const std::string& file_name)
{
    if (m_file_to_merge.empty())
    {
        if (!m_buff.empty())
        {
            std::ranges::sort(m_buff, m_comp);
            m_algorithm->process_in_memory(std::move(m_buff), m_out_file_name);
        }
        else
        {
            spdlog::error("Empty data, can't find median");
            return;
        }
    }
    else
    {
        std::ranges::sort(m_buff, m_comp);
        write_to_temporary(m_buff);
        m_buff.clear();
        m_algorithm->process_file(m_serializer, merge_sort(), m_out_file_name)
    }
}

template<typename T, typename Compare>
std::string OutWriter<T, Compare>::merge_sort()
{
    std::vector<FileStream> streams;
    streams.reserve(m_file_to_merge.size());

    for (const auto& fname : m_file_to_merge)
    {
        std::ifstream ifs(fname, std::ios::binary);
        if (!ifs.is_open())
        {
            spdlog::error("Cannot open temporary file {}", fname);
            continue;
        }
        uint64_t total_elements = 0;
        ifs.read(reinterpret_cast<char*>(&total_elements), sizeof(total_elements));
        if (total_elements == 0) continue;
        T first;
        m_serializer->read(ifs, first);
        streams.push_back(FileStream{std::move(ifs), total_elements - 1, std::move(first)});
    }

    if (streams.empty()) 
    {
        spdlog::error("No data to merge");
        return "";
    }

    auto heap_cmp = [this](const FileStream* a, const FileStream* b) 
    {
        return m_comp(b->current, a->current);
    };

    std::priority_queue<FileStream*, std::vector<FileStream*>, decltype(heap_cmp)> prior_queue(heap_cmp);

    for (auto& fs : streams) 
    {
        prior_queue.push(&fs);
    }

    std::string file_name = generate_file_name();
    std::ofstream out(file_name, std::ios::binary);
    uint64_t total = 0;
    out.write(reinterpret_cast<const char*>(&total), sizeof(total));

    while (!prior_queue.empty())
    {
        FileStream* top = prior_queue.top();
        prior_queue.pop();

        m_serializer->write(out, top->current);
        ++total;

        if (top->remaining > 0)
        {
            T next;
            m_serializer->read(top->stream, next);
            --top->remaining;
            top->current = std::move(next);
            prior_queue.push(top);
        }
    }

    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&total), sizeof(total));

    for (const auto& fname : m_file_to_merge) 
    {
        std::filesystem::remove(fname);
    }
    m_file_to_merge.clear();
    return file_name;
}

template<typename T, typename Compare>
void OutWriter<T, Compare>::write_to_temporary(std::vector<T>&& data)
{
    if (data.empty())
    {
        spdlog::error("Writing empty data is not allowed");
        return;
    }
    std::string file_name = generate_file_name();
    m_file_to_merge.push_back(file_name);
    m_queue->push([file_name = std::move(file_name), data = std::move(data), ser = m_serializer]()
    {
        std::ofstream ofs(fname, std::ios::binary);
        uint64_t size = data.size();
        ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
        for (const auto& item : data) 
        {
            ser->write(ofs, item);
        }
    });
}