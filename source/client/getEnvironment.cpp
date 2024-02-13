#include "getEnvironment.h"

#include <cstdlib>
#include <fmt/format.h>

#include "logging/logging.h"

using namespace uda::logging;

bool env_host = true; // User can change these before startup so flag to the getEnvironment function
bool env_port = true;

static uda::client_server::ENVIRONMENT udaEnviron;

void uda::client::putIdamClientEnvironment(const uda::client_server::ENVIRONMENT* environment)
{
    udaEnviron = *environment;
}

void uda::client::printIdamClientEnvironment(const uda::client_server::ENVIRONMENT* environment)
{
    UDA_LOG(UDA_LOG_INFO, "Client Environment Variable values\n");
    UDA_LOG(UDA_LOG_INFO, "Log Location    : %s\n", environment->logdir);
    UDA_LOG(UDA_LOG_INFO, "Log Write Mode  : %s\n", environment->logmode);
    UDA_LOG(UDA_LOG_INFO, "Log Level       : %d\n", environment->loglevel);
    UDA_LOG(UDA_LOG_INFO, "Client Flags    : %u\n", environment->clientFlags);
    UDA_LOG(UDA_LOG_INFO, "Alt Rank        : %d\n", environment->altRank);
#ifdef FATCLIENT
    UDA_LOG(UDA_LOG_INFO, "External User?  : %d\n", environment->external_user);
#  ifdef PROXYSERVER
    UDA_LOG(UDA_LOG_INFO, "UDA Proxy Host : %s\n", environment->server_proxy);
    UDA_LOG(UDA_LOG_INFO, "UDA This Host  : %s\n", environment->server_this);
#  endif
#endif
    UDA_LOG(UDA_LOG_INFO, "UDA Server Host: %s\n", environment->server_host);
    UDA_LOG(UDA_LOG_INFO, "UDA Server Port: %d\n", environment->server_port);
    UDA_LOG(UDA_LOG_INFO, "UDA Server Host2: %s\n", environment->server_host2);
    UDA_LOG(UDA_LOG_INFO, "UDA Server Port2: %d\n", environment->server_port2);
    UDA_LOG(UDA_LOG_INFO, "Server Reconnect: %d\n", environment->server_reconnect);
    UDA_LOG(UDA_LOG_INFO, "Server Change Socket: %d\n", environment->server_change_socket);
    UDA_LOG(UDA_LOG_INFO, "Server Socket ID: %d\n", environment->server_socket);
    UDA_LOG(UDA_LOG_INFO, "API Delimiter   : %s\n", environment->api_delim);
    UDA_LOG(UDA_LOG_INFO, "Default Device  : %s\n", environment->api_device);
    UDA_LOG(UDA_LOG_INFO, "Default Archive : %s\n", environment->api_archive);
    UDA_LOG(UDA_LOG_INFO, "Default Format  : %s\n", environment->api_format);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Target    : %s\n", environment->private_path_target);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Substitute: %s\n", environment->private_path_substitute);
}

bool uda::client::udaGetEnvHost()
{
    return env_host;
}

bool uda::client::udaGetEnvPort()
{
    return env_port;
}

void uda::client::udaSetEnvHost(bool value)
{
    env_host = value;
}

void uda::client::udaSetEnvPort(bool value)
{
    env_port = value;
}

