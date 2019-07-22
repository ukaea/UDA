/*---------------------------------------------------------------
* Read Server Environment Variables
*
* Reads and returns values for a Standard list of IDAM Server Environment variables
*
* Change History
*
*--------------------------------------------------------------*/

#include "getServerEnvironment.h"

#include <stdlib.h>

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

#ifndef NOTGENERICENABLED
    UDA_LOG(UDA_LOG_INFO, "IDAM SQL Server Host: %s\n", environment->sql_host);
    UDA_LOG(UDA_LOG_INFO, "IDAM SQL Server Port: %d\n", environment->sql_port);
    UDA_LOG(UDA_LOG_INFO, "IDAM SQL Database   : %s\n", environment->sql_dbname);
    UDA_LOG(UDA_LOG_INFO, "IDAM SQL USer       : %s\n", environment->sql_user);
#endif
}

ENVIRONMENT* getIdamServerEnvironment()
{
    char* env = NULL;

    if (g_environ.initialised) {
        return &g_environ;
    }

//--- Read Standard Set of Environment Variables ------------------------------------

// Log Output

    if ((env = getenv("UDA_LOG")) != NULL) {
        strcpy(g_environ.logdir, env);
        strcat(g_environ.logdir, "/");
    } else {
        strcpy(g_environ.logdir, "/scratch/idamlog/");        // Log is on Scratch
    }

    g_environ.loglevel = UDA_LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != NULL) {
        if (strncmp(env, "ACCESS", 6) == 0) { g_environ.loglevel = UDA_LOG_ACCESS; }
        else if (strncmp(env, "ERROR", 5) == 0) { g_environ.loglevel = UDA_LOG_ERROR; }
        else if (strncmp(env, "WARN", 4) == 0) { g_environ.loglevel = UDA_LOG_WARN; }
        else if (strncmp(env, "DEBUG", 5) == 0) { g_environ.loglevel = UDA_LOG_DEBUG; }
        else if (strncmp(env, "INFO", 4) == 0) { g_environ.loglevel = UDA_LOG_INFO; }
    }

// Log Output Write Mode

    strcpy(g_environ.logmode, "w");                    // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != NULL) {
        if (env[0] == 'a' && strlen(env) == 1) {
            g_environ.logmode[0] = 'a';
        }
    }    // Append Mode

//-------------------------------------------------------------------------------------------
// API Defaults

    if ((env = getenv("UDA_DEVICE")) != NULL) {
        strcpy(g_environ.api_device, env);
    } else {
        strcpy(g_environ.api_device, API_DEVICE);
    }

    if ((env = getenv("UDA_ARCHIVE")) != NULL) {
        strcpy(g_environ.api_archive, env);
    } else {
        strcpy(g_environ.api_archive, API_ARCHIVE);
    }

    if ((env = getenv("UDA_API_DELIM")) != NULL) {
        strcpy(g_environ.api_delim, env);
    } else {
        strcpy(g_environ.api_delim, API_PARSE_STRING);
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != NULL) {
        strcpy(g_environ.api_format, env);
    } else {
        strcpy(g_environ.api_format, API_FILE_FORMAT);
    }

//-------------------------------------------------------------------------------------------
// Standard Data Location Path Algorithm ID

    g_environ.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != NULL) { g_environ.data_path_id = atoi(env); }

//-------------------------------------------------------------------------------------------
// External User?

#ifdef EXTERNAL_USER
    g_environ.external_user = 1;
#else
    g_environ.external_user = 0;
#endif

    if ((env = getenv("EXTERNAL_USER")) != NULL) { g_environ.external_user = 1; }
    if ((env = getenv("UDA_EXTERNAL_USER")) != NULL) { g_environ.external_user = 1; }

//-------------------------------------------------------------------------------------------
// IDAM Proxy Host: redirect ALL requests

    if ((env = getenv("UDA_PROXY_TARGETHOST")) != NULL) {
        strcpy(g_environ.server_proxy, env);
    } else {
        g_environ.server_proxy[0] = '\0';
    }

    if ((env = getenv("UDA_PROXY_THISHOST")) != NULL) {
        strcpy(g_environ.server_this, env);
    } else {
        g_environ.server_this[0] = '\0';
    }

//-------------------------------------------------------------------------------------------
// Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != NULL) {
        strcpy(g_environ.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != NULL) {
            strcpy(g_environ.private_path_substitute, env);
        } else {
            g_environ.private_path_substitute[0] = '\0';
        }
    } else {
        g_environ.private_path_target[0] = '\0';
        g_environ.private_path_substitute[0] = '\0';
    }

//-------------------------------------------------------------------------------------------
// SQL Database Connection Details

#ifndef NOTGENERICENABLED

// IDAM SQL Server Host Name

    strcpy(g_environ.sql_host, SQL_HOST);             // Default, e.g. fuslwn
    if ((env = getenv("UDA_SQLHOST")) != NULL) strcpy(g_environ.sql_host, env);

// IDAM SQL Server Port name

    g_environ.sql_port = (int) SQL_PORT;              // Default, e.g. 56566
    if ((env = getenv("UDA_SQLPORT")) != NULL) { g_environ.sql_port = atoi(env); }

// IDAM SQL Database name

    strcpy(g_environ.sql_dbname, SQL_DBNAME);         // Default, e.g. idam
    if ((env = getenv("UDA_SQLDBNAME")) != NULL) strcpy(g_environ.sql_dbname, env);

// IDAM SQL Access username

    strcpy(g_environ.sql_user, SQL_USER);             // Default, e.g. mast_db
    if ((env = getenv("UDA_SQLUSER")) != NULL) strcpy(g_environ.sql_user, env);

#endif

    g_environ.initialised = 1;

    return &g_environ;
}
