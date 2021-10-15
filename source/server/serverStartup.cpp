#include "serverStartup.h"

#include <cstdlib>
#include <cerrno>
#include <string>

#include <logging/logging.h>
#include <clientserver/errorLog.h>

#include "getServerEnvironment.h"

int startup()
{
    //----------------------------------------------------------------
    // Read Environment Variable Values (Held in a Global Structure)

    const ENVIRONMENT* environment = getServerEnvironment();

    //---------------------------------------------------------------
    // Open the Log Files

    udaSetLogLevel((LOG_LEVEL)environment->loglevel);

    if (environment->loglevel <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        sprintf(cmd, "mkdir -p %s 2>/dev/null", environment->logdir);
        system(cmd);

        errno = 0;
        std::string log_file = std::string{ environment->logdir } + "Access.log";
        FILE* accout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(SYSTEMERRORTYPE, "startup", errno, "Access Log: ");
            if (accout != nullptr) {
                fclose(accout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ACCESS, accout);
        }
    }

    if (environment->loglevel <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::string{ environment->logdir } + "Error.log";
        FILE* errout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(SYSTEMERRORTYPE, "startup", errno, "Error Log: ");
            if (errout != nullptr) {
                fclose(errout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ERROR, errout);
        }
    }

    if (environment->loglevel <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::string{ environment->logdir } + "DebugServer.log";
        FILE* dbgout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(SYSTEMERRORTYPE, "startup", errno, "Debug Log: ");
            if (dbgout != nullptr) {
                fclose(dbgout);
            }
        } else {
            udaSetLogFile(UDA_LOG_WARN, dbgout);
            udaSetLogFile(UDA_LOG_DEBUG, dbgout);
            udaSetLogFile(UDA_LOG_INFO, dbgout);
        }
    }

    printServerEnvironment(environment);

    return 0;
}
