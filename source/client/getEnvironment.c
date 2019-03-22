/*---------------------------------------------------------------
* Read Client Environment Variables
*
* Reads and returns values for a Standard list of IDAM Environment variables
*
* Change History
*
*--------------------------------------------------------------*/
#include "getEnvironment.h"

#include <stdlib.h>

#include <logging/logging.h>

int env_host = 1;    // User can change these before startup so flag to the getEnvironment function
int env_port = 1;

static ENVIRONMENT udaEnviron;

void putIdamClientEnvironment(const ENVIRONMENT* environment)
{
    udaEnviron = *environment;
}

void printIdamClientEnvironment(const ENVIRONMENT* environment)
{
    UDA_LOG(UDA_LOG_INFO, "\nClient Environment Variable values\n\n");
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

#ifndef NOTGENERICENABLED
    UDA_LOG(UDA_LOG_INFO, "UDA SQL Server Host: %s\n", environment->sql_host);
    UDA_LOG(UDA_LOG_INFO, "UDA SQL Server Port: %d\n", environment->sql_port);
    UDA_LOG(UDA_LOG_INFO, "UDA SQL Database   : %s\n", environment->sql_dbname);
    UDA_LOG(UDA_LOG_INFO, "UDA SQL USer       : %s\n", environment->sql_user);
#endif

}

ENVIRONMENT* getIdamClientEnvironment()
{
    char* env = NULL;

    if (udaEnviron.initialised) {
        return &udaEnviron;
    }

    //--- Read Standard Set of Environment Variables ------------------------------------

    // Log Output

    if ((env = getenv("UDA_LOG")) != NULL) {
        strcpy(udaEnviron.logdir, env);
        strcat(udaEnviron.logdir, PATH_SEPARATOR);
    } else {
#ifndef _WIN32
        strcpy(udaEnviron.logdir, "./");                    // Client Log is local to pwd
#else
        strcpy(udaEnviron.logdir, "");
#endif
    }

    if (udaEnviron.loglevel <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        sprintf(cmd, "mkdir -p %s 2>/dev/null", udaEnviron.logdir);
        system(cmd);
    }
    
    // Log Output Write Mode

    strcpy(udaEnviron.logmode, "w");                    // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != NULL) {
        if (env[0] == 'a' && strlen(env) == 1) {
            udaEnviron.logmode[0] = 'a';
        }
    }    // Append Mode

    udaEnviron.loglevel = UDA_LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != NULL) {
        if (strncmp(env, "ACCESS", 6) == 0)      udaEnviron.loglevel = UDA_LOG_ACCESS;
        else if (strncmp(env, "ERROR", 5) == 0)  udaEnviron.loglevel = UDA_LOG_ERROR;
        else if (strncmp(env, "WARN", 4) == 0)   udaEnviron.loglevel = UDA_LOG_WARN;
        else if (strncmp(env, "DEBUG", 5) == 0)  udaEnviron.loglevel = UDA_LOG_DEBUG;
        else if (strncmp(env, "INFO", 4) == 0)   udaEnviron.loglevel = UDA_LOG_INFO;
    }

    // UDA Server Host Name

    if (env_host) {                            // Check Not already set by User
        if ((env = getenv("UDA_HOST")) != NULL) {
            strcpy(udaEnviron.server_host, env);
        } else {
            strcpy(udaEnviron.server_host, UDA_SERVER_HOST);            // Default, e.g. fuslwn
        }
        // Check Not already set by User
        if ((env = getenv("UDA_HOST2")) != NULL) {
            strcpy(udaEnviron.server_host2, env);
        } else {
            strcpy(udaEnviron.server_host2, UDA_SERVER_HOST2);        // Default, e.g. fuslwi
        }
        env_host = 0;
    }

    // UDA Server Port name

    if (env_port) {
        if ((env = getenv("UDA_PORT")) != NULL) {
            udaEnviron.server_port = atoi(env);
        } else {
            udaEnviron.server_port = (int) UDA_SERVER_PORT;
        }            // Default, e.g. 56565
        if ((env = getenv("UDA_PORT2")) != NULL) {
            udaEnviron.server_port2 = atoi(env);
        } else {
            udaEnviron.server_port2 = (int) UDA_SERVER_PORT2;
        }        // Default, e.g. 56565
        env_port = 0;
    }

    // UDA Reconnect Status

    udaEnviron.server_reconnect = 0;    // No reconnection needed at startup!
    udaEnviron.server_socket = -1;    // No Socket open at startup

    //-------------------------------------------------------------------------------------------
    // API Defaults

    if ((env = getenv("UDA_DEVICE")) != NULL) {
        strcpy(udaEnviron.api_device, env);
    } else {
        strcpy(udaEnviron.api_device, API_DEVICE);
    }

    if ((env = getenv("UDA_ARCHIVE")) != NULL) {
        strcpy(udaEnviron.api_archive, env);
    } else {
        strcpy(udaEnviron.api_archive, API_ARCHIVE);
    }

    if ((env = getenv("UDA_API_DELIM")) != NULL) {
        strcpy(udaEnviron.api_delim, env);
    } else {
        strcpy(udaEnviron.api_delim, API_PARSE_STRING);
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != NULL) {
        strcpy(udaEnviron.api_format, env);
    } else {
        strcpy(udaEnviron.api_format, API_FILE_FORMAT);
    }

    //-------------------------------------------------------------------------------------------
    // Standard Data Location Path Algorithm ID

