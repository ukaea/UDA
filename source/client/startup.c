//! $LastChangedRevision: 290 $
//! $LastChangedDate: 2012-02-06 15:19:02 +0000 (Mon, 06 Feb 2012) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/startup.c $

/*---------------------------------------------------------------
*  Open Client Side Log Files
*
* Argument: reset => if 1 then always reopen Files
*
* Checks for an Environment var (IDAM_LOG) for the directory of the log file
* otherwise it assumes the file is to be local to the PWD.
*
* The directory named must not have a trailing forward slash.
*
* It is assumed that the directory specified by IDAM_LOG already exits
*
* Returns:
*
* Change History
*
* 05Aug2004	D.G.Muir
* 04Aug2006	dgm	Client Side version Only: Uses global environment
*			variable values
* 17Apr2007	dgm	X.509 Digital Security Cerification added
* 24Oct2007	dgm	ERRORSTACK components added
* 08Jan2008	dgm	fflush(NULL) changed to individual files as this can have unexpected side effects
* 08Jul2009	dgm	dbgout, errout, reopen_logs changed to static class
* 11Nov2010	dgm	Copy environment values for clientFlags and altRank to global variables as initial values
*--------------------------------------------------------------*/

#include "startup.h"

#include "idamLog.h"
#include "idamclientserver.h"
#include "idamclient.h"
#include "idamErrorLog.h"
#include "getEnvironment.h"

int reopen_logs = 0;        // No need to Re-Open Logs

#ifdef FATCLIENT
int idamStartupFat(int reset) {
#else

int idamStartup(int reset)
{
#endif

    static int start_status = 0;

//---------------------------------------------------------------
// Are the Files Already Open?

    if (start_status && !reset && !reopen_logs) return (0);

//----------------------------------------------------------------
// Read Environment Variable Values (Held in a Global Structure)

    if (!start_status) getIdamClientEnvironment(&environment);

    printIdamClientEnvironment(&environment);

//----------------------------------------------------------------
// Client set Property Flags (can be changed via property accessor functions)
// Coded user properties changes have priority

    if (environment.clientFlags != 0) {
        clientFlags = clientFlags | environment.clientFlags;
    }

    if (environment.altRank != 0 && altRank == 0) {
        altRank = environment.altRank;
    }

//----------------------------------------------------------------
// X.509 Security Certification

    //if((rc = readIdamSecurityCert(environment.security_cert)) != 0){
    //   if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //   return(-1);
    //}

//----------------------------------------------------------------
// Check if Output Requested

    idamSetLogLevel((LOG_MODE)environment.loglevel);

    if (environment.loglevel == LOG_NONE) return 0;

//---------------------------------------------------------------
// Open the Log File

    start_status = 1;
    errno = 0;

    FILE* file = NULL;

    char idamFile[STRING_LENGTH];

    strcpy(idamFile, environment.logdir);
    strcat(idamFile, "Debug.dbg");
    file = fopen(idamFile, environment.logmode);
    idamSetLogFile(LOG_WARN, file);
    idamSetLogFile(LOG_DEBUG, file);
    idamSetLogFile(LOG_INFO, file);

    if (errno != 0) {
        addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamStartup", errno, "failed to open debug log");
        idamCloseLogging();
        return -1;
    }

    if (idamGetLogLevel() == LOG_ERROR) {
        strcpy(idamFile, environment.logdir);
        strcat(idamFile, "Error.err");
        file = fopen(idamFile, environment.logmode);
        idamSetLogFile(LOG_ERROR, file);
    }

    if (errno != 0) {
        addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "idamStartup", errno, "failed to open error log");
        idamCloseLogging();
        return -1;
    }

    reopen_logs = 0;

    return 0;
}
