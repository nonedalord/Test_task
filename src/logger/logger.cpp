#include "logger.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <memory>
#include <vector>
#include <string>

namespace logger_helper
{
    void inti_logger(const std::string& log_name, const int log_max_size, const int log_max_files_cnt, const std::chrono::milliseconds flushing_interval_ms)
    {
        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_name, log_max_size, log_max_files_cnt));
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
        sinks.push_back(std::make_shared<spdlog::sinks::stderr_sink_mt>());
        sinks.back()->set_level(spdlog::level::err);
        std::shared_ptr<spdlog::logger> logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
        spdlog::set_default_logger(logger);
        spdlog::flush_every(flushing_interval_ms);
    }

    void create_log_dir()
    {
	    std::filesystem::path logs_path = "logs";

	    if (!std::filesystem::exists(logs_path))
	    {
		    std::filesystem::create_directory(logs_path);
	    }
    }
}