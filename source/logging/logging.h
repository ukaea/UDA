#ifndef UDA_LOGGING_IDAMLOG_H
#define UDA_LOGGING_IDAMLOG_H

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define UDA_LOG(LEVEL, FMT, ...) idamLog(LEVEL, "%s:%d >> " FMT, basename(__FILE__), __LINE__, ##__VA_ARGS__)

extern int reopen_logs;         // Flags whether or Not Logs need to be Re-Opened

typedef enum LogLevel {
    UDA_LOG_DEBUG   = 1,
    UDA_LOG_INFO    = 2,
    UDA_LOG_WARN    = 3,
    UDA_LOG_ERROR   = 4,
    UDA_LOG_ACCESS  = 5,
    UDA_LOG_NONE    = 6
} LOG_LEVEL;

typedef void (*logFunc)(FILE*);

void idamSetLogLevel(LOG_LEVEL log_level);

LOG_LEVEL idamGetLogLevel();

void idamCloseLogging();

void idamSetLogFile(LOG_LEVEL mode, FILE* file_name);

void idamLogWithFunc(LOG_LEVEL mode, logFunc func);

void idamLog(LOG_LEVEL mode, const char* fmt, ...);

#endif // UDA_LOGGING_IDAMLOG_H
