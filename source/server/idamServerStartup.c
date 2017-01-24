/*---------------------------------------------------------------
* Open Server Side Log Files
*
* Returns:
*
* Revision 0.0  05-Aug-2004	D.G.Muir
* 0.1	21Mar2007	dgm	accout renamed server_accout
* 0.2   09Jul2007	dgm	debugon enabled
* 23Oct207	dgm	ERRORSTACK components enabled
* 08Nov207	dgm	ENVIRONMENT Structure added
* 08Jul2009	dgm	FILE changed to static FILE and moved to top of idamServer
*--------------------------------------------------------------*/

#include "idamServerStartup.h"

#include <idamLog.h>
#include "getServerEnvironment.h"
#include "idamErrorLog.h"

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
