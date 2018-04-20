/*---------------------------------------------------------------
* IDAM Plugin to Access the IDAM Database DOI Table
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		issueDOI	0 if read was successful
*					otherwise a Error Code is returned 
*			DATA_BLOCK	Structure with Data from the File 
*      
Issues:
*
*---------------------------------------------------------------------------------------------------------------*/
#include "issueDOI.h"

#include <libpq-fe.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <server/udaServer.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>
#include <server/getServerEnvironment.h>

static const char* dbhost = NULL;
static char dbport[56];
static const char* dbname = NULL;
static const char* dbuser = NULL;
static char* pswrd = NULL;

static PGconn* startSQL_DOI()
{
    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

    // Login password is stored in .pgpass for POSTGRESQL database so no need to set

    PGconn* dbConn = NULL;

    //-------------------------------------------------------------
    // Debug Trace Queries

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection: host %s\n", dbhost);
    UDA_LOG(UDA_LOG_DEBUG, "                port %s\n", dbport);
    UDA_LOG(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    UDA_LOG(UDA_LOG_DEBUG, "                user %s\n", dbuser);

    //-------------------------------------------------------------
    // Connect to the Database Server

    if ((dbConn = PQsetdbLogin(dbhost, dbport, pgoptions, pgtty, dbname, dbuser, pswrd)) == NULL) {
        addIdamError(CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
        PQfinish(dbConn);
        return NULL;
    }

    if (PQstatus(dbConn) == CONNECTION_BAD) {
        addIdamError(CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
        PQfinish(dbConn);
        return NULL;
    }

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(dbConn));

    return dbConn;
}

// Replace the passed string with an Escaped String
// Free the Original string from Heap if requested
static bool preventSQLInjection(PGconn* dbConn, char** from, bool freeHeap)
{
    bool isOK = true;

    if (*from == NULL) return isOK;

    size_t fromCount = strlen(*from);
    char* to = (char*)malloc((2 * fromCount + 1) * sizeof(char));

    int err = 0;
    PQescapeStringConn(dbConn, to, *from, fromCount, &err);
    if (err != 0) {
        free((void*)to);
        return false;
    }

    if (freeHeap) {
        free((void*)*from);
    }

    *from = to;

    return isOK;
}

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn);
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn);
static int do_list(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn);

