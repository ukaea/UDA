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

static ENVIRONMENT environ = {};

void putIdamClientEnvironment(const ENVIRONMENT* environment)
{
    environ = *environment;
}

void printIdamClientEnvironment(const ENVIRONMENT* environ)
{
    IDAM_LOG(LOG_INFO, "\nClient Environment Variable values\n\n");
    IDAM_LOGF(LOG_INFO, "Log Location    : %s\n", environ->logdir);
    IDAM_LOGF(LOG_INFO, "Log Write Mode  : %s\n", environ->logmode);
    IDAM_LOGF(LOG_INFO, "Log Level       : %d\n", environ->loglevel);
    IDAM_LOGF(LOG_INFO, "Client Flags    : %u\n", environ->clientFlags);
    IDAM_LOGF(LOG_INFO, "Alt Rank        : %d\n", environ->altRank);
#ifdef FATCLIENT
    IDAM_LOGF(LOG_INFO, "External User?  : %d\n", environ->external_user);
#  ifdef PROXYSERVER
    IDAM_LOGF(LOG_INFO, "IDAM Proxy Host : %s\n", environ->server_proxy);
    IDAM_LOGF(LOG_INFO, "IDAM This Host  : %s\n", environ->server_this);
#  endif
#endif
    IDAM_LOGF(LOG_INFO, "IDAM Server Host: %s\n", environ->server_host);
    IDAM_LOGF(LOG_INFO, "IDAM Server Port: %d\n", environ->server_port);
    IDAM_LOGF(LOG_INFO, "IDAM Server Host2: %s\n", environ->server_host2);
    IDAM_LOGF(LOG_INFO, "IDAM Server Port2: %d\n", environ->server_port2);
    IDAM_LOGF(LOG_INFO, "Server Reconnect: %d\n", environ->server_reconnect);
    IDAM_LOGF(LOG_INFO, "Server Change Socket: %d\n", environ->server_change_socket);
    IDAM_LOGF(LOG_INFO, "Server Socket ID: %d\n", environ->server_socket);
    IDAM_LOGF(LOG_INFO, "API Delimiter   : %s\n", environ->api_delim);
    IDAM_LOGF(LOG_INFO, "Default Device  : %s\n", environ->api_device);
    IDAM_LOGF(LOG_INFO, "Default Archive : %s\n", environ->api_archive);
    IDAM_LOGF(LOG_INFO, "Default Format  : %s\n", environ->api_format);
    IDAM_LOGF(LOG_INFO, "Private File Path Target    : %s\n", environ->private_path_target);
    IDAM_LOGF(LOG_INFO, "Private File Path Substitute: %s\n", environ->private_path_substitute);

#ifndef NOTGENERICENABLED
    IDAM_LOGF(LOG_INFO, "IDAM SQL Server Host: %s\n", environ->sql_host);
    IDAM_LOGF(LOG_INFO, "IDAM SQL Server Port: %d\n", environ->sql_port);
    IDAM_LOGF(LOG_INFO, "IDAM SQL Database   : %s\n", environ->sql_dbname);
    IDAM_LOGF(LOG_INFO, "IDAM SQL USer       : %s\n", environ->sql_user);
#endif

}

ENVIRONMENT* getIdamClientEnvironment()
{
    char* env = NULL;

    if (environ.initialised) {
        return &environ;
    }

//--- Read Standard Set of Environment Variables ------------------------------------

// Log Output

    if ((env = getenv("UDA_LOG")) != NULL) {
        strcpy(environ.logdir, env);
        strcat(environ.logdir, PATH_SEPARATOR);
    } else {
#ifndef _WIN32
        strcpy(environ.logdir, "./");                    // Client Log is local to pwd
#else
        strcpy(environ.logfile, "");
#endif
    }

// Log Output Write Mode

    strcpy(environ.logmode, "w");                    // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != NULL) {
        if (env[0] == 'a' && strlen(env) == 1) {
            environ.logmode[0] = 'a';
        }
    }    // Append Mode

    environ.loglevel = LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != NULL) {
        if (strncmp(env, "ACCESS", 6) == 0)      environ.loglevel = LOG_ACCESS;
        else if (strncmp(env, "ERROR", 5) == 0)  environ.loglevel = LOG_ERROR;
        else if (strncmp(env, "WARN", 4) == 0)   environ.loglevel = LOG_WARN;
        else if (strncmp(env, "DEBUG", 5) == 0)  environ.loglevel = LOG_DEBUG;
        else if (strncmp(env, "INFO", 4) == 0)   environ.loglevel = LOG_INFO;
    }

// IDAM Server Host Name

    if (env_host) {                            // Check Not already set by User
        if ((env = getenv("UDA_HOST")) != NULL) {
            strcpy(environ.server_host, env);
        } else {
            strcpy(environ.server_host, UDA_SERVER_HOST);            // Default, e.g. fuslwn
        }
        // Check Not already set by User
        if ((env = getenv("UDA_HOST2")) != NULL) {
            strcpy(environ.server_host2, env);
        } else {
            strcpy(environ.server_host2, UDA_SERVER_HOST2);        // Default, e.g. fuslwi
        }
        env_host = 0;
    }

