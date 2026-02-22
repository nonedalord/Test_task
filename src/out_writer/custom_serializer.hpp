#pragma once
#include "serializer.hpp"
#include "../csv_parser/csv_parser.hpp"

class ParserDataSerializer : public ISerializer<CsvParser::ParserData> 
{
public:
    void write(std::ostream& os, const CsvParser::ParserData& value) override 
    {
        os.write(reinterpret_cast<const char*>(&value.receive_ts), sizeof(value.receive_ts));
        os.write(reinterpret_cast<const char*>(&value.price), sizeof(value.price));
    }

    void read(std::istream& is, CsvParser::ParserData& value) override 
    {
        is.read(reinterpret_cast<char*>(&value.receive_ts), sizeof(value.receive_ts));
        is.read(reinterpret_cast<char*>(&value.price), sizeof(value.price));
    }
};