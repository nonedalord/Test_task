#include "algorithm_median.hpp"
#include "../logger/logger.hpp"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include <fstream>
#include <iomanip>
#include <cmath>
#include <queue>
#include <vector>

void MedianAlgorithm::process_in_memory(std::vector<CsvParser::ParserData>&& sorted_data, const std::string& output_file)
{
    std::filesystem::path out_path(output_file);
    std::filesystem::create_directories(out_path.parent_path());

    std::ofstream out(output_file);
    if (!out.is_open()) 
    {
        throw std::runtime_error("Cannot create output file: " + output_file);
    }

    out << "receive_ts;price_median\n"; 
    out << std::fixed << std::setprecision(8);

    std::priority_queue<double> max_heap;
    std::priority_queue<double, std::vector<double>, std::greater<double>> min_heap;

    bool first = true;
    double last_median = 0;
    spdlog::info("Started finding median(in memory)");

    for (const auto& data : sorted_data)
    {
        if (max_heap.empty() || data.price <= max_heap.top())
        {
            max_heap.push(data.price);
        }
        else
        {
            min_heap.push(data.price);
        }

        if (max_heap.size() > min_heap.size() + 1) 
        {
            min_heap.push(max_heap.top());
            max_heap.pop();
        } 
        else if (min_heap.size() > max_heap.size()) 
        {
            max_heap.push(min_heap.top());
            min_heap.pop();
        }

        double current_median;
        if (max_heap.size() == min_heap.size())
        {
            current_median = (max_heap.top() + min_heap.top()) / 2.0;
        }
        else
        {
            current_median = max_heap.top();
        }

        if (first || std::fabs(current_median - last_median) > m_eps) 
        {
            out << data.receive_ts << ";" << current_median << "\n";
            last_median = current_median;
            first = false;
        }
    }
    spdlog::info("Results are written to a file {}", output_file);
}

void MedianAlgorithm::process_file(const std::shared_ptr<ISerializer<CsvParser::ParserData>> serializer, const std::string& sorted_input_file, const std::string& output_file)
{
    using namespace boost::accumulators;

    accumulator_set<double, stats<tag::median>> acc;
    std::vector<double> buffer;
    constexpr size_t buffer_size = 5;
    buffer.reserve(buffer_size);

    std::ifstream in(sorted_input_file, std::ios::binary);
    if (!in.is_open())
    {
        delete_process_file(sorted_input_file);
        throw std::runtime_error("Cannot open input file: " + sorted_input_file);
    }

    uint64_t total_elements = 0;
    in.read(reinterpret_cast<char*>(&total_elements), sizeof(total_elements));

    if (total_elements == 0)
    {
        delete_process_file(sorted_input_file);
        throw std::runtime_error("Input file " + sorted_input_file + " is empty");
    }
    
    std::filesystem::path out_path(output_file);
    std::filesystem::create_directories(out_path.parent_path());
    std::ofstream out(output_file);
    if (!out.is_open())
    {
        delete_process_file(sorted_input_file);
        throw std::runtime_error("Cannot create output file: " + output_file);
    }

    out << "receive_ts;price_median\n" << std::fixed << std::setprecision(8);

    bool first = true;
    double last_median = 0.0;
    spdlog::info("Started finding median for file {}", sorted_input_file);

    CsvParser::ParserData record;

    for (size_t i = 0; i < buffer_size; ++i)
    {
        serializer->read(in, record);
        if (!in)
        {
            if (in.eof())
            {
                break;
            }
            std::streamoff pos = in.tellg();
            spdlog::info("Results are written to a file {}", output_file);
            delete_process_file(sorted_input_file);
            throw std::runtime_error("Read error at position: " + pos);
        }

        acc(record.price);
        buffer.push_back(record.price);
        std::sort(buffer.begin(), buffer.end());

        size_t n = buffer.size();
        double current_median;
        if (n % 2 == 1)
        {
            current_median = buffer[n / 2];
        }
        else
        {
            current_median = (buffer[n / 2 - 1] + buffer[n / 2]) / 2.0;
        }

        if (first || std::fabs(current_median - last_median) > m_eps)
        {
            out << record.receive_ts << ";" << current_median << "\n";
            last_median = current_median;
            first = false;
        }
    }

    if (in.eof() && buffer.size() <= buffer_size)
    {
        return;
    }

    while (true)
    {
        serializer->read(in, record);
        if (!in)
        {
            if (in.eof()) 
            {
                break;
            }
            std::streamoff pos = in.tellg();
            spdlog::info("Results are written to a file {}", output_file);
            delete_process_file(sorted_input_file);
            throw std::runtime_error("Read error at position: " + pos);
        }

        acc(record.price);
        double current_median = median(acc);

        if (std::fabs(current_median - last_median) > m_eps)
        {
            out << record.receive_ts << ";" << current_median << "\n";
            last_median = current_median;
        }
    }
    delete_process_file(sorted_input_file);
    spdlog::info("Results are written to a file {}", output_file);
}

void MedianAlgorithm::delete_process_file(const std::string& file_name)
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