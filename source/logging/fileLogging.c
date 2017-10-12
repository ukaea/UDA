#include "logging.h"

#include <stdarg.h>

int reopen_logs = 0;        // No need to Re-Open Logs

static FILE* error_log  = NULL;
static FILE* warn_log   = NULL;
static FILE* debug_log  = NULL;
static FILE* info_log   = NULL;
static FILE* access_log = NULL;

static LOG_LEVEL log_level = UDA_LOG_NONE;

void idamSetLogLevel(LOG_LEVEL level)
{
    log_level = level;
}

LOG_LEVEL idamGetLogLevel()
{
    return log_level;
}

static FILE* idamGetLogFile(LOG_LEVEL mode)
{
    switch (mode) {
        case UDA_LOG_ACCESS: return access_log;
        case UDA_LOG_ERROR:  return error_log;
        case UDA_LOG_WARN:   return warn_log;
        case UDA_LOG_INFO:   return info_log;
        case UDA_LOG_DEBUG:  return debug_log;
        default:             return NULL;
    }
}

void idamSetLogFile(LOG_LEVEL mode, FILE* file)
{
    switch (mode) {
        case UDA_LOG_ACCESS: access_log = file; break;
        case UDA_LOG_ERROR:  error_log  = file; break;
        case UDA_LOG_WARN:   warn_log   = file; break;
        case UDA_LOG_INFO:   info_log   = file; break;
        case UDA_LOG_DEBUG:  debug_log  = file; break;
        default: return; // do nothing
    }
}

void idamLogWithFunc(LOG_LEVEL mode, logFunc func)
{
    FILE* log_file = idamGetLogFile(mode);

    if (mode >= log_level && log_file != NULL) {
        func(log_file);
        fflush(log_file);
    }
}

void idamLog(LOG_LEVEL mode, const char * fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    FILE* log_file = idamGetLogFile(mode);

    if (mode >= log_level && log_file != NULL) {
        vfprintf(log_file, fmt, args);
        fflush(log_file);
    }

    va_end(args);
}

void idamCloseLogging()
{
    if (access_log != NULL) {
        fclose(access_log);
    }
    if (error_log != NULL && error_log != access_log) {
        fclose(error_log);
    }
    if (warn_log != NULL && warn_log != error_log && warn_log != access_log) {
        fclose(warn_log);
    }
    if (debug_log != NULL && debug_log != warn_log && debug_log != error_log && debug_log != access_log) {
        fclose(debug_log);
    }
    if (info_log != NULL && info_log != debug_log && info_log != warn_log && info_log != error_log && info_log != access_log) {
        fclose(info_log);
    }

    access_log = NULL;
    error_log = NULL;
    warn_log = NULL;
    debug_log = NULL;
    info_log = NULL;
}
