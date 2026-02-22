#include "csv_parser.hpp"
#include "../logger/logger.hpp"

#include <filesystem>
#include <fstream>
#include <functional>

CsvParser::CsvParser(u_int32_t max_threads = 4, u_int64_t total_space_to_use) : m_queue(std::make_unique<ThreadPoolQueue>()), 
m_ready_data_queue(std::make_unique<ThreadQueue<std::vector<ParserData>>>()), m_total_task(0),
m_vec_size(total_space_to_use / max_threads / sizeof(ParserData)), m_max_elements(total_space_to_use / sizeof(ParserData)), m_max_threads(max_threads)
{
    m_queue->start_async(m_max_threads);
}

CsvParser::~CsvParser()
{
    m_total_task.store(0, std::memory_order_acq_rel);
    m_cv_wait_task.notify_all();
    if (m_task_wait_thread.joinable())
    {
        m_task_wait_thread.join();
    }
    m_queue->stop();
    m_ready_data_queue->delete_queue();
}

void CsvParser::wait_task_done()
{
    m_task_wait_thread = std::thread([this] {
        std::unique_lock<std::mutex> lock(m_wait_task_mutex);
        m_cv_wait_task.wait(lock, [this]
        {
            return m_total_task.load(std::memory_order_acq_rel) == 0;
        });
        m_queue->stop();
    });
}

void CsvParser::add_file_to_parse(const std::string& file_name)
{
    if (!check_empty_file(file_name))
    {
        return;
    }
    m_queue->push(std::bind(parse_csv_data, file_name));
    ++m_total_task;
}

void CsvParser::parse_csv_data(const std::string& file_name)
{
    std::vector<ParserData> data {};
    data.reserve(m_vec_size);

    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) 
    {
        notify_task();
        spdlog::error("Cannot open file: {}", file_name);
        return;
    }

    std::string line {};

    u_int64_t line_num = 0;
    while (std::getline(file, line))
    {
        ++line_num;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ';')) 
        {
            tokens.push_back(token);
        }

        if (tokens.size() != 6)
        {
            spdlog::error("File {} have incorrect line {}", file_name, line_num);
            continue;
        }
        try
        {
            uint64_t time = std::stoull(tokens[0]);
            double price = std::stod(tokens[2]);
            if (data.size() == m_vec_size)
            {
                m_ready_data_queue->push(std::move(data));
                data.clear();
            }
            data.push_back({time, price});
        }
        catch (const std::exception& err)
        {
            spdlog::error("Error in line {} : {}", line_num, err.what());
            continue;
        }
    }
    if (!data.empty())
    {
        m_ready_data_queue->push(std::move(data));
    }
    notify_task();
}

void CsvParser::notify_task()
{
    m_total_task.fetch_sub(1, std::memory_order_relaxed);
    m_cv_wait_task.notify_all();
}

std::optional<std::vector<CsvParser::ParserData>> CsvParser::get_ready_data()
{
    std::vector<ParserData> data;
    if (m_ready_data_queue->front(data))
    {
        return data;
    }
    return std::nullopt;
}

bool CsvParser::check_empty_file(const std::string& file_path)
{
    if (std::filesystem::is_empty(file_path))
    {
        spdlog::error("File {} is empty", file_path);
        return false;
    }
    return true;
}