#ifdef FATCLIENT
    udaEnviron.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != NULL) udaEnviron.data_path_id = atoi(env);
#endif

    //-------------------------------------------------------------------------------------------
    // External User?

#ifdef FATCLIENT
#  ifdef EXTERNAL_USER
    udaEnviron.external_user = 1;
#  else
    udaEnviron.external_user = 0;
#  endif
    if ((env = getenv("EXTERNAL_USER")) != NULL) udaEnviron.external_user = 1;
    if ((env = getenv("UDA_EXTERNAL_USER")) != NULL) udaEnviron.external_user = 1;
#endif

    //-------------------------------------------------------------------------------------------
    // UDA Proxy Host: redirect ALL requests

#ifdef FATCLIENT
#  ifdef PROXYSERVER
    if((env = getenv("UDA_PROXY_TARGETHOST")) != NULL)
         strcpy(udaEnviron.server_proxy, env);
     else
         udaEnviron.server_proxy[0] = '\0';

    if((env = getenv("UDA_PROXY_THISHOST")) != NULL)
         strcpy(udaEnviron.server_this, env);
     else
         udaEnviron.server_this[0] = '\0';
#  endif
#endif

    //-------------------------------------------------------------------------------------------
    // Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != NULL) {
        strcpy(udaEnviron.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != NULL) {
            strcpy(udaEnviron.private_path_substitute, env);
        } else {
            udaEnviron.private_path_substitute[0] = '\0';
        }
    } else {
        udaEnviron.private_path_target[0] = '\0';
        udaEnviron.private_path_substitute[0] = '\0';
    }

    //-------------------------------------------------------------------------------------------
    // Fat Client SQL Database Connection Details

#ifndef NOTGENERICENABLED

    // UDA SQL Server Host Name

    strcpy(udaEnviron.sql_host, SQL_HOST); // Default, e.g. fuslwn
    if ((env = getenv("UDA_SQLHOST")) != NULL) strcpy(udaEnviron.sql_host, env);

    // UDA SQL Server Port name

    udaEnviron.sql_port = (int) SQL_PORT; // Default, e.g. 56566
    if ((env = getenv("UDA_SQLPORT")) != NULL) udaEnviron.sql_port = atoi(env);

    // UDA SQL Database name

    strcpy(udaEnviron.sql_dbname, SQL_DBNAME); // Default, e.g. idam
    if ((env = getenv("UDA_SQLDBNAME")) != NULL) strcpy(udaEnviron.sql_dbname, env);

    // UDA SQL Access username

    strcpy(udaEnviron.sql_user, SQL_USER); // Default, e.g. mast_db
    if ((env = getenv("UDA_SQLUSER")) != NULL) strcpy(udaEnviron.sql_user, env);

#endif

    //-------------------------------------------------------------------------------------------
    // Client defined Property Flags

    udaEnviron.clientFlags = 0;
    if ((env = getenv("UDA_FLAGS")) != NULL) udaEnviron.clientFlags = atoi(env);

    udaEnviron.altRank = 0;
    if ((env = getenv("UDA_ALTRANK")) != NULL) udaEnviron.altRank = atoi(env);

    //-------------------------------------------------------------------------------------------

    udaEnviron.initialised = 1;        // Initialisation Complete

    return &udaEnviron;
}
