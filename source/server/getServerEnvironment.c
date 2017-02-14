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

#include <logging/idamLog.h>

void printIdamServerEnvironment(ENVIRONMENT* environ)
{
    IDAM_LOG(LOG_INFO, "\nServer Environment Variable values\n\n");
    IDAM_LOGF(LOG_INFO, "Log Location    : %s\n", environ->logdir);
    IDAM_LOGF(LOG_INFO, "Log Write Mode  : %s\n", environ->logmode);
    IDAM_LOGF(LOG_INFO, "Log Level       : %d\n", environ->loglevel);
    IDAM_LOGF(LOG_INFO, "External User?  : %d\n", environ->external_user);
    IDAM_LOGF(LOG_INFO, "IDAM Proxy Host : %s\n", environ->server_proxy);
    IDAM_LOGF(LOG_INFO, "IDAM This Host  : %s\n", environ->server_this);
    IDAM_LOGF(LOG_INFO, "Private File Path Target    : %s\n", environ->private_path_target);
    IDAM_LOGF(LOG_INFO, "Private File Path Substitute: %s\n", environ->private_path_substitute);

#ifndef NOTGENERICENABLED
    IDAM_LOGF(LOG_INFO, "IDAM SQL Server Host: %s\n", environ->sql_host);
    IDAM_LOGF(LOG_INFO, "IDAM SQL Server Port: %d\n", environ->sql_port);
    IDAM_LOGF(LOG_INFO, "IDAM SQL Database   : %s\n", environ->sql_dbname);
    IDAM_LOGF(LOG_INFO, "IDAM SQL USer       : %s\n", environ->sql_user);
#endif
}

void getIdamServerEnvironment(ENVIRONMENT* environ)
{

    char* env = NULL;

//--- Read Standard Set of Environment Variables ------------------------------------

// Log Output

    if ((env = getenv("UDA_LOG")) != NULL) {
        strcpy(environ->logdir, env);
        strcat(environ->logdir, "/");
    } else {
        strcpy(environ->logdir, "/scratch/idamlog/");        // Log is on Scratch
    }

    environ->loglevel = LOG_NONE;
    if ((env = getenv("UDA_LOG_LEVEL")) != NULL) {
        if (strncmp(env, "ACCESS", 6) == 0) { environ->loglevel = LOG_ACCESS; }
        else if (strncmp(env, "ERROR", 5) == 0) { environ->loglevel = LOG_ERROR; }
        else if (strncmp(env, "WARN", 4) == 0) { environ->loglevel = LOG_WARN; }
        else if (strncmp(env, "DEBUG", 5) == 0) { environ->loglevel = LOG_DEBUG; }
        else if (strncmp(env, "INFO", 4) == 0) { environ->loglevel = LOG_INFO; }
    }

// Log Output Write Mode

    strcpy(environ->logmode, "w");                    // Write & Replace Mode
    if ((env = getenv("UDA_LOG_MODE")) != NULL) {
        if (env[0] == 'a' && strlen(env) == 1) {
            environ->logmode[0] = 'a';
        }
    }    // Append Mode

//-------------------------------------------------------------------------------------------
// API Defaults

    if ((env = getenv("UDA_DEVICE")) != NULL) {
        strcpy(environ->api_device, env);
    } else {
        strcpy(environ->api_device, API_DEVICE);
    }

    if ((env = getenv("UDA_ARCHIVE")) != NULL) {
        strcpy(environ->api_archive, env);
    } else {
        strcpy(environ->api_archive, API_ARCHIVE);
    }

    if ((env = getenv("UDA_API_DELIM")) != NULL) {
        strcpy(environ->api_delim, env);
    } else {
        strcpy(environ->api_delim, API_PARSE_STRING);
    }

    if ((env = getenv("UDA_FILE_FORMAT")) != NULL) {
        strcpy(environ->api_format, env);
    } else {
        strcpy(environ->api_format, API_FILE_FORMAT);
    }

//-------------------------------------------------------------------------------------------
// Standard Data Location Path Algorithm ID

    environ->data_path_id = 0;
    if ((env = getenv("UDA_DATAPATHID")) != NULL) { environ->data_path_id = atoi(env); }

//-------------------------------------------------------------------------------------------
// External User?

#ifdef EXTERNAL_USER
    environ->external_user = 1;
#else
    environ->external_user = 0;
#endif

    if ((env = getenv("EXTERNAL_USER")) != NULL) { environ->external_user = 1; }
    if ((env = getenv("UDA_EXTERNAL_USER")) != NULL) { environ->external_user = 1; }

//-------------------------------------------------------------------------------------------
// IDAM Proxy Host: redirect ALL requests

    if ((env = getenv("UDA_PROXY_TARGETHOST")) != NULL) {
        strcpy(environ->server_proxy, env);
    } else {
        environ->server_proxy[0] = '\0';
    }

    if ((env = getenv("UDA_PROXY_THISHOST")) != NULL) {
        strcpy(environ->server_this, env);
    } else {
        environ->server_this[0] = '\0';
    }

//-------------------------------------------------------------------------------------------
// Private File Path substitution: Enables server to see files if the path contains too many hierarchical elements

    if ((env = getenv("UDA_PRIVATE_PATH_TARGET")) != NULL) {
        strcpy(environ->private_path_target, env);
        if ((env = getenv("UDA_PRIVATE_PATH_SUBSTITUTE")) != NULL) {
            strcpy(environ->private_path_substitute, env);
        } else {
            environ->private_path_substitute[0] = '\0';
        }
    } else {
        environ->private_path_target[0] = '\0';
        environ->private_path_substitute[0] = '\0';
    }

//-------------------------------------------------------------------------------------------
// SQL Database Connection Details

#ifndef NOTGENERICENABLED

// IDAM SQL Server Host Name

    strcpy(environ->sql_host, SQL_HOST);                // Default, e.g. fuslwn
    if ((env = getenv("UDA_SQLHOST")) != NULL) strcpy(environ->sql_host, env);

// IDAM SQL Server Port name

    environ->sql_port = (int) SQL_PORT;                // Default, e.g. 56566
    if ((env = getenv("UDA_SQLPORT")) != NULL) { environ->sql_port = atoi(env); }

// IDAM SQL Database name

    strcpy(environ->sql_dbname, SQL_DBNAME);                // Default, e.g. idam
    if ((env = getenv("UDA_SQLDBNAME")) != NULL) strcpy(environ->sql_dbname, env);

// IDAM SQL Access username

    strcpy(environ->sql_user, SQL_USER);                // Default, e.g. mast_db
    if ((env = getenv("UDA_SQLUSER")) != NULL) strcpy(environ->sql_user, env);

#endif

    return;
}
