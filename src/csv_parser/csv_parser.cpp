#include "csv_parser.hpp"
#include "string_counter.hpp"
#include "../logger/logger.hpp"

#include <filesystem>
#include <fstream>
#include <functional>

CsvParser::CsvParser(int max_threads = 4, unsigned long total_space_to_use) : m_queue(std::make_unique<ThreadPoolQueue>()), 
m_ready_data_queue(std::make_unique<ThreadQueue<std::vector<ParserData>>>()),
m_max_threads(max_threads), m_vec_size(total_space_to_use / max_threads / sizeof(ParserData))
{
    m_queue->start_async(m_max_threads);
}

void CsvParser::add_file_to_parse(const std::string& file_name)
{
    if (!check_empty_file(file_name))
    {
        return;
    }
    std::ifstream file(file_name, std::ios::binary);
    m_queue->push(std::bind(parse_csv_data, file_name));
}

void CsvParser::parse_csv_data(const std::string& file_name)
{
    std::vector<ParserData> data;
    data.reserve(m_vec_size);

    std::ifstream file(file_name, std::ios::binary);
    if (!file.is_open()) 
    {
        throw std::runtime_error("Cannot open file: " + file_name);
    }

    std::string line;

    StringCounter line_num;
    ++line_num;
    uint64_t vec_size;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(ss, token, ';')) 
        {
            tokens.push_back(token);
        }

        if (token.size() != 6)
        {
            spdlog::error("File {} have incorrect line {}", file_name, line_num);
            continue;
        }
        try
        {
            uint64_t time = std::stoull(tokens[0]);
            double price = std::stod(tokens[2]);
            if (vec_size == m_vec_size)
            {
                m_ready_data_queue->push(data);
                vec_size = 0;
                data.clear();
            }
            data.push_back({time, price});
            vec_size++;
        }
        catch (const std::exception& err)
        {
            spdlog::error("Error in line {} : {}", line_num, err.what());
            continue;
        }
    }
    if (!data.empty())
    {
        m_ready_data_queue->push(data);
    }
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