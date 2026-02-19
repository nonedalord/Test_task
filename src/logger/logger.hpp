#include <spdlog/spdlog.h>
#include <filesystem>

namespace logger_helper
{
    void inti_logger(const std::string& log_name, const int log_max_size, const int log_max_files_cnt, const std::chrono::milliseconds flushing_interval_ms);
    void create_log_dir();
} //namespace logger