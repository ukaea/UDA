#pragma once

#include <logging/logging.h>

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

#ifdef __cplusplus
extern "C" {
#endif

void openAuthLog(const char* logdir, const char* logmode);
void closeAuthLog();
void auth_log_write(LOG_LEVEL level, const char* file, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

// Write to both auth.log and the regular debug log.
#define AUTH_LOG_BASENAME(p) (strrchr(p, '/') ? strrchr(p, '/') + 1 : (p))
#define AUTH_LOG(LEVEL, FMT, ...) \
    do { \
        UDA_LOG(LEVEL, FMT, ##__VA_ARGS__); \
        if ((LEVEL) >= udaGetLogLevel()) { \
            auth_log_write(LEVEL, AUTH_LOG_BASENAME(__FILE__), __LINE__, FMT, ##__VA_ARGS__); \
        } \
    } while (0)

#else

// auth.log is disabled for fat-client builds and when SSLAUTHENTICATION is not compiled in.
// AUTH_LOG still forwards to the regular debug log so call-sites need no #ifdef guards.
#define AUTH_LOG(LEVEL, FMT, ...) UDA_LOG(LEVEL, FMT, ##__VA_ARGS__)
#define openAuthLog(dir, mode)    ((void)0)
#define closeAuthLog()            ((void)0)

#endif