uda::client_server::ENVIRONMENT* uda::client::getIdamClientEnvironment()
{
    char* env = nullptr;

    if (udaEnviron.initialised) {
        return &udaEnviron;
    }

    //--- Read Standard Set of Environment Variables ------------------------------------

    // Log Output

    if ((env = getenv("UDA_LOG")) != nullptr) {
        strcpy(udaEnviron.logdir, env);
        strcat(udaEnviron.logdir, PATH_SEPARATOR);
    } else {
#ifndef _WIN32
        // Client Log is local to pwd
        strcpy(udaEnviron.logdir, "./");
#else
        strcpy(udaEnviron.logdir, "");
#endif
    }
    udaEnviron.loglevel = UDA_LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != nullptr) {
        if (strncmp(env, "ACCESS", 6) == 0) {
            udaEnviron.loglevel = UDA_LOG_ACCESS;
        } else if (strncmp(env, "ERROR", 5) == 0) {
            udaEnviron.loglevel = UDA_LOG_ERROR;
        } else if (strncmp(env, "WARN", 4) == 0) {
            udaEnviron.loglevel = UDA_LOG_WARN;
        } else if (strncmp(env, "DEBUG", 5) == 0) {
            udaEnviron.loglevel = UDA_LOG_DEBUG;
        } else if (strncmp(env, "INFO", 4) == 0) {
            udaEnviron.loglevel = UDA_LOG_INFO;
        }
    }

    if (udaEnviron.loglevel <= UDA_LOG_ACCESS) {
        std::string cmd = fmt::format("mkdir -p {} 2>/dev/null", udaEnviron.logdir);
        if (system(cmd.c_str()) != 0) {
            // TODO: How to log error before log files are open?
        };
    }

    // Log Output Write Mode

    strcpy(udaEnviron.logmode, "w"); // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != nullptr) {
        if (env[0] == 'a' && strlen(env) == 1) {
            udaEnviron.logmode[0] = 'a';
        }
    } // Append Mode

    // UDA Server Host Name

    if (env_host) { // Check Not already set by User
        if ((env = getenv("UDA_HOST")) != nullptr) {
            strcpy(udaEnviron.server_host, env);
        } else {
            UDA_LOG(UDA_LOG_WARN, "UDA_HOST environmental variable not defined");
        }
        // Check Not already set by User
        if ((env = getenv("UDA_HOST2")) != nullptr) {
            strcpy(udaEnviron.server_host2, env);
        } else {
            UDA_LOG(UDA_LOG_WARN, "UDA_HOST2 environmental variable not defined");
        }
        env_host = false;
    }

    // UDA Server Port name

    if (env_port) {
        if ((env = getenv("UDA_PORT")) != nullptr) {
            udaEnviron.server_port = atoi(env);
        } else {
            udaEnviron.server_port = 56565;
            UDA_LOG(UDA_LOG_WARN, "UDA_PORT environmental variable not defined");
        }
        if ((env = getenv("UDA_PORT2")) != nullptr) {
            udaEnviron.server_port2 = atoi(env);
        } else {
            udaEnviron.server_port2 = 0;
            UDA_LOG(UDA_LOG_WARN, "UDA_PORT2 environmental variable not defined");
        }
        env_port = 0;
    }

    // UDA Reconnect Status

    udaEnviron.server_reconnect = 0; // No reconnection needed at startup!
    udaEnviron.server_socket = -1;   // No Socket open at startup

    //-------------------------------------------------------------------------------------------
    // API Defaults

    if ((env = getenv("UDA_DEVICE")) != nullptr) {
        strcpy(udaEnviron.api_device, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "API_DEVICE environmental variable not defined");
    }

    if ((env = getenv("UDA_ARCHIVE")) != nullptr) {
        strcpy(udaEnviron.api_archive, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "API_ARCHIVE environmental variable not defined");
    }

    if ((env = getenv("UDA_API_DELIM")) != nullptr) {
        strcpy(udaEnviron.api_delim, env);
    } else {
        strcpy(udaEnviron.api_delim, "::");
        UDA_LOG(UDA_LOG_WARN, "API_PARSE_STRING environmental variable not defined");
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != nullptr) {
        strcpy(udaEnviron.api_format, env);
    } else {
        UDA_LOG(UDA_LOG_WARN, "API_FILE_FORMAT environmental variable not defined");
    }

    //-------------------------------------------------------------------------------------------
    // Standard Data Location Path Algorithm ID

#ifdef FATCLIENT
    udaEnviron.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != nullptr) {
        udaEnviron.data_path_id = atoi(env);
    }
#endif

    //-------------------------------------------------------------------------------------------
    // External User?

#ifdef FATCLIENT
#  ifdef EXTERNAL_USER
    udaEnviron.external_user = 1;
#  else
    udaEnviron.external_user = 0;
#  endif
    if ((env = getenv("EXTERNAL_USER")) != nullptr) {
        udaEnviron.external_user = 1;
    }
    if ((env = getenv("UDA_EXTERNAL_USER")) != nullptr) {
        udaEnviron.external_user = 1;
    }
#endif

    //-------------------------------------------------------------------------------------------
    // UDA Proxy Host: redirect ALL requests

#ifdef FATCLIENT
#  ifdef PROXYSERVER
    if ((env = getenv("UDA_PROXY_TARGETHOST")) != nullptr) {
        strcpy(udaEnviron.server_proxy, env);
    } else {
        udaEnviron.server_proxy[0] = '\0';
    }

    if ((env = getenv("UDA_PROXY_THISHOST")) != nullptr) {
        strcpy(udaEnviron.server_this, env);
    } else {
        udaEnviron.server_this[0] = '\0';
    }
#  endif
#endif

    //-------------------------------------------------------------------------------------------
    // Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != nullptr) {
        strcpy(udaEnviron.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != nullptr) {
            strcpy(udaEnviron.private_path_substitute, env);
        } else {
            udaEnviron.private_path_substitute[0] = '\0';
        }
    } else {
        udaEnviron.private_path_target[0] = '\0';
        udaEnviron.private_path_substitute[0] = '\0';
    }

    //-------------------------------------------------------------------------------------------
    // Client defined Property Flags

    udaEnviron.clientFlags = 0;
    if ((env = getenv("UDA_FLAGS")) != nullptr) {
        udaEnviron.clientFlags = atoi(env);
    }

    udaEnviron.altRank = 0;
    if ((env = getenv("UDA_ALTRANK")) != nullptr) {
        udaEnviron.altRank = atoi(env);
    }

    //-------------------------------------------------------------------------------------------

    udaEnviron.initialised = 1; // Initialisation Complete

    return &udaEnviron;
}
