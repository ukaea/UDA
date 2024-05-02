#include "logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

void uda::logging::uda_init_logging()
{
    auto debug_logger = std::make_shared<spdlog::logger>("debug");
    auto error_logger = std::make_shared<spdlog::logger>("error");
    auto access_logger = std::make_shared<spdlog::logger>("access");

    spdlog::register_logger(debug_logger);
    spdlog::register_logger(error_logger);
    spdlog::register_logger(access_logger);
}

void uda::logging::uda_set_log_level(LogLevel level)
{
    auto logger = spdlog::default_logger();
    switch (level) {
        case LogLevel::UDA_LOG_DEBUG:
            logger->set_level(spdlog::level::debug);
            break;
        case LogLevel::UDA_LOG_INFO:
            logger->set_level(spdlog::level::info);
            break;
        case LogLevel::UDA_LOG_WARN:
            logger->set_level(spdlog::level::warn);
            break;
        case LogLevel::UDA_LOG_ERROR:
            logger->set_level(spdlog::level::err);
            break;
        case LogLevel::UDA_LOG_ACCESS:
            logger->set_level(spdlog::level::trace);
            break;
        case LogLevel::UDA_LOG_NONE:
            logger->set_level(spdlog::level::off);
            break;
    }
}

uda::logging::LogLevel uda::logging::uda_get_log_level()
{
    auto logger = spdlog::default_logger();
    auto level = logger->level();
    switch (level) {
        case spdlog::level::debug:
            return LogLevel::UDA_LOG_DEBUG;
        case spdlog::level::info:
            return LogLevel::UDA_LOG_INFO;
        case spdlog::level::warn:
            return LogLevel::UDA_LOG_WARN;
        case spdlog::level::err:
            return LogLevel::UDA_LOG_ERROR;
        case spdlog::level::off:
            return LogLevel::UDA_LOG_NONE;
        default:
            return LogLevel::UDA_LOG_ACCESS;
    }
}

void uda::logging::uda_close_logging()
{
    spdlog::shutdown();
}

void uda::logging::uda_set_log_file(LogLevel mode, const std::string& file_name, const std::string& open_mode)
{
    bool truncate = open_mode == "a";

    std::shared_ptr<spdlog::logger> logger;
    switch (mode) {
        case LogLevel::UDA_LOG_DEBUG:
        case LogLevel::UDA_LOG_INFO:
        case LogLevel::UDA_LOG_WARN:
            logger = spdlog::get("debug");
            break;
        case LogLevel::UDA_LOG_ERROR:
            logger = spdlog::get("error");
            break;
        case LogLevel::UDA_LOG_ACCESS:
            logger = spdlog::get("access");
            break;
        case LogLevel::UDA_LOG_NONE:
            return;
    }

    auto sinks = logger->sinks();
    bool found = false;
    for (const auto& sink : sinks) {
        auto file_sink = dynamic_cast<spdlog::sinks::basic_file_sink_st*>(sink.get());
        if (file_sink->filename() == file_name) {
            found = true;
            break;
        }
    }
    if (!found) {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(file_name, truncate));
    }
}