// IDAM Server Port name

    if (env_port) {
        if ((env = getenv("UDA_PORT")) != NULL) {
            environ.server_port = atoi(env);
        } else {
            environ.server_port = (int) UDA_SERVER_PORT;
        }            // Default, e.g. 56565
        if ((env = getenv("UDA_PORT2")) != NULL) {
            environ.server_port2 = atoi(env);
        } else {
            environ.server_port2 = (int) UDA_SERVER_PORT2;
        }        // Default, e.g. 56565
        env_port = 0;
    }

// IDAM Reconnect Status

    environ.server_reconnect = 0;    // No reconnection needed at startup!
    environ.server_socket = -1;    // No Socket open at startup

//-------------------------------------------------------------------------------------------
// API Defaults

    if ((env = getenv("UDA_DEVICE")) != NULL) {
        strcpy(environ.api_device, env);
    } else {
        strcpy(environ.api_device, API_DEVICE);
    }

    if ((env = getenv("UDA_ARCHIVE")) != NULL) {
        strcpy(environ.api_archive, env);
    } else {
        strcpy(environ.api_archive, API_ARCHIVE);
    }

    if ((env = getenv("UDA_API_DELIM")) != NULL) {
        strcpy(environ.api_delim, env);
    } else {
        strcpy(environ.api_delim, API_PARSE_STRING);
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != NULL) {
        strcpy(environ.api_format, env);
    } else {
        strcpy(environ.api_format, API_FILE_FORMAT);
    }

//-------------------------------------------------------------------------------------------
// Security Defaults
    /*
       if(env_cert){
          if((env = getenv("UDA_CERTIFICATE")) !=NULL)
             strcpy(environ.security_cert, env);
          else
             strcpy(environ.security_cert, SECURITY_CERT);
       }
    */

//-------------------------------------------------------------------------------------------
// Standard Data Location Path Algorithm ID

#ifdef FATCLIENT
    environ.data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != NULL) environ.data_path_id = atoi(env);
#endif

//-------------------------------------------------------------------------------------------
// External User?

#ifdef FATCLIENT
#  ifdef EXTERNAL_USER
    environ.external_user = 1;
#  else
    environ.external_user = 0;
#  endif
    if ((env = getenv("EXTERNAL_USER")) != NULL) environ.external_user = 1;
    if ((env = getenv("UDA_EXTERNAL_USER")) != NULL) environ.external_user = 1;
#endif

//-------------------------------------------------------------------------------------------
// IDAM Proxy Host: redirect ALL requests

#ifdef FATCLIENT
#  ifdef PROXYSERVER
    if((env = getenv("UDA_PROXY_TARGETHOST")) != NULL)
         strcpy(environ.server_proxy, env);
     else
         environ.server_proxy[0] = '\0';

    if((env = getenv("UDA_PROXY_THISHOST")) != NULL)
         strcpy(environ.server_this, env);
     else
         environ.server_this[0] = '\0';
#  endif
#endif

//-------------------------------------------------------------------------------------------
// Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != NULL) {
        strcpy(environ.private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != NULL) {
            strcpy(environ.private_path_substitute, env);
        } else {
            environ.private_path_substitute[0] = '\0';
        }
    } else {
        environ.private_path_target[0] = '\0';
        environ.private_path_substitute[0] = '\0';
    }

//-------------------------------------------------------------------------------------------
// Fat Client SQL Database Connection Details

#ifndef NOTGENERICENABLED

// IDAM SQL Server Host Name

    strcpy(environ.sql_host, SQL_HOST);                // Default, e.g. fuslwn
    if ((env = getenv("UDA_SQLHOST")) != NULL) strcpy(environ.sql_host, env);

// IDAM SQL Server Port name

    environ.sql_port = (int) SQL_PORT;                // Default, e.g. 56566
    if ((env = getenv("UDA_SQLPORT")) != NULL) environ.sql_port = atoi(env);

// IDAM SQL Database name

    strcpy(environ.sql_dbname, SQL_DBNAME);                // Default, e.g. idam
    if ((env = getenv("UDA_SQLDBNAME")) != NULL) strcpy(environ.sql_dbname, env);

// IDAM SQL Access username

    strcpy(environ.sql_user, SQL_USER);                // Default, e.g. mast_db
    if ((env = getenv("UDA_SQLUSER")) != NULL) strcpy(environ.sql_user, env);

#endif

//-------------------------------------------------------------------------------------------
// Client defined Property Flags

    environ.clientFlags = 0;
    if ((env = getenv("UDA_FLAGS")) != NULL) environ.clientFlags = atoi(env);

    environ.altRank = 0;
    if ((env = getenv("UDA_ALTRANK")) != NULL) environ.altRank = atoi(env);


//-------------------------------------------------------------------------------------------

    environ.initialised = 1;        // Initialisation Complete

    return &environ;
}