extern int issueDOI(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    unsigned short housekeeping = 0;

    static unsigned short DBType = PLUGINSQLNOTKNOWN;
    static PGconn* dbConn = NULL;

    if (idam_plugin_interface->interfaceVersion == 1) {
        idam_plugin_interface->pluginVersion = 1;
        housekeeping = idam_plugin_interface->housekeeping;
    } else {
        RAISE_PLUGIN_ERROR("Plugin Interface Version is Not Known: Unable to execute the request!")
    }

    UDA_LOG(UDA_LOG_DEBUG, "Plugin Interface transferred\n");

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    static bool init = false;
    static bool sqlPrivate = false;

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        UDA_LOG(UDA_LOG_DEBUG, "reset function called.\n");

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (dbConn != NULL && DBType == PLUGINSQLPOSTGRES && sqlPrivate) {
            UDA_LOG(UDA_LOG_DEBUG, "Closing SQL connection\n");
            PQfinish(dbConn);
            sqlPrivate = 1;        // Remains Private
            dbConn = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }

        init = false; // Ready to re-initialise
        UDA_LOG(UDA_LOG_DEBUG, "reset executed\n");
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise if requested (the previous private SQL connection must be closed)

    if (!STR_IEQUALS(request_block->function, "help") &&
            (!init || STR_IEQUALS(request_block->function, "init")
             || STR_IEQUALS(request_block->function, "initialise"))) {

        UDA_LOG(UDA_LOG_DEBUG, "init function called.\n");

        const ENVIRONMENT* environment = idam_plugin_interface->environment;
        dbhost = environment->sql_host;
        dbname = environment->sql_dbname;
        dbuser = environment->sql_user;
        sprintf(dbport, "%d", environment->sql_port);

        FIND_STRING_VALUE(request_block->nameValueList, dbhost);
        FIND_STRING_VALUE(request_block->nameValueList, dbname);
        FIND_STRING_VALUE(request_block->nameValueList, dbuser);

        int port;
        if (FIND_INT_VALUE(request_block->nameValueList, port)) {
            sprintf(dbport, "%d", port);
        }

        char* env = NULL;
        if ((env = getenv("UDA_DOIDBHOST")) != NULL) dbhost = env;
        if ((env = getenv("UDA_DOIDBPORT")) != NULL) strcpy(dbport, env);
        if ((env = getenv("UDA_DOIDBNAME")) != NULL) dbname = env;
        if ((env = getenv("UDA_DOIDBUSER")) != NULL) dbuser = env;
        if ((env = getenv("UDA_DOIDBPSWD")) != NULL) pswrd = env;

        // Is there an Open SQL Connection? If not then open a private connection

        if (dbConn == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)) {
            dbConn = startSQL_DOI();  // No prior connection to IDAM Postgres SQL Database
            if (dbConn != NULL) {
                DBType = PLUGINSQLPOSTGRES;
                sqlPrivate = 1;
                UDA_LOG(UDA_LOG_DEBUG, "Private regular database connection made.\n");
            }
        }

        if (dbConn == NULL) { // No connection!
            RAISE_PLUGIN_ERROR("SQL Database Server Connect Error");
        }

        init = true;

        UDA_LOG(UDA_LOG_DEBUG, "Plugin initialised and SQL connection made\n");

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Standard DOI specific Name Value pairs

    // Keywords have higher priority

    //----------------------------------------------------------------------------------------
    // Functions

    int err = 0;

    if (STR_IEQUALS(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "get") || STR_IEQUALS(request_block->function, "new")) {
        err = do_get(idam_plugin_interface, dbConn);
    } else if (STR_IEQUALS(request_block->function, "put") ||
               STR_IEQUALS(request_block->function, "record") || STR_IEQUALS(request_block->function, "add")) {
        err = do_put(idam_plugin_interface, dbConn);
    } else if (STR_IEQUALS(request_block->function, "list")) {
        err = do_list(idam_plugin_interface, dbConn);
    } else {
        RAISE_PLUGIN_ERROR("Unknown Function requested");
    }

    return err;
}

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    UDA_LOG(UDA_LOG_DEBUG, "entering function help\n");

    const char* help = "\nIssue a new DOI for a specific scientific study.\n\n"
            "get(owner=owner, icatRef=icatRef)\n\n"
            "Issue a new DOI with a pending status\n\n"
            "status(doi=doi, status=[Delete | Firm | Pending])\n\n"
            "Enquire about or Change a DOI's status\n\n"
            "list(doi=doi)\n\n"
            "put(user=user, doi=doi, requestedSignal=requestedSignal, requestedSource=requestedSource, \n"
            "trueSignal=trueSignal, trueSource=trueSource, trueSourceDOI=trueSourceDOI, \n"
            "logRecord=logRecord, created=created, status=[New|Update|Close|Delete])\n\n";

    UDA_LOG(UDA_LOG_DEBUG, "issueDOI:\n%s\n", help);

    // Create the Returned Structure Definition

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    USERDEFINEDTYPE usertype;

    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "DOIHELP");
    strcpy(usertype.source, "DOI:issueDOI");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(DOIHELP);            // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    defineField(&field, "help", "Information about the issueDOI plugin functions", &offset,
                SCALARSTRING);    // Single string, arbitrary length
    addCompoundField(&usertype, field);
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    DOIHELP* data;
    data = (DOIHELP*)malloc(sizeof(DOIHELP));
    data->value = strdup(help);

    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;
    addMalloc(logmalloclist, (void*)data, 1, sizeof(DOIHELP), "DOIHELP");
    addMalloc(logmalloclist, (void*)data->value, 1, strlen(help) * sizeof(char), "char");

    // Pass Data

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "issueDOI Plugin help");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DOIHELP", 0);

    UDA_LOG(UDA_LOG_DEBUG, "exiting function help\n");
    if (data_block->opaque_block == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "DOIHELP type not found\n");
    }

    return 0;
}

