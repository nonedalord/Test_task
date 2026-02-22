#pragma once

#include "serializer.hpp"

#include <vector>
#include <string>

template<typename T>
class IAlgorithm 
{
public:
    virtual ~IAlgorithm() = default;
    virtual void process_in_memory(std::vector<T>&& sorted_data, const std::string& output_file) = 0;
    virtual void process_file(const std::shared_ptr<ISerializer<T>> serializer, const std::string& sorted_input_file, const std::string& output_file) = 0;
};