#include "../logger/logger.hpp"

#include <algorithm>
#include <queue>
#include <string_view>
#include <atomic>

namespace
{
    constexpr uint64_t binary_hash = 0x12345678;

    inline std::string generate_file_name() 
    {
        static std::string base_name = "binary_data";
        static std::atomic<int> counter{0};
        return base_name + "_" + std::to_string(binary_hash) + "_" + std::to_string(counter++) + ".bin";
    }
} //anonymous namespace

template <typename T, typename Compare>
OutWriter<T, Compare>::OutWriter(uint64_t max_elements, std::shared_ptr<ISerializer<T>> serializer, std::unique_ptr<IAlgorithm<T>> algorithm, Compare comp) : 
m_serializer(serializer), m_algorithm(std::move(algorithm)), m_queue(std::make_unique<ThreadPoolQueue>()), m_comp(comp), m_max_elements(max_elements) 
{
    m_buff.reserve(m_max_elements);
    m_queue->start_async(4);
    spdlog::debug("OutWriter created");
}

template <typename T, typename Compare>
OutWriter<T, Compare>::~OutWriter()
{
    m_queue->stop();
    spdlog::debug("OutWriter destroyed");
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
            spdlog::info("In memory model was chosen");
            std::ranges::sort(m_buff, m_comp);
            try
            {
                m_algorithm->process_in_memory(std::move(m_buff), file_name);
            }
            catch (const std::exception& err)
            {
                spdlog::error("Error occurred while running the algorithm: {}", err.what());
            }
        }
        else
        {
            spdlog::error("Empty data, can't find median");
            return;
        }
    }
    else
    {
        spdlog::info("File model was chosen");
        std::ranges::sort(m_buff, m_comp);
        write_to_temporary(std::move(m_buff));
        m_buff.clear();
        std::string out_put_file = merge_sort();
        if (out_put_file.empty())
        {
            return;
        }
        try 
        {
            m_algorithm->process_file(m_serializer, merge_sort(), file_name);
        }
        catch (const std::exception& err)
        {
            spdlog::error("Error occurred while running the algorithm: {}", err.what());
        }
    }
}

template<typename T, typename Compare>
std::string OutWriter<T, Compare>::merge_sort()
{
    spdlog::debug("Stated to merge files");
    std::vector<FileStream> streams;
    streams.reserve(m_file_to_merge.size());

    for (const auto& file_name : m_file_to_merge)
    {
        std::ifstream ifs(file_name, std::ios::binary);
        if (!ifs.is_open())
        {
            spdlog::error("Cannot open temporary file {}", file_name);
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
        return std::string();
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

    for (const auto& file_name : m_file_to_merge) 
    {
        try 
        {
            std::filesystem::remove(file_name);
            spdlog::debug("Temporary file removed {}", file_name);
        }
        catch(const std::exception& err)
        {
            spdlog::warn("Error while deleting temporary file {}", file_name);
        }
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
    
    {
        std::unique_lock lock(m_mutex);
        m_file_to_merge.push_back(file_name);
    }
    m_queue->push([file_name = std::move(file_name), data = std::move(data), ser = m_serializer]()
    {
        std::ofstream ofs(file_name, std::ios::binary);
        uint64_t size = data.size();
        ofs.write(reinterpret_cast<const char*>(&size), sizeof(size));
        for (const auto& item : data) 
        {
            ser->write(ofs, item);
        }
        spdlog::debug("Created temporary file: {}", file_name);
    });
}