//----------------------------------------------------------------------------------------
// NEW - issue a new DOI
//
// DOI table contains the following
//
// DOI 		Unique Identifier: Prefix+'/'+Year+'/'+Ordinal Date+'/'+Seconds+'/'+Microseconds
// Owner	name of the user or group that owns the DOI
// icatRef	ICAT reference Id (foreign key)
// status	DOI status [delete | firm | pending]
static int do_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn)
{
    UDA_LOG(UDA_LOG_DEBUG, "entering function new\n");

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* owner = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, owner);

    const char* icatRef = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, icatRef);

    if (!preventSQLInjection(dbConn, (char**)&owner, false)) {
        RAISE_PLUGIN_ERROR("argument owner incorrectly formatted");
    }
    if (!preventSQLInjection(dbConn, (char**)&icatRef, false)) {
        RAISE_PLUGIN_ERROR("argument catRef incorrectly formatted");
    }

    // Performance

    struct timeval tv_start, tv_stop;
    int msecs, usecs;
    gettimeofday(&tv_start, NULL);

    // Create the Data Structure to be returned

    ISSUEDOI* data = NULL;
    data = (ISSUEDOI*)malloc(sizeof(ISSUEDOI));    // Structured Data Must be a heap variable

    // Create the Unique Identifier

    struct timeval tv_uid;
    gettimeofday(&tv_uid, NULL);

    int microsecs = (int)tv_uid.tv_usec;    // If this is loo low resolution, use struct timespec (nano-secs)
    int secs = (int)tv_uid.tv_sec;
    struct tm* uid_time = localtime((time_t*)&secs);

    char year[5];
    char day[4];
    char hms[9];    // hh/mm/ss

    strftime(year, sizeof(year), "%Y", uid_time);        // 4 digit year
    strftime(day, sizeof(day), "%j", uid_time);        // 3 digit day of year
    strftime(hms, sizeof(hms), "%H/%M/%S", uid_time);    // Hour/Minute/Second

    char* env = NULL;
    char work[MAXSQL];
    if ((env = getenv("UDA_DOIPREFIX")) != NULL) {
        sprintf(work, "%s/%s/%s/%s/%06d", env, year, day, hms, microsecs);
    } else {
        sprintf(work, "%s/%s/%s/%06d", year, day, hms, microsecs);
    }

    gettimeofday(&tv_stop, NULL);
    UDA_LOG(UDA_LOG_DEBUG, "new() Unique ID = '%s'\n", work);
    msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
            (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
    usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int)(tv_stop.tv_usec - tv_start.tv_usec);
    UDA_LOG(UDA_LOG_DEBUG, "new() Unique ID Cost = %d (ms), %d (microsecs)\n", msecs, usecs);
    tv_start = tv_stop;

    // Create Transaction Block SQL

    char sql[MAXSQL];
    sprintf(sql, "BEGIN; INSERT INTO doi_table (doi, owner, icatref) VALUES ('%s', '%s','%s'); END;",
            work, owner, icatRef);

    UDA_LOG(UDA_LOG_DEBUG, "%s\n", sql);

    // execute

    PGresult* dbQuery = NULL;
    if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
        PQclear(dbQuery);
        RAISE_PLUGIN_ERROR("SQL Execution Failed!");
    }

    PQclear(dbQuery);

    gettimeofday(&tv_stop, NULL);
    msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
            (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
    usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int)(tv_stop.tv_usec - tv_start.tv_usec);
    UDA_LOG(UDA_LOG_DEBUG, "new() SQL Cost = %d (ms), %d (microsecs)\n", msecs, usecs);

    // Write Return Structure

    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;

    size_t stringLength = strlen(work) + 1;
    data->doi = (char*)malloc(stringLength * sizeof(char));
    strcpy(data->doi, work);
    addMalloc(logmalloclist, (void*)data->doi, 1, stringLength * sizeof(char), "char");

    stringLength = strlen(owner) + 1;
    data->owner = (char*)malloc(stringLength * sizeof(char));
    strcpy(data->owner, owner);
    addMalloc(logmalloclist, (void*)data->owner, 1, stringLength * sizeof(char), "char");

    stringLength = strlen(icatRef) + 1;
    data->icatRef = (char*)malloc(stringLength * sizeof(char));
    strcpy(data->icatRef, icatRef);
    addMalloc(logmalloclist, (void*)data->icatRef, 1, stringLength * sizeof(char), "char");

    data->status = ' ';

    UDA_LOG(UDA_LOG_DEBUG, "issueDOI doi: %s\n", data->doi);
    UDA_LOG(UDA_LOG_DEBUG, "owner       : %s\n", data->owner);
    UDA_LOG(UDA_LOG_DEBUG, "icatRefId   : %s\n", data->icatRef);
    UDA_LOG(UDA_LOG_DEBUG, "Status      : %c\n", data->status);

    // the Returned Structure Definition

    data->structVersion = 1;                // This structure's version number

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "ISSUEDOI");
    usertype.size = sizeof(ISSUEDOI);

    strcpy(usertype.source, "issueDOI");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    defineField(&field, "structVersion", "This Data Structure's version number", &offset, SCALARUSHORT);
    addCompoundField(&usertype, field);

    defineField(&field, "doi", "Digital Object Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "owner", "DOI Owner", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "icatRef", "ICAT Reference ID", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "status", "DOI Status", &offset, SCALARCHAR);
    addCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Register the Pointer to the Data Structure

    addMalloc(logmalloclist, (void*)data, 1, sizeof(ISSUEDOI), "ISSUEDOI");

    // Pass the Data back

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "issue a new DOI");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "ISSUEDOI", 0);

    UDA_LOG(UDA_LOG_DEBUG, "exiting function new\n");

    return 0;
}

