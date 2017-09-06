/*---------------------------------------------------------------
* Open Server Side Log Files
*
* Returns:
*
*--------------------------------------------------------------*/

#include "serverStartup.h"

#include <stdlib.h>
#include <errno.h>

#include <logging/logging.h>
#include <clientserver/errorLog.h>

#include "getServerEnvironment.h"

int startup(void)
{
    char idamFile[STRING_LENGTH];

//----------------------------------------------------------------
// Read Environment Variable Values (Held in a Global Structure)

    const ENVIRONMENT* environment = getIdamServerEnvironment();

//---------------------------------------------------------------
// Open the Log Files

    idamSetLogLevel((LOG_LEVEL)environment->loglevel);

    if (environment->loglevel <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        sprintf(cmd, "mkdir -p %s 2>/dev/null", environment->logdir);
        system(cmd);

        errno = 0;
        strcpy(idamFile, environment->logdir);
        strcat(idamFile, "Access.log");
        FILE* accout = fopen(idamFile, environment->logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Access Log: ");
            if (accout != NULL) fclose(accout);
        } else {
            idamSetLogFile(UDA_LOG_ACCESS, accout);
        }
    }

    if (environment->loglevel <= UDA_LOG_ERROR) {
        errno = 0;
        strcpy(idamFile, environment->logdir);
        strcat(idamFile, "Error.log");
        FILE* errout = fopen(idamFile, environment->logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Error Log: ");
            if (errout != NULL) fclose(errout);
        } else {
            idamSetLogFile(UDA_LOG_ERROR, errout);
        }
    }

    if (environment->loglevel <= UDA_LOG_WARN) {
        errno = 0;
        strcpy(idamFile, environment->logdir);
        strcat(idamFile, "DebugServer.log");
        FILE* dbgout = fopen(idamFile, environment->logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Debug Log: ");
            if (dbgout != NULL) fclose(dbgout);
        } else {
            idamSetLogFile(UDA_LOG_WARN, dbgout);
            idamSetLogFile(UDA_LOG_DEBUG, dbgout);
            idamSetLogFile(UDA_LOG_INFO, dbgout);
        }
    }

    printIdamServerEnvironment(environment);

    return 0;
}
