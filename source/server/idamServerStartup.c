/*---------------------------------------------------------------
* Open Server Side Log Files
*
* Returns:
*
*--------------------------------------------------------------*/

#include "idamServerStartup.h"

#include <stdlib.h>
#include <errno.h>

#include <logging/idamLog.h>
#include <clientserver/idamErrorLog.h>

#include "getServerEnvironment.h"

int startup(void)
{
    char idamFile[STRING_LENGTH];

//----------------------------------------------------------------
// Read Environment Variable Values (Held in a Global Structure)

    getIdamServerEnvironment(&environment);

//---------------------------------------------------------------
// Open the Log Files

    idamSetLogLevel(environment.loglevel);

    if (environment.loglevel <= LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        sprintf(cmd, "mkdir -p %s 2>/dev/null", environment.logdir);
        system(cmd);

        errno = 0;
        strcpy(idamFile, environment.logdir);
        strcat(idamFile, "Access.log");
        FILE* accout = fopen(idamFile, environment.logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Access Log: ");
            if (accout != NULL) fclose(accout);
        } else {
            idamSetLogFile(LOG_ACCESS, accout);
        }
    }

    if (environment.loglevel <= LOG_ERROR) {
        errno = 0;
        strcpy(idamFile, environment.logdir);
        strcat(idamFile, "Error.log");
        FILE* errout = fopen(idamFile, environment.logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Error Log: ");
            if (errout != NULL) fclose(errout);
        } else {
            idamSetLogFile(LOG_ERROR, errout);
        }
    }

    if (environment.loglevel <= LOG_WARN) {
        errno = 0;
        strcpy(idamFile, environment.logdir);
        strcat(idamFile, "DebugServer.log");
        FILE* dbgout = fopen(idamFile, environment.logmode);

        if (errno != 0) {
            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "startup", errno, "Debug Log: ");
            if (dbgout != NULL) fclose(dbgout);
        } else {
            idamSetLogFile(LOG_WARN, dbgout);
            idamSetLogFile(LOG_DEBUG, dbgout);
            idamSetLogFile(LOG_INFO, dbgout);
        }
    }

    printIdamServerEnvironment(&environment);

    return 0;
}