//----------------------------------------------------------------------------------------
// Record data access requets in a temporary database and manage the status of the records:
//
// put(user=user, doi=doi, requestedSignal=requestedSignal, requestedSource=requestedSource,
//	trueSignal=trueSignal, trueSource=trueSource, trueSourceDOI=trueSourceDOI,
//	logRecord=logRecord, created=created, status=[new|update|close|delete], execMethod=execMethod)
//
// DOI_Log table contains the following
//
// DOI_Log_Key		Primary key
// DOI 			    DOI reference
// requestedSignal	Requested data signal name
// requestedSource 	Requested data source name
// trueSignal		Signal within the source file
// trueSource		File path and name
// trueSourceDOI	Data source file's DOI
// logRecord		Log record of the data access
// created		    date record was written
// status		    0=>open, 1=>closed (Closed when requested by client application)
// execMethod = 1	Execute the SQL commands in the background immediately
//			        Unreliable as often the log record is missing - perhaps executed before the record is created.
// execMethod = 2	Collect the SQL commands together into a file and execute at the end - when
//			        the log record is written. This method is only employed for ADD and UPDATE.
//			        Require automatic purge of sql command files
// execMethod = 3	Collect the SQL commands together into a static string buffer and execute at the end - when
//			        the log record is written. This method is only employed for ADD and UPDATE. No files to delete!
//
// The cost of writing to the database after each individual data access is high. This can be mitigated by
// writing SQL commands to a file and executing these together at the end of the process. The DOI can be
// used to identify this file. Alternatively, each SQL command can be executed via a command pipe in the
// background without blocking. This also has a time cost as a command shell must be forked.

// The cost of method 1 ~5-10 ms	default server setting
// The cost of method 2 ~3-5  ms
// The cost of method 3 ?     ms

