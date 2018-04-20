/*---------------------------------------------------------------
* IDAM Plugin to Access the Provenance Database
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		admin	0 	if read was successful
*					otherwise a Error Code is returned 
*			DATA_BLOCK	Structure with Data from the File 
*      
* Issues:
*
*---------------------------------------------------------------------------------------------------------------*/
#include "provenance.h"

#include <stdlib.h>
#include <strings.h>

#include <structures/struct.h>
#include <clientserver/stringUtils.h>
#include <server/getServerEnvironment.h>

#include "put.h"
#include "help.h"
#include "get.h"
#include "putSignal.h"
#include "status.h"
#include "listSignals.h"

const char* pghost = NULL;
char pgport[56];
char* dbname = NULL;
char* user = NULL;
char* pswrd = NULL;
int initTime = 0;

PGconn* startSQL_Provenance(const ENVIRONMENT* environment)
{
    pghost = environment->sql_host;
    pswrd = NULL;
    sprintf(pgport, "%d", environment->sql_port);

    char* env;
    if ((env = getenv("UDA_PROVDBHOST")) != NULL) pghost = env;
    if ((env = getenv("UDA_PROVDBPORT")) != NULL) strcpy(pgport, env);
    if ((env = getenv("UDA_PROVDBNAME")) != NULL) dbname = env;
    if ((env = getenv("UDA_PROVDBUSER")) != NULL) user = env;
    if ((env = getenv("UDA_PROVDBPSWD")) != NULL) pswrd = env;

    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

    // Login password is stored in .pgpass for POSTGRESQL database so no need to set

    PGconn* DBConnect = NULL;

    //-------------------------------------------------------------
    // Debug Trace Queries

    UDA_LOG(UDA_LOG_DEBUG, "Provenance SQL Connection: host %s\n", pghost);
    UDA_LOG(UDA_LOG_DEBUG, "                           port %s\n", pgport);
    UDA_LOG(UDA_LOG_DEBUG, "                           db   %s\n", dbname);
    UDA_LOG(UDA_LOG_DEBUG, "                           user %s\n", user);

    //-------------------------------------------------------------
    // Connect to the Database Server

    if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL) {
        addIdamError(CODEERRORTYPE, "startSQL_Provenance", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        addIdamError(CODEERRORTYPE, "startSQL_Provenance", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

        UDA_LOG(UDA_LOG_DEBUG, "Provenance SQL Connection Options: %s\n", PQoptions(DBConnect));

    return (DBConnect);
}

int preventSQLInjection(PGconn* DBConnect, char** from, unsigned short freeHeap)
{

// Replace the passed string with an Escaped String
// Free the Original string from Heap if requested

    int err = 0;
    size_t fromCount = strlen(*from);
    char* to = (char*) malloc((2 * fromCount + 1) * sizeof(char));
    PQescapeStringConn(DBConnect, to, *from, fromCount, &err);
    if (err != 0) {
        if (to != NULL) free((void*) to);
        return 1;
    }
    if (freeHeap) free((void*) *from);
    *from = to;
    return 0;
}

int admin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static short init = 0;
    static short sqlPrivate = 1;            // The SQL connection is private and is not passed back.

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    IDAM_PLUGIN_INTERFACE local_idam_plugin_interface;

    REQUEST_BLOCK* request_block;

    unsigned short housekeeping;

    static unsigned short DBType = PLUGINSQLNOTKNOWN;
    static PGconn* DBConnect = NULL;

    if (idam_plugin_interface->interfaceVersion == 1) {
        idam_plugin_interface->pluginVersion = 1;
        request_block = idam_plugin_interface->request_block;
        housekeeping = idam_plugin_interface->housekeeping;
    } else {
        RAISE_PLUGIN_ERROR("Plugin Interface Version is Not Known");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Provenance: Plugin Interface transferred\n");

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        UDA_LOG(UDA_LOG_DEBUG, "Provenance: reset function called.\n");

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (DBConnect != NULL && DBType == PLUGINSQLPOSTGRES && sqlPrivate) {
            UDA_LOG(UDA_LOG_DEBUG, "Provenance: Closing SQL connection\n");
            PQfinish(DBConnect);
            sqlPrivate = 1;        // Remains Private
            DBConnect = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }
        init = 0;        // Ready to re-initialise
        UDA_LOG(UDA_LOG_DEBUG, "Provenance: reset executed\n");
        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise if requested (the previous private SQL connection must be closed) 

    if (!STR_IEQUALS(request_block->function, "help") && (!init || STR_IEQUALS(request_block->function, "init")
                                                          || STR_IEQUALS(request_block->function, "initialise"))) {

        UDA_LOG(UDA_LOG_DEBUG, "Provenance: init function called.\n");

// Is there an Open SQL Connection? If not then open a private connection

        if (DBConnect == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)) {
            DBConnect = startSQL_Provenance(idam_plugin_interface->environment);        // No prior connection to Postgres SQL Database
            if (DBConnect != NULL) {
                DBType = PLUGINSQLPOSTGRES;
                sqlPrivate = 1;
                UDA_LOG(UDA_LOG_DEBUG, "Provenance: Private regular database connection made.\n");
            }
        }

        if (DBConnect == NULL) {        // No connection!
            RAISE_PLUGIN_ERROR("SQL Database Server Connect Error");
        }

// Initialisation complete

        initTime = (int)time(NULL);
        init = 1;

        UDA_LOG(UDA_LOG_DEBUG, "Provenance: Plugin initialised and SQL connection made\n");

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Create a local Interface structure and pass the SQL connection details

    local_idam_plugin_interface = *idam_plugin_interface;

    local_idam_plugin_interface.sqlConnectionType = DBType;
    local_idam_plugin_interface.sqlConnection = (void*)DBConnect;

    //----------------------------------------------------------------------------------------
    // Functions

    int err;

    if (STR_IEQUALS(request_block->function, "help")) {
        //----------------------------------------------------------------------------------------
        // HELP
        err = help(&local_idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "get") || STR_IEQUALS(request_block->function, "new")) {
        //----------------------------------------------------------------------------------------
        // GET a new Registered UUID
        err = get(&local_idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "status")) {
        //----------------------------------------------------------------------------------------
        // STATUS of a UUID
        err = status(&local_idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "put")) {
        //----------------------------------------------------------------------------------------
        // PUT Provenance metadata
        err = put(&local_idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "putSignal") ||
               STR_IEQUALS(request_block->function, "recordSignal") ||
               STR_IEQUALS(request_block->function, "addSignal")) {
        //----------------------------------------------------------------------------------------
        // putSignal
        err = putSignal(&local_idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "listSignals") || STR_IEQUALS(request_block->function, "list")) {
        //----------------------------------------------------------------------------------------
        // listSignals

        err = listSignals(&local_idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown Function requested");
    }

    return err;
}
