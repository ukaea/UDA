#include "logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

void uda::logging::uda_init_logging()
{
    auto debug_logger = spdlog::stdout_logger_st("debug");
    auto error_logger = spdlog::stdout_logger_st("error");
    auto access_logger = spdlog::stdout_logger_st("access");
}

void uda::logging::uda_set_log_level(LogLevel level)
{
    spdlog::level::level_enum spdlog_level;

    switch (level) {
        case LogLevel::UDA_LOG_DEBUG:
            spdlog_level = spdlog::level::debug;
            break;
        case LogLevel::UDA_LOG_INFO:
            spdlog_level = spdlog::level::info;
            break;
        case LogLevel::UDA_LOG_WARN:
            spdlog_level = spdlog::level::warn;
            break;
        case LogLevel::UDA_LOG_ERROR:
            spdlog_level = spdlog::level::err;
            break;
        case LogLevel::UDA_LOG_ACCESS:
            spdlog_level = spdlog::level::trace;
            break;
        case LogLevel::UDA_LOG_NONE:
            spdlog_level = spdlog::level::off;
            break;
    }

    spdlog::set_level(spdlog_level);
    spdlog::get("debug")->set_level(spdlog_level);
    spdlog::get("error")->set_level(spdlog_level);
    spdlog::get("access")->set_level(spdlog_level);
}

uda::logging::LogLevel uda::logging::uda_get_log_level()
{
    auto level = spdlog::get_level();
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

    auto& sinks = logger->sinks();

    // remove existing stdout sink
    auto to_remove = std::remove_if(sinks.begin(), sinks.end(), [](const auto& sink){
        return dynamic_cast<spdlog::sinks::stdout_sink_st*>(sink.get()) != nullptr;
    });
    sinks.erase(to_remove, sinks.end());

    // only add file sink if it doesn't exist
    bool found = false;
    for (const auto& sink : sinks) {
        auto file_sink = dynamic_cast<spdlog::sinks::basic_file_sink_st*>(sink.get());
        if (file_sink && file_sink->filename() == file_name) {
            found = true;
            break;
        }
    }
    if (!found) {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>(file_name, truncate));
    }
}
