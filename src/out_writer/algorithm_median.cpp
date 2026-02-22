#include "algorithm_median.hpp"
#include "../logger/logger.hpp"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <fstream>
#include <iomanip>
#include <cmath>

void MedianAlgorithm::process_in_memory(std::vector<CsvParser::ParserData>&& sorted_data, const std::string& output_file)
{
    using namespace boost::accumulators;

    accumulator_set<double, stats<tag::median>> acc;

    std::ofstream out(output_file);
    if (!out.is_open()) 
    {
        throw std::runtime_error("Cannot create output file: " + output_file);
    }

    out << std::fixed << std::setprecision(8);

    double last_median = 0.0;
    bool first = true;

    for (const auto& record : sorted_data)
    {
        acc(record.price);
        double current_median = median(acc);

        if (first || std::fabs(current_median - last_median) > m_eps)
        {
            out << record.receive_ts << ";" << current_median << "\n";
            last_median = current_median;
            first = false;
        }
    }
}

void MedianAlgorithm::process_file(const std::shared_ptr<ISerializer<CsvParser::ParserData>> serializer, const std::string& sorted_input_file, const std::string& output_file)
{
    using namespace boost::accumulators;

    accumulator_set<double, stats<tag::median>> acc;

    std::ifstream in(sorted_input_file, std::ios::binary);
    if (!in.is_open()) 
    {
        throw std::runtime_error("Cannot open input file: " + sorted_input_file);
    }

    std::ofstream out(output_file);
    if (!out.is_open()) 
    {
        throw std::runtime_error("Cannot create output file: " + output_file);
    }

    out << std::fixed << std::setprecision(8);
    double last_median = 0.0;
    bool first = true;

    CsvParser::ParserData record;
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
            spdlog::error("Read error at position {}", pos);
            break;
        }

        acc(record.price);
        double current_median = median(acc);

        if (first || std::fabs(current_median - last_median) > m_eps) 
        {
            out << record.receive_ts << ";" << current_median << "\n";
            last_median = current_median;
            first = false;
        }
   }
}