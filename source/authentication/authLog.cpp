#include "authLog.h"

// Only compile the auth log implementation when SSLAUTHENTICATION is active and
// this is not a fat-client build.  In the other cases authLog.h provides no-op
// macros so call-sites still compile without #ifdef guards.
#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>

#ifndef _WIN32
#  include <sys/time.h>
#endif

static FILE* g_auth_log = nullptr;

void openAuthLog(const char* logdir, const char* logmode)
{
    if (logdir == nullptr || logdir[0] == '\0') {
        return;
    }
    std::string path = std::string(logdir) + "auth.log";
    g_auth_log = fopen(path.c_str(), (logmode != nullptr && logmode[0] != '\0') ? logmode : "w");
}

void closeAuthLog()
{
    if (g_auth_log != nullptr) {
        fclose(g_auth_log);
        g_auth_log = nullptr;
    }
}

void auth_log_write(LOG_LEVEL level, const char* file, int line, const char* fmt, ...)
{
    if (g_auth_log == nullptr) {
        return;
    }

    const char* level_str;
    switch (level) {
        case UDA_LOG_ERROR:  level_str = "ERROR"; break;
        case UDA_LOG_WARN:   level_str = "WARN";  break;
        case UDA_LOG_INFO:   level_str = "INFO";  break;
        case UDA_LOG_DEBUG:  level_str = "DEBUG"; break;
        default:             level_str = "?";     break;
    }

#ifndef _WIN32
    struct timeval tv = {};
    gettimeofday(&tv, nullptr);
    struct tm* tm_info = localtime(&tv.tv_sec);
    char tbuf[32] = {};
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%dT%H:%M:%S", tm_info);
    fprintf(g_auth_log, "%s.%06dZ [%s] %s:%d >> ", tbuf, (int)tv.tv_usec, level_str, file, line);
#else
    fprintf(g_auth_log, "[%s] %s:%d >> ", level_str, file, line);
#endif

    va_list args;
    va_start(args, fmt);
    vfprintf(g_auth_log, fmt, args);
    va_end(args);

    fflush(g_auth_log);
}

#endif // defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
