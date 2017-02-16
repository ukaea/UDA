#ifndef IDAM_LOGGING_IDAMLOG_H
#define IDAM_LOGGING_IDAMLOG_H

#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define IDAM_LOG(MODE, MSG) idamLog(MODE, "%s:%d >> " MSG, basename(__FILE__), __LINE__)
#define IDAM_LOGF(MODE, FMT, ...) idamLog(MODE, "%s:%d >> " FMT, basename(__FILE__), __LINE__, __VA_ARGS__)

extern int reopen_logs;         // Flags whether or Not Logs need to be Re-Opened

typedef enum LogMode {
    LOG_DEBUG   = 1,
    LOG_INFO    = 2,
    LOG_WARN    = 3,
    LOG_ERROR   = 4,
    LOG_ACCESS  = 5,
    LOG_NONE    = 6
} LOG_MODE;

void idamSetLogLevel(LOG_MODE mode);
LOG_MODE idamGetLogLevel();

void idamCloseLogging();

FILE* idamGetLogFile(LOG_MODE mode);
void idamSetLogFile(LOG_MODE mode, FILE* file_name);

void idamLog(LOG_MODE mode, const char* fmt, ...);

#endif //IDAM_LOGGING_IDAMLOG_H