// The default is over-ruled in server startup script by IDAM_PROVENANCE_EXEC_METHOD
// The passed keyword execMethod has highest priority
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn)
{
    static struct timeval tv_start, tv_stop;    // Performance
    int msecs, usecs;

    UDA_LOG(UDA_LOG_DEBUG, "entering function record\n");

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

// Name Value pairs (Keywords have higher priority)

    const char* doi = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, doi);

    const char* requestedSignal = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, requestedSignal);

    const char* requestedSource = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, requestedSource);

    const char* trueSignal = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, trueSignal);

    const char* trueSource = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, trueSource);

    const char* trueSourceDOI = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, trueSourceDOI);

    const char* logRecord = NULL;
    bool isLogRecord = FIND_STRING_VALUE(request_block->nameValueList, logRecord);

    char status = '?';
    FIND_CHAR_VALUE(request_block->nameValueList, status);

    short execMethod = 0;
    bool isExecMethod = FIND_SHORT_VALUE(request_block->nameValueList, execMethod);

    char* env = NULL;
    if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != NULL) {
        execMethod = (unsigned short)atoi(env);
    }

    if (!preventSQLInjection(dbConn, (char**)&doi, false))
        RAISE_PLUGIN_ERROR("argument doi incorrectly formattted");
    if (!preventSQLInjection(dbConn, (char**)&requestedSignal, false))
        RAISE_PLUGIN_ERROR("argument requestedSignal incorrectly formattted");
    if (!preventSQLInjection(dbConn, (char**)&requestedSource, false))
        RAISE_PLUGIN_ERROR("argument requestedSource incorrectly formattted");
    if (!preventSQLInjection(dbConn, (char**)&trueSignal, false))
        RAISE_PLUGIN_ERROR("argument trueSignal incorrectly formattted");
    if (!preventSQLInjection(dbConn, (char**)&trueSourceDOI, false))
        RAISE_PLUGIN_ERROR("argument trueSourceDOI incorrectly formattted");
    if (!preventSQLInjection(dbConn, (char**)&logRecord, false))
        RAISE_PLUGIN_ERROR("argument logRecord incorrectly formattted");

    UDA_LOG(UDA_LOG_DEBUG, "passed parameters\n");
    UDA_LOG(UDA_LOG_DEBUG, "DOI = %s\n", doi);
    UDA_LOG(UDA_LOG_DEBUG, "requestedSignal = %s\n", requestedSignal);
    UDA_LOG(UDA_LOG_DEBUG, "requestedSource = %s\n", requestedSource);
    UDA_LOG(UDA_LOG_DEBUG, "trueSignal = %s\n", trueSignal);
    UDA_LOG(UDA_LOG_DEBUG, "trueSource = %s\n", trueSource);
    UDA_LOG(UDA_LOG_DEBUG, "trueSourceDOI = %s\n", trueSourceDOI);
    UDA_LOG(UDA_LOG_DEBUG, "logRecord = %s\n", logRecord);
    UDA_LOG(UDA_LOG_DEBUG, "Status = %c\n", status);
    UDA_LOG(UDA_LOG_DEBUG, "execMethod = %d\n", execMethod);

    // 1> Create a new record if status == new
    // 2> add log data to an existing record if status == update
    // 3> close status for all records if status == close

    static FILE* sqlSet = NULL;
    static char* key = NULL;
    static char* tmpfile = NULL;
    static unsigned short keySeq = 0;

    if (status == 'n') {    // Create a new record and reset the current primary key in scope
        UDA_LOG(UDA_LOG_DEBUG, "record() Create a new record\n");

        if (key != NULL) {    // Always renewed for each new record
            free((void*)key);
            key = NULL;
        }

        if (isExecMethod) {
            time_t initTime = time(NULL);

            // No Primary Key returned so must create a unique key - use the randomised component of the file name + User's DOI

            char work[MAXSQL];
            sprintf(work, "%s/%ld/%d", doi, (long)initTime, keySeq++);    // Create a unique key
            key = (char*)malloc((strlen(work) + 1) * sizeof(char));
            strcpy(key, work);

            if (execMethod == 1) {

                // Login password is stored in .pgpass for POSTGRESQL database so no need to set

                char cmd[MAXSQL];
                if ((env = getenv("UDA_CLI_SQL")) != NULL) {
                    strcpy(cmd, env);                    // Command line sql utility
                } else {
                    strcpy(cmd, "psql");
                }

                sprintf(&cmd[strlen(cmd)],
                        " -d %s -U %s -h %s -p %s -c \""
                                "INSERT INTO doi_log "
                                "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                                "trueSourceDOI, tmpKey) "
                                "VALUES ('%s','%s','%s','%s','%s','%s','%s');\" > /dev/null 2>&1 &",
                        dbname, dbuser, dbhost, dbport,
                        doi, requestedSignal, requestedSource, trueSignal, trueSource,
                        trueSourceDOI, key);

                UDA_LOG(UDA_LOG_DEBUG, "record() SQL\n%s\n", cmd);

                gettimeofday(&tv_start, NULL);

                errno = 0;
                FILE* ph = popen(cmd, "r");

                if (ph == NULL || errno != 0) {
                    RAISE_PLUGIN_ERROR("Cannot execute background SQL command");
                }

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 1 Cost A = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            } else if (execMethod == 2) {

                // File directory

                char* tmpdir = getenv("UDA_WORK_DIR");

                // Create a temporary file to collect the SQL commands

                if (tmpdir != NULL) {
                    tmpfile = (char*)malloc((strlen(tmpdir) + 11) * sizeof(char));
                    sprintf(tmpfile, "%s/sqlXXXXXX", tmpdir);
                } else {
                    tmpfile = (char*)malloc(16 * sizeof(char));
                    strcpy(tmpfile, "/tmp/sqlXXXXXX");
                }

                gettimeofday(&tv_start, NULL);

                errno = 0;
                if (mkstemp(tmpfile) < 0 || errno != 0) {
                    RAISE_PLUGIN_ERROR("Unable to Obtain a Temporary File Name");
                }

                if ((sqlSet = fopen(tmpfile, "w")) == NULL) {
                    RAISE_PLUGIN_ERROR("Unable to Open a Temporary File");
                }

                fprintf(sqlSet, "BEGIN; INSERT INTO doi_log "
                                "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                                "trueSourceDOI, tmpKey) "
                                "VALUES ('%s','%s','%s','%s','%s','%s','%s');",
                        doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI, key);

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 2 Cost A = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            } else if (execMethod == 3) {

                gettimeofday(&tv_start, NULL);

                char sqlBuffer[MAXSQL];
                sprintf(sqlBuffer, "BEGIN; INSERT INTO doi_log "
                        "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                        "trueSourceDOI, logRecord) "
                        "VALUES ('%s','%s','%s','%s','%s','%s',",
                        doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI);

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 3 Cost A = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            } else {
                RAISE_PLUGIN_ERROR("Incorrect execMethod argument {1|2|3}");
            }
        } else {
            // Expensive & blocking database write method

            char sql[MAXSQL];
            sprintf(sql, "BEGIN; "
                    "INSERT INTO doi_log "
                    "(doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI) "
                    "VALUES ('%s','%s','%s','%s','%s','%s');",
                    doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI);

            UDA_LOG(UDA_LOG_DEBUG, "record() SQL\n%s\n", sql);
            // Execute the SQL

            PGresult* dbQuery;
            if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
                RAISE_PLUGIN_ERROR("database query failed");
            }

            PQclear(dbQuery);

            // Return the Primary Key

            strcpy(sql, "SELECT doi_log_id FROM doi_log WHERE "
                    "doi_log_id=currval('doi_log_id_seq');");

            UDA_LOG(UDA_LOG_DEBUG, "%s\n", sql);

            if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_TUPLES_OK) {
                RAISE_PLUGIN_ERROR("database query failed");
            }

            int nrows = PQntuples(dbQuery);

            if (nrows != 1) {
                RAISE_PLUGIN_ERROR("New doi_log record not found!");
                UDA_LOG(UDA_LOG_ERROR, "ERROR issueDOI new: New doi_log record not found!\n");
            }

            // Extract the SQL data

            size_t stringLength = strlen(PQgetvalue(dbQuery, 0, 0)) + 1;
            key = (char*)malloc(stringLength * sizeof(char));
            strcpy(key, PQgetvalue(dbQuery, 0, 0));
            PQclear(dbQuery);

            UDA_LOG(UDA_LOG_DEBUG, "issueDOI key: %s\n", key);

            // Complete the transaction

            sprintf(sql, "END;");

            if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
                free((void*)key);
                key = NULL;
                RAISE_PLUGIN_ERROR("database query failed");
            }

            free((void*)key);
            PQclear(dbQuery);
        }

    } else if (status == 'u') {
        // update an existing record using the key from the ADD step
        UDA_LOG(UDA_LOG_DEBUG, "record() update an existing record with the Server Log record\n");

        if (!isLogRecord) {
            RAISE_PLUGIN_ERROR("No Log record!");
        }

        if (isExecMethod) {
            if (execMethod == 1) {

                char cmd[MAXSQL];
                if ((env = getenv("UDA_CLI_SQL")) != NULL) {
                    strcpy(cmd, env);
                } else {
                    strcpy(cmd, "psql");
                }

                sprintf(&cmd[strlen(cmd)],
                        " -d %s -U %s -h %s -p %s -c \""
                                "UPDATE doi_log SET logRecord = '%s' WHERE tmpKey = '%s';\" > /dev/null 2>&1 &",
                        dbname, dbuser, dbhost, dbport, logRecord, key);

                UDA_LOG(UDA_LOG_DEBUG, "update() SQL\n%s\n", cmd);

                gettimeofday(&tv_start, NULL);

                errno = 0;
                FILE* ph = popen(cmd, "r");

                if (ph == NULL || errno != 0) {
                    RAISE_PLUGIN_ERROR("Cannot execute background SQL command");
                }

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 1 Cost B = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            } else if (execMethod == 2) {
                gettimeofday(&tv_start, NULL);

                fprintf(sqlSet, "UPDATE doi_log SET logRecord = '%s' WHERE tmpKey = '%s'; END;", logRecord,
                        key);
                fclose(sqlSet);

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 2 Cost B = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;

                char work[MAXSQL];
                if ((env = getenv("UDA_CLI_SQL")) != NULL) {
                    strcpy(work, env);
                } else {
                    strcpy(work, "psql");
                }

                char cmd[MAXSQL];
                sprintf(cmd,
                        "%s -d %s -U %s -h %s -p %s -f %s > /dev/null 2>&1 &",
                        work, dbname, dbuser, dbhost, dbport, tmpfile);

                UDA_LOG(UDA_LOG_DEBUG, "update() SQL\n%s\n", cmd);

                gettimeofday(&tv_start, NULL);

                errno = 0;
                FILE* ph = popen(cmd, "r");

                if (ph == NULL || errno != 0) {
                    RAISE_PLUGIN_ERROR("Cannot execute background SQL command");
                }

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 2 Cost C = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            } else if (execMethod == 3) {

                gettimeofday(&tv_start, NULL);

                char work[MAXSQL];
                if ((env = getenv("UDA_CLI_SQL")) != NULL) {
                    strcpy(work, env);
                } else {
                    strcpy(work, "psql");
                }

                char cmd[MAXSQL];
                char sqlBuffer[MAXSQL];
                sprintf(cmd,
                        "\"%s '%s'); END;\" > /dev/null 2>&1 &",
                        sqlBuffer, logRecord);

                UDA_LOG(UDA_LOG_DEBUG, "update() SQL\n%s\n", cmd);

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 3 Cost B = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;

                errno = 0;
                FILE* ph = popen(cmd, "r");

                if (ph == NULL || errno != 0) {
                    RAISE_PLUGIN_ERROR("Cannot execute background SQL command");
                }

                gettimeofday(&tv_stop, NULL);
                msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                usecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                        (int)(tv_stop.tv_usec - tv_start.tv_usec);
                UDA_LOG(UDA_LOG_DEBUG, "update() execMethod 3 Cost C = %d (ms), %d (microsecs)\n",
                          msecs, usecs);
                tv_start = tv_stop;
            }
        } else {

            char sql[MAXSQL];
            sprintf(sql, "BEGIN; "
                    "UPDATE doi_log SET logRecord = '%s' WHERE doi_log_id = %s;"
                    "END;", logRecord, key);

            UDA_LOG(UDA_LOG_DEBUG, "record() SQL\n%s\n", sql);

            // Execute the SQL

            PGresult* dbQuery = NULL;
            if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
                PQclear(dbQuery);
                RAISE_PLUGIN_ERROR("doi_log table update failed!");
            }

            PQclear(dbQuery);
        }

    } else if (status == 'c') {
        // close all records for future deletion and execute collected SQL statements
        UDA_LOG(UDA_LOG_DEBUG, "record() Close all records\n");

        char sql[MAXSQL];
        sprintf(sql, "BEGIN; "
                "UPDATE doi_log SET status = 1 WHERE doi = '%s';"
                "END;", doi);

        UDA_LOG(UDA_LOG_DEBUG, "record() SQL\n%s\n", sql);

        // Execute the SQL

        PGresult* dbQuery = NULL;
        if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
            PQclear(dbQuery);
            RAISE_PLUGIN_ERROR("doi_log status update failed!");
        }

        PQclear(dbQuery);

    } else if (status == 'd') {
        // Delete closed records (Protection against malicious intent? user field?)
        UDA_LOG(UDA_LOG_DEBUG, "record() Delete closed records\n");

        char sql[MAXSQL];
        sprintf(sql, "BEGIN; "
                "DELETE FROM doi_log WHERE status = 1 AND doi = '%s';" // and user='%s'
                "END;", doi);

        UDA_LOG(UDA_LOG_DEBUG, "record() SQL\n%s\n", sql);

        // Execute the SQL

        PGresult* dbQuery = NULL;
        if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
            PQclear(dbQuery);
            RAISE_PLUGIN_ERROR("doi_log deletion failed!");
        }

        PQclear(dbQuery);
    }

