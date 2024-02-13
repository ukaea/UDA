#include "logging.h"

#include <cstdarg>

using namespace uda::logging;

int reopen_logs = 0; // No need to Re-Open Logs

static FILE* error_log = nullptr;
static FILE* warn_log = nullptr;
static FILE* debug_log = nullptr;
static FILE* info_log = nullptr;
static FILE* access_log = nullptr;

static LOG_LEVEL log_level = UDA_LOG_NONE;

void uda::logging::udaSetLogLevel(LOG_LEVEL level)
{
    log_level = level;
}

LOG_LEVEL uda::logging::udaGetLogLevel()
{
    return log_level;
}

static FILE* idamGetLogFile(LOG_LEVEL mode)
{
    switch (mode) {
        case UDA_LOG_ACCESS:
            return access_log;
        case UDA_LOG_ERROR:
            return error_log;
        case UDA_LOG_WARN:
            return warn_log;
        case UDA_LOG_INFO:
            return info_log;
        case UDA_LOG_DEBUG:
            return debug_log;
        default:
            return nullptr;
    }
}

void uda::logging::udaSetLogFile(LOG_LEVEL mode, FILE* file)
{
    switch (mode) {
        case UDA_LOG_ACCESS:
            access_log = file;
            break;
        case UDA_LOG_ERROR:
            error_log = file;
            break;
        case UDA_LOG_WARN:
            warn_log = file;
            break;
        case UDA_LOG_INFO:
            info_log = file;
            break;
        case UDA_LOG_DEBUG:
            debug_log = file;
            break;
        default:
            return; // do nothing
    }
}

void uda::logging::udaLogWithFunc(LOG_LEVEL mode, logFunc func)
{
    FILE* log_file = idamGetLogFile(mode);

    if (mode >= log_level && log_file != nullptr) {
        func(log_file);
        fflush(log_file);
    }
}

void uda::logging::udaLog(LOG_LEVEL mode, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    FILE* log_file = idamGetLogFile(mode);

    if (mode >= log_level && log_file != nullptr) {
        vfprintf(log_file, fmt, args);
        fflush(log_file);
    }

    va_end(args);
}

void uda::logging::udaCloseLogging()
{
    if (access_log != nullptr) {
        fclose(access_log);
    }
    if (error_log != nullptr && error_log != access_log) {
        fclose(error_log);
    }
    if (warn_log != nullptr && warn_log != error_log && warn_log != access_log) {
        fclose(warn_log);
    }
    if (debug_log != nullptr && debug_log != warn_log && debug_log != error_log && debug_log != access_log) {
        fclose(debug_log);
    }
    if (info_log != nullptr && info_log != debug_log && info_log != warn_log && info_log != error_log &&
        info_log != access_log) {
        fclose(info_log);
    }

    access_log = nullptr;
    error_log = nullptr;
    warn_log = nullptr;
    debug_log = nullptr;
    info_log = nullptr;
}
