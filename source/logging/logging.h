#pragma once

#include <stdio.h>
#include <string.h>
#include <string>
#include <spdlog/spdlog.h>

#define UDA_LOG(LEVEL, FMT, ...) uda::logging::log(LEVEL, __FILE__, __LINE__, FMT, ##__VA_ARGS__);

namespace uda::logging
{

enum LogLevel {
    UDA_LOG_DEBUG = 1,
    UDA_LOG_INFO = 2,
    UDA_LOG_WARN = 3,
    UDA_LOG_ERROR = 4,
    UDA_LOG_ACCESS = 5,
    UDA_LOG_NONE = 6
};

typedef void (*logFunc)(FILE*);

void init_logging();

void set_log_level(LogLevel level);

LogLevel get_log_level();

void close_logging();

void set_log_file(LogLevel mode, const std::string& file_name, const std::string& open_mode);

template<typename... Args>
void log(LogLevel mode, const char* file, int line, const std::string& fmt, Args &&...args)
{
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

    spdlog::level::level_enum level;
    switch (mode) {
        case LogLevel::UDA_LOG_DEBUG:
            level = spdlog::level::debug;
            break;
        case LogLevel::UDA_LOG_INFO:
            level = spdlog::level::info;
            break;
        case LogLevel::UDA_LOG_WARN:
            level = spdlog::level::warn;
            break;
        case LogLevel::UDA_LOG_ERROR:
            level = spdlog::level::err;
            break;
        case LogLevel::UDA_LOG_ACCESS:
            level = spdlog::level::trace;
            break;
        default:
            level = spdlog::level::off;
            break;
    }

    spdlog::source_loc loc{file, line, ""};

    logger->log(loc, level, fmt, args...);
    logger->flush();
}

} // namespace uda::logging
