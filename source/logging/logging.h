#ifndef UDA_LOGGING_IDAMLOG_H
#define UDA_LOGGING_IDAMLOG_H

#include <stdio.h>
#include <string.h>
#include <clientserver/export.h>

#ifdef _WIN32
#  define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#  define UDA_LOG(LEVEL, FMT, ...) idamLog(LEVEL, "%s:%d >> " FMT, FILENAME, __LINE__, ##__VA_ARGS__)
#else
#  include <libgen.h>
#  define UDA_LOG(LEVEL, FMT, ...) idamLog(LEVEL, "%s:%d >> " FMT, basename((char *)__FILE__), __LINE__, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int reopen_logs;         // Flags whether or Not Logs need to be Re-Opened

typedef enum LogLevel {
    UDA_LOG_DEBUG = 1,
    UDA_LOG_INFO = 2,
    UDA_LOG_WARN = 3,
    UDA_LOG_ERROR = 4,
    UDA_LOG_ACCESS = 5,
    UDA_LOG_NONE = 6
} LOG_LEVEL;

typedef void (* logFunc)(FILE*);

LIBRARY_API void idamSetLogLevel(LOG_LEVEL log_level);
LIBRARY_API LOG_LEVEL idamGetLogLevel();
LIBRARY_API void idamCloseLogging();
LIBRARY_API void idamSetLogFile(LOG_LEVEL mode, FILE* file_name);
LIBRARY_API void idamLogWithFunc(LOG_LEVEL mode, logFunc func);
LIBRARY_API void idamLog(LOG_LEVEL mode, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_IDAMLOG_H