//    if (err != 0 && (!isExecMethod || status == 'd')) {    // Rollback the Transaction
//
//        if (err == 1) {
//            err = 999;
//            if (dbQuery == NULL) {
//                UDA_LOG(UDA_LOG_ERROR, "ERROR issueDOI add: Database Query Failed!\n");
//                addIdamError(CODEERRORTYPE, "issueDOI add", err, "Database Query Failed!");
//            } else if (PQresultStatus(dbQuery) != PGRES_COMMAND_OK) {
//                UDA_LOG(UDA_LOG_ERROR, "ERROR issueDOI add: %s\n", PQresultErrorMessage(dbQuery));
//                addIdamError(CODEERRORTYPE, "issueDOI add", err,
//                             PQresultErrorMessage(dbQuery));
//            }
//        }
//
//        PQclear(dbQuery);
//
//        sprintf(sql, "ROLLBACK; END;");
//        UDA_LOG(UDA_LOG_DEBUG, "%s\n", sql);
//
//        dbQuery = PQexec(dbConn, sql);
//        PQclear(dbQuery);
//    }

    // Return

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)malloc(sizeof(int));
    int* pi = (int*)data_block->data;
    pi[0] = 0;

    strcpy(data_block->data_desc, "data access logged against DOI");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    UDA_LOG(UDA_LOG_DEBUG, "exiting function record\n");

    return 0;
}

