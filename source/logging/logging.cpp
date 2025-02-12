#include "logging.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/ostream_sink.h>
#include <ostream>

//TODO: should this global sit in the logging (or private) namespace or have a more logging-specific name instead of what it is currently?
static bool g_initialised = false;

void uda::logging::init_logging()
{
    if (g_initialised) {
        return;
    }
    auto debug_logger = spdlog::stdout_logger_st("debug");
    auto error_logger = spdlog::stdout_logger_st("error");
    auto access_logger = spdlog::stdout_logger_st("access");
    // default logger required to avoid segfaults if logging is reopened after shutdown
    // e.g. in calls to spdlog::get_level()
    spdlog::set_default_logger(debug_logger);
    g_initialised = true;
}

bool uda::logging::logging_initialised()
{
    return g_initialised;
}

void uda::logging::set_log_level(LogLevel level)
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
            spdlog_level = spdlog::level::critical;
            break;
        case LogLevel::UDA_LOG_NONE:
            spdlog_level = spdlog::level::off;
            break;
    }

    //TODO: consider calling get and set separately to catch nullptr errors
    spdlog::set_level(spdlog_level);
    spdlog::get("debug")->set_level(spdlog_level);
    spdlog::get("error")->set_level(spdlog_level);
    spdlog::get("access")->set_level(spdlog_level);
}

uda::logging::LogLevel uda::logging::get_log_level()
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
        case spdlog::level::critical:
            return LogLevel::UDA_LOG_ACCESS;
        default:
            return LogLevel::UDA_LOG_NONE;
    }
}

void uda::logging::close_logging()
{
    spdlog::shutdown();
    spdlog::drop_all();
    g_initialised = false;
}

namespace {

std::shared_ptr<spdlog::logger> get_logger(uda::logging::LogLevel mode)
{
    std::shared_ptr<spdlog::logger> logger;
    using uda::logging::LogLevel;
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
            break;
    }
    return logger;
}

}

void uda::logging::set_log_stdout(LogLevel mode)
{
    std::shared_ptr<spdlog::logger> logger = get_logger(mode);
    if (!logger) {
        return;
    }

    auto& sinks = logger->sinks();

    // only add file sink if it doesn't exist
    bool found = false;
    for (const auto& sink : sinks) {
        auto stdout_sink = dynamic_cast<spdlog::sinks::stdout_sink_st*>(sink.get());
        if (stdout_sink) {
            found = true;
            break;
        }
    }
    if (!found) {
        sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    }
}

void uda::logging::set_log_file(LogLevel mode, const std::string& file_name, const std::string& open_mode)
{
    bool truncate = open_mode != "a";

    std::shared_ptr<spdlog::logger> logger = get_logger(mode);
    if (!logger) {
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

// note: only used for testing. defined here to access the get_logger function
// TODO: check there's not a better way (e.g. copy get_logger into the test file with this function and remove this function from here)
void uda::logging::capture_log_output(LogLevel mode, std::ostream& oss)
{
    std::shared_ptr<spdlog::logger> logger = get_logger(mode);
    if (!logger) {
        return;
    }

    auto& sinks = logger->sinks();
    sinks.clear();
    sinks.push_back(std::make_shared<spdlog::sinks::ostream_sink_st>(oss));
}

