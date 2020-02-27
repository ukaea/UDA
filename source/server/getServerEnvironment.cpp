/*---------------------------------------------------------------
* Read Server Environment Variables
*
* Reads and returns values for a Standard list of IDAM Server Environment variables
*
* Change History
*
*--------------------------------------------------------------*/

#include "getServerEnvironment.h"

#include <cstdlib>

#include <logging/logging.h>

// 2019-07-04 Herve Ancher (CEA): Add prefix "g_" to avoid conflict with internal MinGW varaible
static ENVIRONMENT g_environ;

void putIdamServerEnvironment(const ENVIRONMENT* environment)
{
    g_environ = *environment;
}

void printIdamServerEnvironment(const ENVIRONMENT* environment)
{
    UDA_LOG(UDA_LOG_INFO, "\nServer Environment Variable values\n\n");
    UDA_LOG(UDA_LOG_INFO, "Log Location    : %s\n", environment->logdir);
    UDA_LOG(UDA_LOG_INFO, "Log Write Mode  : %s\n", environment->logmode);
    UDA_LOG(UDA_LOG_INFO, "Log Level       : %d\n", environment->loglevel);
    UDA_LOG(UDA_LOG_INFO, "External User?  : %d\n", environment->external_user);
    UDA_LOG(UDA_LOG_INFO, "IDAM Proxy Host : %s\n", environment->server_proxy);
    UDA_LOG(UDA_LOG_INFO, "IDAM This Host  : %s\n", environment->server_this);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Target    : %s\n", environment->private_path_target);
    UDA_LOG(UDA_LOG_INFO, "Private File Path Substitute: %s\n", environment->private_path_substitute);
}

ENVIRONMENT* getIdamServerEnvironment()
{
    char* env = nullptr;

    if (g_environ.initialised) {
        return &g_environ;
    }

    //--- Read Standard Set of Environment Variables ------------------------------------

    // Log Output

    if ((env = getenv("UDA_LOG")) != nullptr) {
        strcpy(g_environ.logdir, env);
        strcat(g_environ.logdir, "/");
    } else {
        strcpy(g_environ.logdir, "/scratch/idamlog/");        // Log is on Scratch
    }

    g_environ.loglevel = UDA_LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != nullptr) {
        if (strncmp(env, "ACCESS", 6) == 0) { g_environ.loglevel = UDA_LOG_ACCESS; }
        else if (strncmp(env, "ERROR", 5) == 0) { g_environ.loglevel = UDA_LOG_ERROR; }
        else if (strncmp(env, "WARN", 4) == 0) { g_environ.loglevel = UDA_LOG_WARN; }
        else if (strncmp(env, "DEBUG", 5) == 0) { g_environ.loglevel = UDA_LOG_DEBUG; }
        else if (strncmp(env, "INFO", 4) == 0) { g_environ.loglevel = UDA_LOG_INFO; }
    }

    // Log Output Write Mode

    strcpy(g_environ.logmode, "w");                    // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != nullptr) {
        if (env[0] == 'a' && strlen(env) == 1) {
            g_environ.logmode[0] = 'a';
        }
    }    // Append Mode

    //-------------------------------------------------------------------------------------------
    // API Defaults

    if ((env = getenv("UDA_DEVICE")) != nullptr) {
        strcpy(g_environ.api_device, env);
    } else {
        strcpy(g_environ.api_device, API_DEVICE);
    }

    if ((env = getenv("UDA_ARCHIVE")) != nullptr) {
        strcpy(g_environ.api_archive, env);
    } else {
        strcpy(g_environ.api_archive, API_ARCHIVE);
    }

    if ((env = getenv("UDA_API_DELIM")) != nullptr) {
        strcpy(g_environ.api_delim, env);
    } else {
        strcpy(g_environ.api_delim, API_PARSE_STRING);
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != nullptr) {
        strcpy(g_environ.api_format, env);
    } else {
        strcpy(g_environ.api_format, API_FILE_FORMAT);
    }

    //-------------------------------------------------------------------------------------------
    // Standard Data Location Path Algorithm ID

    g_environ.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != nullptr) { g_environ.data_path_id = atoi(env); }

    //-------------------------------------------------------------------------------------------
    // External User?

#ifdef EXTERNAL_USER
    g_environ.external_user = 1;
#else
    g_environ.external_user = 0;
#endif

    if ((env = getenv("EXTERNAL_USER")) != nullptr) { g_environ.external_user = 1; }
    if ((env = getenv("UDA_EXTERNAL_USER")) != nullptr) { g_environ.external_user = 1; }

    //-------------------------------------------------------------------------------------------
    // IDAM Proxy Host: redirect ALL requests

    if ((env = getenv("UDA_PROXY_TARGETHOST")) != nullptr) {
        strcpy(g_environ.server_proxy, env);
    } else {
        g_environ.server_proxy[0] = '\0';
    }

    if ((env = getenv("UDA_PROXY_THISHOST")) != nullptr) {
        strcpy(g_environ.server_this, env);
    } else {
        g_environ.server_this[0] = '\0';
    }

    //-------------------------------------------------------------------------------------------
    // Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != nullptr) {
        strcpy(g_environ.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != nullptr) {
            strcpy(g_environ.private_path_substitute, env);
        } else {
            g_environ.private_path_substitute[0] = '\0';
        }
    } else {
        g_environ.private_path_target[0] = '\0';
        g_environ.private_path_substitute[0] = '\0';
    }

    g_environ.initialised = 1;

    return &g_environ;
}