//----------------------------------------------------------------------------------------
// LIST all data access records from the DOI_Log table
static int do_list(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* dbConn)
{
    UDA_LOG(UDA_LOG_DEBUG, "entering function list\n");

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* doi;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, doi);

    if (!preventSQLInjection(dbConn, (char**)&doi, false)) {
        RAISE_PLUGIN_ERROR("argument DOI incorrectly formatted");
    }

    char sql[MAXSQL];
    sprintf(sql, "SELECT doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI, "
            "logRecord, creation FROM doi_log WHERE doi='%s';", doi);

    UDA_LOG(UDA_LOG_DEBUG, "list() SQL\n%s\n", sql);

    // Execute the SQL

    PGresult* dbQuery = NULL;
    if ((dbQuery = PQexec(dbConn, sql)) == NULL || PQresultStatus(dbQuery) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR("SQL Failed");
    }

    int nrows = PQntuples(dbQuery);

    if (nrows == 0) {
        RAISE_PLUGIN_ERROR("No doi_log records found");
    }

    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;

    // Create the Data Structures to be returned

    DOIRECORD* data = (DOIRECORD*)malloc(nrows * sizeof(DOIRECORD));

    addMalloc(logmalloclist, (void*)data, nrows, sizeof(DOIRECORD), "DOIRECORD");

    LISTDOI* list = (LISTDOI*)malloc(1 * sizeof(LISTDOI));

    list->structVersion = 1;
    list->count = nrows;
    list->doi = (char*)malloc((strlen(doi) + 1) * sizeof(char));
    strcpy(list->doi, doi);
    list->list = data;

    addMalloc(logmalloclist, (void*)list, 1, sizeof(LISTDOI), "LISTDOI");
    addMalloc(logmalloclist, (void*)list->doi, 1, (strlen(doi) + 1) * sizeof(char), "char");

    // Extract the SQL data

    int i;
    for (i = 0; i < nrows; i++) {
        size_t stringLength = strlen(PQgetvalue(dbQuery, i, 0)) + 1;
        data[i].doi = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].doi, PQgetvalue(dbQuery, i, 0));
        addMalloc(logmalloclist, (void*)data[i].doi, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 1)) + 1;
        data[i].requestedSignal = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].requestedSignal, PQgetvalue(dbQuery, i, 1));
        addMalloc(logmalloclist, (void*)data[i].requestedSignal, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 2)) + 1;
        data[i].requestedSource = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].requestedSource, PQgetvalue(dbQuery, i, 2));
        addMalloc(logmalloclist, (void*)data[i].requestedSource, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 3)) + 1;
        data[i].trueSignal = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].trueSignal, PQgetvalue(dbQuery, i, 3));
        addMalloc(logmalloclist, (void*)data[i].trueSignal, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 4)) + 1;
        data[i].trueSource = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].trueSource, PQgetvalue(dbQuery, i, 4));
        addMalloc(logmalloclist, (void*)data[i].trueSource, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 5)) + 1;
        data[i].trueSourceDOI = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].trueSourceDOI, PQgetvalue(dbQuery, i, 5));
        addMalloc(logmalloclist, (void*)data[i].trueSourceDOI, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 6)) + 1;
        data[i].logRecord = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].logRecord, PQgetvalue(dbQuery, i, 6));
        addMalloc(logmalloclist, (void*)data[i].logRecord, 1, stringLength * sizeof(char), "char");

        stringLength = strlen(PQgetvalue(dbQuery, i, 7)) + 1;
        data[i].creation = (char*)malloc(stringLength * sizeof(char));
        strcpy(data[i].creation, PQgetvalue(dbQuery, i, 7));
        addMalloc(logmalloclist, (void*)data[i].creation, 1, stringLength * sizeof(char), "char");

        UDA_LOG(UDA_LOG_DEBUG, "doi            : %s\n\n", data[i].doi);
        UDA_LOG(UDA_LOG_DEBUG, "requestedSignal: %s\n", data[i].requestedSignal);
        UDA_LOG(UDA_LOG_DEBUG, "requestedSource: %s\n", data[i].requestedSource);
        UDA_LOG(UDA_LOG_DEBUG, "trueSignal     : %s\n", data[i].trueSignal);
        UDA_LOG(UDA_LOG_DEBUG, "trueSource     : %s\n", data[i].trueSource);
        UDA_LOG(UDA_LOG_DEBUG, "trueSourceDOI  : %s\n", data[i].trueSourceDOI);
        UDA_LOG(UDA_LOG_DEBUG, "logRecord      : %s\n", data[i].logRecord);
        UDA_LOG(UDA_LOG_DEBUG, "creation date  : %s\n", data[i].creation);
    }
    PQclear(dbQuery);

    // the Returned Structure Definition

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "DOIRECORD");
    usertype.size = sizeof(DOIRECORD);

    strcpy(usertype.source, "DOI Plugin");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    defineField(&field, "doi", "Digital Object Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "requestedSignal", "requested Signal", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "requestedSource", "requested Source", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "trueSignal", "true Signal", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "trueSource", "true Source", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "trueSourceDOI", "true Source DOI", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "logRecord", " IDAM log Record", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    defineField(&field, "creation", "record creation date", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "LISTDOI");
    usertype.size = sizeof(LISTDOI);

    strcpy(usertype.source, "DOI Plugin");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    defineField(&field, "structVersion", "This Data Structure's version number", &offset, SCALARUSHORT);
    addCompoundField(&usertype, field);

    defineField(&field, "count", "The number of records in the list", &offset, SCALARUSHORT);
    addCompoundField(&usertype, field);

    defineField(&field, "doi", "Digital Object Identifier", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "list");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "DOIRECORD");
    strcpy(field.desc, "List of Records");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;                // Needed when rank >= 1
    field.size = field.count * sizeof(DOIRECORD);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;        // Next Offset

    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    // Pass the Data back

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)list;

    strcpy(data_block->data_desc, "List of Signals accessed with a DOI");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "LISTDOI", 0);

    UDA_LOG(UDA_LOG_DEBUG, "Function list called\n");

    if (data_block->opaque_block == NULL) {
        RAISE_PLUGIN_ERROR("failed to locate LISTDOI structure!");
    }

    return 0;
}
