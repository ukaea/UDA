/*---------------------------------------------------------------
 *  Open Client Side Log Files
 *
 * Argument: reset => if 1 then always reopen Files
 *
 * Checks for an Environment var (UDA_LOG) for the directory of the log file
 * otherwise it assumes the file is to be local to the PWD.
 *
 * The directory named must not have a trailing forward slash.
 *
 * It is assumed that the directory specified by UDA_LOG already exits
 *
 * Returns:
 *
 *--------------------------------------------------------------*/
#include "startup.h"

#include <cerrno>

#include "clientserver/errorLog.h"
#include "clientserver/udaErrors.h"
#include "logging/logging.h"

#include "getEnvironment.h"
#include "udaClient.h"

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::logging;

int uda::client::udaStartup(int reset, CLIENT_FLAGS* client_flags, bool* reopen_logs)
{
    static int start_status = 0;

    //---------------------------------------------------------------
    // Are the Files Already Open?

    if (start_status && !reset && !*reopen_logs) {
        return 0;
    }

    //----------------------------------------------------------------
    // Read Environment Variable Values (Held in a Global Structure)

    const Environment* environment = nullptr;

    if (!start_status || environment == nullptr) {
        environment = getIdamClientEnvironment();
    }

    printIdamClientEnvironment(environment);

    //----------------------------------------------------------------
    // Client set Property Flags (can be changed via property accessor functions)
    // Coded user properties changes have priority

    if (environment->clientFlags != 0) {
        client_flags->flags |= environment->clientFlags;
    }

    if (environment->altRank != 0 && client_flags->alt_rank == 0) {
        client_flags->alt_rank = environment->altRank;
    }

    //----------------------------------------------------------------
    // X.509 Security Certification

    // if((rc = readIdamSecurityCert(environment->security_cert)) != 0){
    //    if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //    return(-1);
    // }

    //----------------------------------------------------------------
    // Check if Output Requested

    init_logging();
    set_log_level((LogLevel) environment->loglevel);

    if (environment->loglevel == UDA_LOG_NONE) {
        return 0;
    }

    //---------------------------------------------------------------
    // Open the Log File

    start_status = 1;
    errno = 0;

    char log_file[STRING_LENGTH];

    strcpy(log_file, environment->logdir);
    strcat(log_file, "Debug.dbg");
    set_log_file(UDA_LOG_WARN, log_file, environment->logmode);
    set_log_file(UDA_LOG_DEBUG, log_file, environment->logmode);
    set_log_file(UDA_LOG_INFO, log_file, environment->logmode);

    if (errno != 0) {
        add_error(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "failed to open debug log");
        close_logging();
        return -1;
    }

    if (get_log_level() <= UDA_LOG_ERROR) {
        strcpy(log_file, environment->logdir);
        strcat(log_file, "Error.err");
        set_log_file(UDA_LOG_ERROR, log_file, environment->logmode);
    }

    if (errno != 0) {
        add_error(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "failed to open error log");
        close_logging();
        return -1;
    }

    *reopen_logs = false;

    return 0;
}
