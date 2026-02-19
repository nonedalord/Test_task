#include <pthread.h>
#include <signal.h>
#include <cstring>

#include "logger/logger.hpp"

int main()
{
    constexpr std::chrono::milliseconds flushing_interval_ms(1000); 
    logger_helper::create_log_dir();
    logger_helper::inti_logger("logs/csv_parser", 1024 * 1024, 3, flushing_interval_ms);
    spdlog::set_level(spdlog::level::level_enum::info);
    spdlog::info("CSV Parser started!");
    sigset_t mask {};
    int sig{};

    sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);

    if (pthread_sigmask(SIG_BLOCK,&mask, NULL) != 0)
    {
        int err = errno;
        spdlog::error("Error while block thread for signals: {}", strerror(errno));
        return EXIT_FAILURE;
    }

    if (sigwait(&mask, &sig) != 0)
    {
        int err = errno;
        spdlog::error("Signal handling error: {}", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}