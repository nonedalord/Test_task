#pragma once

#include <string>
#include <vector>
#include <filesystem>

class ConfigReader 
{
public:

    struct Config 
    {
        std::filesystem::path input;
        std::filesystem::path output;
        std::vector<std::string> filename_mask;
    };

    static Config load_from_file(const std::filesystem::path& filepath);
    static std::vector<std::filesystem::path> find_files(const Config& cfg);
};