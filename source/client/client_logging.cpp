#include "client_logging.h"

#include <filesystem>

#include "logging/logging.h"
#include "client/client_config.h"
#include "clientserver/errorLog.h"

using namespace uda::logging;
using namespace uda::client_server;

using namespace std::string_literals;

void uda::client::init_logging()
{
    static bool initialised = false;

    if (initialised) {
        return;
    }

    uda::logging::init_logging();

    auto config = client_config();

    //---------------------------------------------------------------
    // Open the Log Files

    auto log_level = (LogLevel)config->get("logging.level").as_or_default((int)UDA_LOG_NONE);
    set_log_level(log_level);

    auto log_dir = config->get("logging.path").as_or_default(""s);
    auto log_mode = config->get("logging.mode").as_or_default("w"s);

    if (log_level <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::filesystem::path{log_dir} / "Error.err";
        set_log_file(UDA_LOG_ERROR, log_file, log_mode);
    }

    if (log_level <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::filesystem::path{log_dir} / "Debug.dbg";
        set_log_file(UDA_LOG_WARN, log_file, log_mode);
        set_log_file(UDA_LOG_DEBUG, log_file, log_mode);
        set_log_file(UDA_LOG_INFO, log_file, log_mode);
    }

    config->print();

    initialised = true;
}