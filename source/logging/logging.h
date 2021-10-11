#ifndef UDA_LOGGING_IDAMLOG_H
#define UDA_LOGGING_IDAMLOG_H

#include <stdio.h>
#include <string.h>
#include <clientserver/export.h>

#ifdef _WIN32
#  define FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#  define UDA_LOG(LEVEL, FMT, ...) udaLog(LEVEL, "%s:%d >> " FMT, FILENAME, __LINE__, ##__VA_ARGS__)
#else
#  include <libgen.h>
#  include <ctime>
#  include <sys/time.h>
#  include <cstdint>
#  define UIX_DEFINETIME \
        struct timeval uix_tmnow; \
        struct tm* uix_tm;        \
        char uix_buf[30];\
        gettimeofday(&uix_tmnow, nullptr); \
        uix_tm = localtime(&uix_tmnow.tv_sec); \
        strftime(uix_buf, 30, "%Y:%m:%dT%H:%M:%S", uix_tm);
#  define uix_printtime() { \
            UIX_DEFINETIME \
            printf("%s.%dZ, ", uix_buf,(int32_t)uix_tmnow.tv_usec); \
        }
#  define UDA_LOG(LEVEL, FMT, ...) do{ UIX_DEFINETIME udaLog(LEVEL, "%s.%dZ, %s:%d >> " FMT, uix_buf, (int32_t)uix_tmnow.tv_usec, basename((char *)__FILE__), __LINE__, ##__VA_ARGS__); } while(0)
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

LIBRARY_API void udaSetLogLevel(LOG_LEVEL level);
LIBRARY_API LOG_LEVEL udaGetLogLevel();
LIBRARY_API void udaCloseLogging();
LIBRARY_API void udaSetLogFile(LOG_LEVEL mode, FILE* file);
LIBRARY_API void udaLogWithFunc(LOG_LEVEL mode, logFunc func);
LIBRARY_API void udaLog(LOG_LEVEL mode, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_IDAMLOG_H
