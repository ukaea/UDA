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

#include <structures/struct.h>

char* pghost = NULL;
char pgport[56];
char* dbname = NULL;
char* user = NULL;
char* pswrd = NULL;
int initTime = 0;

PGconn* startSQL_Provenance()
{

    pghost = environment.sql_host;
    pswrd = NULL;
    sprintf(pgport, "%d", environment.sql_port);

    char* env;
    if ((env = getenv("IDAM_PROVDBHOST")) != NULL) pghost = env;
    if ((env = getenv("IDAM_PROVDBPORT")) != NULL) strcpy(pgport, env);
    if ((env = getenv("IDAM_PROVDBNAME")) != NULL) dbname = env;
    if ((env = getenv("IDAM_PROVDBUSER")) != NULL) user = env;
    if ((env = getenv("IDAM_PROVDBPSWD")) != NULL) pswrd = env;

    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

// Login password is stored in .pgpass for POSTGRESQL database so no need to set

    PGconn* DBConnect = NULL;

//------------------------------------------------------------- 
// Debug Trace Queries

//        PQtrace(DBConnect, dbgout);
        idamLog(LOG_DEBUG, "Provenance SQL Connection: host %s\n", pghost);
        idamLog(LOG_DEBUG, "                           port %s\n", pgport);
        idamLog(LOG_DEBUG, "                           db   %s\n", dbname);
        idamLog(LOG_DEBUG, "                           user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL_Provenance", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL_Provenance", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

        idamLog(LOG_DEBUG, "Provenance SQL Connection Options: %s\n", PQoptions(DBConnect));

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

    int err = 0;
    static short init = 0;
    static short sqlPrivate = 1;            // The SQL connection is private and is not passed back.

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    IDAM_PLUGIN_INTERFACE local_idam_plugin_interface;

    REQUEST_BLOCK* request_block;

#ifndef USE_PLUGIN_DIRECTLY
    IDAMERRORSTACK idamerrorstack;
    IDAMERRORSTACK* idamErrorStack = getIdamServerPluginErrorStack();        // Server library functions

    logmalloclist = getIdamServerLogMallocList();
    userdefinedtypelist = getIdamServerUserDefinedTypeList();

    initIdamErrorStack(&idamerrorstack);
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;
    //LOGMALLOCLIST  *logmalloclist            = getIdamServerLogMallocList();
    //USERDEFINEDTYPELIST *userdefinedtypelist = getIdamServerUserDefinedTypeList();
#endif

    unsigned short housekeeping;

    static unsigned short DBType = PLUGINSQLNOTKNOWN;
    static PGconn* DBConnect = NULL;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        request_block = idam_plugin_interface->request_block;

        housekeeping = idam_plugin_interface->housekeeping;

#ifndef USE_PLUGIN_DIRECTLY
// Don't copy the structure if housekeeping is requested - may dereference a NULL  or freed pointer!     
        if (!housekeeping && idam_plugin_interface->environment != NULL)
            environment = *idam_plugin_interface->environment;
#endif

    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR Provenance: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    idamLog(LOG_DEBUG, "Provenance: Plugin Interface transferred\n");


//----------------------------------------------------------------------------------------
// Heap Housekeeping 

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        idamLog(LOG_DEBUG, "Provenance: reset function called.\n");

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (DBConnect != NULL && DBType == PLUGINSQLPOSTGRES && sqlPrivate) {
            idamLog(LOG_DEBUG, "Provenance: Closing SQL connection\n");
            PQfinish(DBConnect);
            sqlPrivate = 1;        // Remains Private
            DBConnect = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }
        init = 0;        // Ready to re-initialise
        idamLog(LOG_DEBUG, "Provenance: reset executed\n");
        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise if requested (the previous private SQL connection must be closed) 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        idamLog(LOG_DEBUG, "Provenance: init function called.\n");

// Is there an Open SQL Connection? If not then open a private connection

        if (DBConnect == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)) {
            DBConnect = (PGconn*) startSQL_Provenance();        // No prior connection to Postgres SQL Database
            if (DBConnect != NULL) {
                DBType = PLUGINSQLPOSTGRES;
                sqlPrivate = 1;
                idamLog(LOG_DEBUG, "Provenance: Private regular database connection made.\n");
            }
        }

        if (DBConnect == NULL) {        // No connection!
            idamLog(LOG_ERROR, "ERROR Provenance: SQL Database Server Connect Error\n");
            err = 777;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, "SQL Database Server Connect Error");
            concatIdamError(idamerrorstack, idamErrorStack);
            return err;
        }

// Initialisation complete

        initTime = (int) time(NULL);
        init = 1;

        idamLog(LOG_DEBUG, "Provenance: Plugin initialised and SQL connection made\n");

        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }


//----------------------------------------------------------------------------------------
// Create a local Interface structure and pass the SQL connection details	

    local_idam_plugin_interface = *idam_plugin_interface;

    local_idam_plugin_interface.sqlConnectionType = DBType;
    local_idam_plugin_interface.sqlConnection = (void*) DBConnect;

//----------------------------------------------------------------------------------------
// Functions 

    err = 0;

    do {        // Error Trap

//----------------------------------------------------------------------------------------
// HELP

        if (!strcasecmp(request_block->function, "help")) {

            err = help(&local_idam_plugin_interface);
            break;

        } else

//----------------------------------------------------------------------------------------
// GET a new Registered UUID

        if (!strcasecmp(request_block->function, "get") || !strcasecmp(request_block->function, "new")) {

            err = get(&local_idam_plugin_interface);
            break;

        } else

//----------------------------------------------------------------------------------------
// STATUS of a UUID

        if (!strcasecmp(request_block->function, "status")) {

            err = status(&local_idam_plugin_interface);
            break;

        } else

//----------------------------------------------------------------------------------------
// PUT Provenance metadata

        if (!strcasecmp(request_block->function, "put")) {

            err = put(&local_idam_plugin_interface);
            break;


        } else


//----------------------------------------------------------------------------------------
// putSignal

        if (!strcasecmp(request_block->function, "putSignal") ||
            !strcasecmp(request_block->function, "recordSignal") || !strcasecmp(request_block->function, "addSignal")) {

            err = putSignal(&local_idam_plugin_interface);
            break;

        } else

//----------------------------------------------------------------------------------------
// listSignals

        if (!strcasecmp(request_block->function, "listSignals") || !strcasecmp(request_block->function, "list")) {

            err = listSignals(&local_idam_plugin_interface);
            break;

        } else

//----------------------------------------------------------------------------------------
// Not a Known Function!

        idamLog(LOG_ERROR, "ERROR Provenance: Function %s Not Known.!\n", request_block->function);
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, "Unknown Function requested");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, request_block->function);
        break;

    } while (0);

#ifndef USE_PLUGIN_DIRECTLY    // Same structure if local unit test
    concatIdamError(idamerrorstack, idamErrorStack);
    closeIdamError(&idamerrorstack);
#endif

    return err;
}
