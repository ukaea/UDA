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
#include <filesystem>

#include "clientserver/errorLog.h"
#include "clientserver/udaErrors.h"
#include "logging/logging.h"

#include "udaClient.h"
#include "client_config.h"

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::logging;

using namespace std::string_literals;

int uda::client::udaStartup(int reset, CLIENT_FLAGS* client_flags)
{
    static bool start_status = false;

    //---------------------------------------------------------------
    // Are the Files Already Open?

    if (start_status && !reset) {
        return 0;
    }

    //----------------------------------------------------------------
    // Read Environment Variable Values (Held in a Global Structure)

    auto config = client_config();
    config->print();

    //----------------------------------------------------------------
    // Client set Property Flags (can be changed via property accessor functions)
    // Coded user properties changes have priority

    client_flags->flags = config->get("client.flags").as_or_default(0);
    client_flags->alt_rank = config->get("client.alt_rank").as_or_default(0);

    //----------------------------------------------------------------
    // X.509 Security Certification

    // if((rc = readIdamSecurityCert(environment->security_cert)) != 0){
    //    if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //    return(-1);
    // }

    //----------------------------------------------------------------
    // Check if Output Requested

    auto log_level = (LogLevel)config->get("logging.level").as_or_default((int)UDA_LOG_NONE);
    set_log_level(log_level);

    if (log_level == UDA_LOG_NONE) {
        return 0;
    }

    //---------------------------------------------------------------
    // Open the Log File

    start_status = true;

    return 0;
}
