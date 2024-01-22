#include "serverStartup.h"

#include <cerrno>
#include <cstdlib>
#include <string>

#include <clientserver/errorLog.h>
#include <logging/logging.h>

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
        snprintf(cmd, STRING_LENGTH, "mkdir -p %s 2>/dev/null", environment->logdir);
        if (system(cmd) != 0) {
            UDA_THROW_ERROR(999, "mkdir command failed");
        }

        errno = 0;
        std::string log_file = std::string{environment->logdir} + "Access.log";
        FILE* accout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, "startup", errno, "Access Log: ");
            if (accout != nullptr) {
                fclose(accout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ACCESS, accout);
        }
    }

    if (environment->loglevel <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::string{environment->logdir} + "Error.log";
        FILE* errout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, "startup", errno, "Error Log: ");
            if (errout != nullptr) {
                fclose(errout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ERROR, errout);
        }
    }

    if (environment->loglevel <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::string{environment->logdir} + "DebugServer.log";
        FILE* dbgout = fopen(log_file.c_str(), environment->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, "startup", errno, "Debug Log: ");
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
