#include "logger.hpp"

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <mutex>
#include <memory>
#include <vector>
#include <string>

namespace logger_helper
{
    void init_logger(const std::string& log_name, const int log_max_size, const int log_max_files_cnt, const std::chrono::milliseconds flushing_interval_ms)
    {
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_name, log_max_size, log_max_files_cnt);

        auto stdout_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();

        auto stderr_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();

        std::initializer_list<spdlog::level::level_enum> stdout_sink_filter{
            spdlog::level::trace,
            spdlog::level::debug,
            spdlog::level::info
        };
        auto stdout_proxy = std::make_shared<proxy_dist_sink<std::mutex>>(stdout_sink_filter);
        stdout_proxy->add_sink(stdout_sink);

        std::initializer_list<spdlog::level::level_enum> stderr_sink_filter{
            spdlog::level::warn,
            spdlog::level::err,
            spdlog::level::critical
        };

        auto stderr_proxy = std::make_shared<proxy_dist_sink<std::mutex>>(stderr_sink_filter);
        stderr_proxy->add_sink(stderr_sink);

        auto file_proxy = std::make_shared<proxy_dist_sink<std::mutex>>();
        file_proxy->add_sink(file_sink);

        std::vector<spdlog::sink_ptr> proxies = {file_proxy, stdout_proxy, stderr_proxy};
        auto logger = std::make_shared<spdlog::logger>("logger", proxies.begin(), proxies.end());

        logger->set_level(spdlog::level::trace);

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