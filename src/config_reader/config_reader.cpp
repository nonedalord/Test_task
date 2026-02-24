#include "config_reader.hpp"

#include <toml++/toml.h>

#include <stdexcept>
#include <iostream>

ConfigReader::Config ConfigReader::load_from_file(const std::filesystem::path& file_path) 
{
    if (!std::filesystem::exists(file_path)) 
    {
        throw std::runtime_error("Config file not found: " + file_path.string());
    }
    toml::parse_result result;
    try 
    {
        result = toml::parse_file(file_path.string());
    } 
    catch (const toml::parse_error& err) 
    {
        std::ostringstream oss;
        oss << "Failed to parse TOML: " << err.description();
        if (err.source().path) 
        {
            oss << " in file " << *err.source().path;
        }
        oss << " at line " << err.source().begin.line << ", column " << err.source().begin.column;
        throw std::runtime_error(oss.str());
    }

    if (!result.contains("main")) 
    {
        throw std::runtime_error("Missing required table [main] in config file");
    }

    auto main_table = result["main"];

    Config cfg;

    if (auto input_str = main_table["input"].value<std::string>()) 
    {
        cfg.input = std::filesystem::path(*input_str);
    } 
    else 
    {
        throw std::runtime_error("Missing or invalid 'input' field in [main] (must be string)");
    }

    if (auto output_str = main_table["output"].value<std::string>()) 
    {
        cfg.output = std::filesystem::path(*output_str);
    } 
    else 
    {
        cfg.output = std::filesystem::path("./output");
    }

    if (auto mask_array = main_table["filename_mask"].as_array()) 
    {
        for (auto&& elem : *mask_array) 
        {
            if (auto mask = elem.value<std::string>()) 
            {
                cfg.filename_mask.push_back(*mask);
            } 
            else 
            {
                throw std::runtime_error("All elements of 'filename_mask' must be strings");
            }
        }
    }

    return cfg;
}


std::vector<std::filesystem::path> ConfigReader::find_files(const Config& cfg) 
{
    std::vector<std::filesystem::path> result;

    if (!std::filesystem::exists(cfg.input) || !std::filesystem::is_directory(cfg.input)) 
    {
        throw std::runtime_error("Input directory does not exist or is not a directory: " + cfg.input.string());
    }

    std::vector<std::filesystem::path> csv_files;
    for (const auto& entry : std::filesystem::directory_iterator(cfg.input)) 
    {
        if (entry.is_regular_file() && entry.path().extension() == ".csv") 
        {
            csv_files.push_back(entry.path());
        }
    }

    if (cfg.filename_mask.empty()) 
    {
        return csv_files;
    }

    for (const auto& file : csv_files) 
    {
        std::string filename = file.filename().string();
        for (const auto& mask : cfg.filename_mask) 
        {
            if (filename.find(mask) != std::string::npos) 
            {
                result.push_back(file);
                break;
            }
        }
    }

    return result;
}