#pragma once

#include "algorithm.hpp"
#include "../csv_parser/csv_parser.hpp"

class MedianAlgorithm : public IAlgorithm<CsvParser::ParserData>
{
public:
    void process_in_memory(std::vector<CsvParser::ParserData>&& sorted_data, const std::string& output_file) override;
    void process_file(const std::shared_ptr<ISerializer<CsvParser::ParserData>> serializer, const std::string& sorted_input_file, const std::string& output_file) override;
private:
    inline static double m_eps = 1e-8;
};