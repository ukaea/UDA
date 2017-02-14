#include "idamLog.h"

#include <stdarg.h>

int reopen_logs = 0;        // No need to Re-Open Logs

static FILE* error_log  = NULL;
static FILE* warn_log   = NULL;
static FILE* debug_log  = NULL;
static FILE* info_log   = NULL;
static FILE* access_log = NULL;

static LOG_MODE log_level = LOG_NONE;

void idamSetLogLevel(LOG_MODE mode)
{
    log_level = mode;
}

LOG_MODE idamGetLogLevel()
{
    return log_level;
}

FILE* idamGetLogFile(LOG_MODE mode)
{
    switch (mode) {
        case LOG_ACCESS: return access_log;
        case LOG_ERROR:  return error_log;
        case LOG_WARN:   return warn_log;
        case LOG_INFO:   return info_log;
        case LOG_DEBUG:  return debug_log;
        default:         return NULL;
    }
}

void idamSetLogFile(LOG_MODE mode, FILE* file)
{
    switch (mode) {
        case LOG_ACCESS: access_log = file; break;
        case LOG_ERROR:  error_log  = file; break;
        case LOG_WARN:   warn_log   = file; break;
        case LOG_INFO:   info_log   = file; break;
        case LOG_DEBUG:  debug_log  = file; break;
        default: return; // do nothing
    }
}

void idamLog(LOG_MODE mode, const char * fmt, ...)
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
