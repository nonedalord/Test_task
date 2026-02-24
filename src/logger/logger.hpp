#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/dist_sink.h>
#include <filesystem>
#include <unordered_set>

namespace logger_helper
{
    template<typename Mutex>
    class proxy_dist_sink : public spdlog::sinks::dist_sink<Mutex>
    {
    public:
        proxy_dist_sink() = default;
        explicit proxy_dist_sink(std::initializer_list<spdlog::level::level_enum> allowed_levels) : m_allowed_levels(allowed_levels.begin(), allowed_levels.end()) {}
    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override
        {
            if (m_allowed_levels.empty() || m_allowed_levels.count(msg.level))
            {
                spdlog::sinks::dist_sink<Mutex>::sink_it_(msg);
            }
        }
    private:
        std::unordered_set<spdlog::level::level_enum> m_allowed_levels;
    };

    void init_logger(const std::string& log_name, const int log_max_size, const int log_max_files_cnt, const std::chrono::milliseconds flushing_interval_ms);
    void create_log_dir();
} //namespace logger