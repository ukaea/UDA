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

void set_log_stdout(LogLevel mode);

void set_log_file(LogLevel mode, const std::string& file_name, const std::string& open_mode);

template<typename... Args>
void log(LogLevel mode, const char* file, int line, const std::string_view fmt, Args &&...args)
{
    auto log_level = spdlog::get_level();
    if (log_level == spdlog::level::off) {
        return;
    }

    std::shared_ptr<spdlog::logger> logger;
    spdlog::level::level_enum level;

    switch (mode) {
        case LogLevel::UDA_LOG_DEBUG:
            level = spdlog::level::debug;
            logger = spdlog::get("debug");
            break;
        case LogLevel::UDA_LOG_INFO:
            level = spdlog::level::info;
            logger = spdlog::get("debug");
            break;
        case LogLevel::UDA_LOG_WARN:
            level = spdlog::level::warn;
            logger = spdlog::get("debug");
            break;
        case LogLevel::UDA_LOG_ERROR:
            level = spdlog::level::err;
            logger = spdlog::get("error");
            break;
        case LogLevel::UDA_LOG_ACCESS:
            level = spdlog::level::critical;
            logger = spdlog::get("access");
            break;
        case LogLevel::UDA_LOG_NONE:
            return;
    }

    spdlog::source_loc loc{file, line, ""};

    if (!logger) {
        throw std::runtime_error{ "logging not configured" };
    }

    logger->log(loc, level, fmt, args...);
    if (mode == LogLevel::UDA_LOG_ERROR) {
        spdlog::get("debug")->log(loc, level, fmt, args...);
    }
//    logger->flush();
}

} // namespace uda::logging
