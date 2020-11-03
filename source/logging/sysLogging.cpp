#include "logging.h"

#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>

static LOG_LEVEL log_level = UDA_LOG_NONE;

int reopen_logs = 0;        // No need to Re-Open Logs

void udaSetLogLevel(LOG_LEVEL level)
{
    log_level = level;
}

LOG_LEVEL udaGetLogLevel()
{
    return log_level;
}

void udaCloseLogging()
{
    closelog();
}

void udaSetLogFile(LOG_LEVEL mode, FILE* file)
{
    openlog("uda", 0, 0);
}

static int syslogPriority(LOG_LEVEL log_mode)
{
    switch (log_mode) {
        case UDA_LOG_ACCESS: return LOG_ALERT;
        case UDA_LOG_ERROR:  return LOG_ERR;
        case UDA_LOG_WARN:   return LOG_WARNING;
        case UDA_LOG_INFO:   return LOG_INFO;
        case UDA_LOG_DEBUG:  return LOG_DEBUG;
        default:             return LOG_EMERG;
    }
}

void udaLogWithFunc(LOG_LEVEL mode, logFunc func)
{
    char tmpFileName[] = "UdaTempLogXXXXXX";

    mkstemp(tmpFileName);
    FILE* tmpFile = fopen(tmpFileName, "w");
    func(tmpFile);
    fseek(tmpFile, 0, SEEK_SET);

    char* line = nullptr;
    size_t n = 0;

    while (getline(&line, &n, tmpFile) != EOF) {
        udaLog(mode, "%s", line);
    }

    free(line);
    fclose(tmpFile);
}

void udaLog(LOG_LEVEL mode, const char* fmt, ...)
{
    if (mode >= log_level) {
        int priority = syslogPriority(mode);

        va_list args;
        va_start(args, fmt);
        vsyslog(priority, fmt, args);
        va_end(args);
    }
}