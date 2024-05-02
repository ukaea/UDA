#include "serverStartup.h"

#include <cerrno>
#include <cstdlib>
#include <string>

#include "clientserver/errorLog.h"
#include "logging/logging.h"

#include "getServerEnvironment.h"

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

int uda::server::startup()
{
    init_logging();

    //----------------------------------------------------------------
    // Read Environment Variable Values (Held in a Global Structure)

    const Environment* environment = getServerEnvironment();

    //---------------------------------------------------------------
    // Open the Log Files

    set_log_level((LogLevel) environment->loglevel);

    if (environment->loglevel <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        snprintf(cmd, STRING_LENGTH, "mkdir -p %s 2>/dev/null", environment->logdir);
        if (system(cmd) != 0) {
            UDA_THROW_ERROR(999, "mkdir command failed");
        }

        errno = 0;
        std::string log_file = std::string{environment->logdir} + "Access.log";
        set_log_file(UDA_LOG_ACCESS, log_file, environment->logmode);
    }

    if (environment->loglevel <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::string{environment->logdir} + "Error.log";
        set_log_file(UDA_LOG_ERROR, log_file, environment->logmode);
    }

    if (environment->loglevel <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::string{environment->logdir} + "DebugServer.log";
        set_log_file(UDA_LOG_WARN, log_file, environment->logmode);
        set_log_file(UDA_LOG_DEBUG, log_file, environment->logmode);
        set_log_file(UDA_LOG_INFO, log_file, environment->logmode);
    }

    printServerEnvironment(environment);

    return 0;
}
