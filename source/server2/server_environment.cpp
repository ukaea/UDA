#include "server_environment.hpp"

#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <string>

#include "logging/logging.h"

using namespace uda::logging;

void uda::server::Environment::print()
{
    UDA_LOG(UDA_LOG_INFO, "Server Environment Variable values\n");
    UDA_LOG(UDA_LOG_INFO, "Log Location    : %s\n", environment_.logdir);
    UDA_LOG(UDA_LOG_INFO, "Log Write Mode  : %s\n", environment_.logmode);
    UDA_LOG(UDA_LOG_INFO, "Log Level       : %d\n", environment_.loglevel);
    UDA_LOG(UDA_LOG_INFO, "External User?  : %d\n", environment_.external_user);
    UDA_LOG(UDA_LOG_INFO, "UDA Proxy Host  : %s\n", environment_.server_proxy);
    UDA_LOG(UDA_LOG_INFO, "UDA This Host   : %s\n", environment_.server_this);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Target    : %s\n", environment_.private_path_target);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Substitute: %s\n", environment_.private_path_substitute);
}

uda::server::Environment::Environment()
{
    char* env = nullptr;

    //--- Read Standard Set of Environment Variables ------------------------------------

    // Log Output

    if ((env = getenv("UDA_LOG")) != nullptr) {
        strcpy(environment_.logdir, env);
        strcat(environment_.logdir, "/");
    } else {
        strcpy(environment_.logdir, "/scratch/udalog/"); // Log is on Scratch
    }

    environment_.loglevel = UDA_LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != nullptr) {
        std::string level = env;
        boost::to_upper(level);
        if (level == "ACCESS") {
            environment_.loglevel = UDA_LOG_ACCESS;
        } else if (level == "ERROR") {
            environment_.loglevel = UDA_LOG_ERROR;
        } else if (level == "WARN") {
            environment_.loglevel = UDA_LOG_WARN;
        } else if (level == "DEBUG") {
            environment_.loglevel = UDA_LOG_DEBUG;
        } else if (level == "INFO") {
            environment_.loglevel = UDA_LOG_INFO;
        }
    }

    // Log Output Write Mode

    strcpy(environment_.logmode, "w"); // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != nullptr) {
        if (env[0] == 'a' && strlen(env) == 1) {
            environment_.logmode[0] = 'a';
        }
    } // Append Mode

    //-------------------------------------------------------------------------------------------
    // API Defaults

    if ((env = getenv("UDA_DEVICE")) != nullptr) {
        strcpy(environment_.api_device, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_DEVICE environment variable not set");
    }

    if ((env = getenv("UDA_ARCHIVE")) != nullptr) {
        strcpy(environment_.api_archive, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_ARCHIVE environment variable not set");
    }

    if ((env = getenv("UDA_API_DELIM")) != nullptr) {
        strcpy(environment_.api_delim, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_API_DELIM environment variable not set");
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != nullptr) {
        strcpy(environment_.api_format, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "UDA_FILE_FORMAT environment variable not set");
    }

    //-------------------------------------------------------------------------------------------
    // Standard Data Location Path Algorithm ID

    environment_.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != nullptr) {
        environment_.data_path_id = atoi(env);
    }

    //-------------------------------------------------------------------------------------------
    // External User?

#ifdef EXTERNAL_USER
    environment_.external_user = 1;
#else
    environment_.external_user = 0;
#endif

    if ((env = getenv("EXTERNAL_USER")) != nullptr) {
        environment_.external_user = 1;
    }
    if ((env = getenv("UDA_EXTERNAL_USER")) != nullptr) {
        environment_.external_user = 1;
    }

    //-------------------------------------------------------------------------------------------
    // UDA Proxy Host: redirect ALL requests

    if ((env = getenv("UDA_PROXY_TARGETHOST")) != nullptr) {
        strcpy(environment_.server_proxy, env);
    } else {
        environment_.server_proxy[0] = '\0';
    }

    if ((env = getenv("UDA_PROXY_THISHOST")) != nullptr) {
        strcpy(environment_.server_this, env);
    } else {
        environment_.server_this[0] = '\0';
    }

    //-------------------------------------------------------------------------------------------
    // Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != nullptr) {
        strcpy(environment_.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != nullptr) {
            strcpy(environment_.private_path_substitute, env);
        } else {
            environment_.private_path_substitute[0] = '\0';
        }
    } else {
        environment_.private_path_target[0] = '\0';
        environment_.private_path_substitute[0] = '\0';
    }

    environment_.initialised = 1;
}
