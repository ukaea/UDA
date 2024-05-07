#include "serverStartup.h"

#include <cerrno>
#include <cstdlib>
#include <string>
#include <filesystem>

#include "clientserver/errorLog.h"
#include "logging/logging.h"
#include "server_config.h"

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

using namespace std::string_literals;

int uda::server::startup()
{
    init_logging();

    //----------------------------------------------------------------
    // Read Environment Variable Values (Held in a Global Structure)

    auto config = server_config();

    //---------------------------------------------------------------
    // Open the Log Files

    auto log_level = (LogLevel)config->get("server.log_level").as_or_default((int)UDA_LOG_NONE);
    set_log_level(log_level);

    auto log_dir = config->get("server.log_dir").as_or_default(""s);
    auto log_mode = config->get("server.log_mode").as_or_default("w"s);

    if (log_level <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        snprintf(cmd, STRING_LENGTH, "mkdir -p %s 2>/dev/null", log_dir.c_str());
        if (system(cmd) != 0) {
            UDA_THROW_ERROR(999, "mkdir command failed");
        }

        errno = 0;
        std::string log_file = std::filesystem::path{log_dir} / "Access.log";
        set_log_file(UDA_LOG_ACCESS, log_file, log_mode);
    }

    if (log_level <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::filesystem::path{log_dir} / "Error.log";
        set_log_file(UDA_LOG_ERROR, log_file, log_mode);
    }

    if (log_level <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::filesystem::path{log_dir} / "DebugServer.log";
        set_log_file(UDA_LOG_WARN, log_file, log_mode);
        set_log_file(UDA_LOG_DEBUG, log_file, log_mode);
        set_log_file(UDA_LOG_INFO, log_file, log_mode);
    }

    config->print();

    return 0;
}
