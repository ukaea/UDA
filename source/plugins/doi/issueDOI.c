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

#include <include/idamserver.h>
#include <structures/struct.h>
#include <structures/accessors.h>

static char* pghost = NULL;
static char pgport[56];
static char* dbname = NULL;
static char* user = NULL;
static char* pswrd = NULL;

static PGconn* startSQL_DOI()
{
    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

// Login password is stored in .pgpass for POSTGRESQL database so no need to set

    PGconn* DBConnect = NULL;

//------------------------------------------------------------- 
// Debug Trace Queries

    idamLog(LOG_DEBUG, "SQL Connection: host %s\n", pghost);
    idamLog(LOG_DEBUG, "                port %s\n", pgport);
    idamLog(LOG_DEBUG, "                db   %s\n", dbname);
    idamLog(LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

    idamLog(LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return (DBConnect);
}

static int preventSQLInjection(PGconn* DBConnect, char** from, unsigned short freeHeap)
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

extern int issueDOI(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    int err = 0;
    static short init = 0;
    static short sqlPrivate = 1;            // The SQL connection is private and is not passed back.

    static int initTime = 0;
    //int j, ncols, col;
    int i, offset = 0, nrows;

    char* env;
    char work[MAXSQL];
    char sql[MAXSQL];
    int stringLength;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;
    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

#ifndef USE_PLUGIN_DIRECTLY
    IDAMERRORSTACK idamerrorstack;
    IDAMERRORSTACK* idamErrorStack = getIdamServerPluginErrorStack();        // Server library functions
    USERDEFINEDTYPELIST* userdefinedtypelist = getIdamServerUserDefinedTypeList();

    initIdamErrorStack(&idamerrorstack);
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;
#endif

    unsigned short housekeeping = 0;

    static unsigned short DBType = PLUGINSQLNOTKNOWN;
    static PGconn* DBConnect = NULL;

    PGresult* DBQuery = NULL;
    //ExecStatusType DBQueryStatus;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

#ifndef USE_PLUGIN_DIRECTLY
// Don't copy the structure if housekeeping is requested - may dereference a NULL or freed pointer!     
    if (!housekeeping && idam_plugin_interface->environment != NULL) environment = *idam_plugin_interface->environment;
#endif

        housekeeping = idam_plugin_interface->housekeeping;

        DBType = PLUGINSQLNOTKNOWN;
        //DBConnect = NULL;			// Always use a private connection

    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR issueDOI: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    idamLog(LOG_DEBUG, "issueDOI: Plugin Interface transferred\n");


//----------------------------------------------------------------------------------------
// Heap Housekeeping 

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        idamLog(LOG_DEBUG, "issueDOI: reset function called.\n");

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (DBConnect != NULL && DBType == PLUGINSQLPOSTGRES && sqlPrivate) {
            idamLog(LOG_DEBUG, "issueDOI: Closing SQL connection\n");
            PQfinish(DBConnect);
            sqlPrivate = 1;        // Remains Private
            DBConnect = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }
        init = 0;        // Ready to re-initialise
        idamLog(LOG_DEBUG, "issueDOI: reset executed\n");
        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise if requested (the previous private SQL connection must be closed) 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        idamLog(LOG_DEBUG, "issueDOI: init function called.\n");

        pghost = environment.sql_host;
        dbname = environment.sql_dbname;
        user = environment.sql_user;

        sprintf(pgport, "%d", environment.sql_port);

        if ((env = getenv("UDA_DOIDBHOST")) != NULL) pghost = env;
        if ((env = getenv("UDA_DOIDBPORT")) != NULL) strcpy(pgport, env);
        if ((env = getenv("UDA_DOIDBNAME")) != NULL) dbname = env;
        if ((env = getenv("UDA_DOIDBUSER")) != NULL) user = env;
        if ((env = getenv("UDA_DOIDBPSWD")) != NULL) pswrd = env;

// Is there an Open SQL Connection? If not then open a private connection

        if (DBConnect == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)) {
            DBConnect = (PGconn*) startSQL_DOI();        // No prior connection to IDAM Postgres SQL Database
            if (DBConnect != NULL) {
                DBType = PLUGINSQLPOSTGRES;
                sqlPrivate = 1;
                idamLog(LOG_DEBUG, "issueDOI: Private regular database connection made.\n");
            }
        }

        if (DBConnect == NULL) {        // No connection!
            idamLog(LOG_ERROR, "ERROR issueDOI: SQL Database Server Connect Error\n");
            err = 777;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, "SQL Database Server Connect Error");
            concatIdamError(idamerrorstack, idamErrorStack);
            return err;
        }

        initTime = (int) time(NULL);    // used as a unique component of a key field

        init = 1;

        idamLog(LOG_DEBUG, "issueDOI: Plugin initialised and SQL connection made\n");

        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

//----------------------------------------------------------------------------------------
// Standard DOI specific Name Value pairs

// Keywords have higher priority

//----------------------------------------------------------------------------------------
// Functions 

    err = 0;

    do {        // Error Trap

//----------------------------------------------------------------------------------------
// Help: A Description of library functionality
// Information is returned as a single string containing all required format control characters.

// issueDOI::help()		

        if (!strcasecmp(request_block->function, "help")) {

            idamLog(LOG_DEBUG, "issueDOI: entering function help\n");

            strcpy(work, "\nissueDOI: Issue a new DOI for a specific scientific study.\n\n"

                    "get(owner=owner, icatRef=icatRef)\n\n"
                    "Issue a new DOI with a pending status\n\n"

                    "status(doi=doi, status=[Delete | Firm | Pending])\n\n"
                    "Enquire about or Change a DOI's status\n\n"

                    "list(doi=doi)\n\n"

                    "put(user=user, doi=doi, requestedSignal=requestedSignal, requestedSource=requestedSource, \n"
                    "trueSignal=trueSignal, trueSource=trueSource, trueSourceDOI=trueSourceDOI, \n"
                    "logRecord=logRecord, created=created, status=[New|Update|Close|Delete])\n\n"
            );

            idamLog(LOG_DEBUG, "issueDOI:\n%s\n", work);

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "DOIHELP");
            strcpy(usertype.source, "DOI:issueDOI");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(DOIHELP);            // Structure size
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "help", "Information about the issueDOI plugin functions", &offset,
                        SCALARSTRING);    // Single string, arbitrary length
            addCompoundField(&usertype, field);
            addUserDefinedType(userdefinedtypelist, usertype);

// Create Data	 

            DOIHELP* data;
            stringLength = strlen(work) + 1;
            data = (DOIHELP*) malloc(sizeof(DOIHELP));
            data->value = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->value, work);
            addMalloc((void*) data, 1, sizeof(DOIHELP), "DOIHELP");
            addMalloc((void*) data->value, 1, stringLength * sizeof(char), "char");

// Pass Data	 

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "issueDOI Plugin help");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("DOIHELP", 0);

            idamLog(LOG_DEBUG, "issueDOI: exiting function help\n");
            if (data_block->opaque_block == NULL) idamLog(LOG_DEBUG, "issueDOI: DOIHELP type not found\n");

            break;

        } else

//----------------------------------------------------------------------------------------
// NEW - issue a new DOI
//
// DOI table contains the following
// 
// DOI 		Unique Identifier: Prefix+'/'+Year+'/'+Ordinal Date+'/'+Seconds+'/'+Microseconds
// Owner	name of the user or group that owns the DOI
// icatRef	ICAT reference Id (foreign key)
// status	DOI status [delete | firm | pending]

        if (!strcasecmp(request_block->function, "get") || !strcasecmp(request_block->function, "new")) {

            idamLog(LOG_DEBUG, "issueDOI: entering function new\n");

            char* owner, * icatRef;
            unsigned short ownerOK = 0, icatRefOK = 0;

// Standard DOI specific Name Value pairs (Keywords have higher priority)

            for (i = 0; i < request_block->nameValueList.pairCount; i++) {

                idamLog(LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                        request_block->nameValueList.nameValue[i].value);

                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "owner")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    owner = request_block->nameValueList.nameValue[i].value;
                    ownerOK = 1;
                    continue;
                }

                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "icatRef")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    icatRef = request_block->nameValueList.nameValue[i].value;
                    icatRefOK = 1;
                    continue;
                }
            }

            if (!ownerOK && !icatRefOK) {
                err = 999;
                idamLog(LOG_ERROR,
                        "ERROR issueDOI new: Insufficient Meta Data not passed - need DOI owner and ICAT Reference ID!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err,
                             "Insufficient Meta Data not passed - need DOI owner and ICAT Reference ID!");
                break;
            }

// Performance

            struct timeval tv_start, tv_stop;
            int msecs, usecs;
            gettimeofday(&tv_start, NULL);

// Create the Data Structure to be returned	 

            ISSUEDOI* data = NULL;
            data = (ISSUEDOI*) malloc(sizeof(ISSUEDOI));    // Structured Data Must be a heap variable

// Create the Unique Identifier

            struct timeval tv_uid;
            gettimeofday(&tv_uid, NULL);

            int microsecs = (int) tv_uid.tv_usec;    // If this is loo low resolution, use struct timespec (nano-secs)
            int secs = (int) tv_uid.tv_sec;
            struct tm* uid_time = localtime((time_t*) &secs);

            char year[5];
            char day[4];
            char hms[9];    // hh/mm/ss

            strftime(year, sizeof(year), "%Y", uid_time);        // 4 digit year
            strftime(day, sizeof(day), "%j", uid_time);        // 3 digit day of year
            strftime(hms, sizeof(hms), "%H/%M/%S", uid_time);    // Hour/Minute/Second

            if ((env = getenv("UDA_DOIPREFIX")) != NULL)
                sprintf(work, "%s/%s/%s/%s/%06d", env, year, day, hms, microsecs);
            else
                sprintf(work, "%s/%s/%s/%06d", year, day, hms, microsecs);

            gettimeofday(&tv_stop, NULL);
            idamLog(LOG_DEBUG, "issueDOI: new() Unique ID = '%s'\n", work);
            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int) (tv_stop.tv_usec - tv_start.tv_usec);
            idamLog(LOG_DEBUG, "issueDOI: new() Unique ID Cost = %d (ms), %d (microsecs)\n", msecs, usecs);
            tv_start = tv_stop;

// Create Transaction Block SQL

            sprintf(sql, "BEGIN; INSERT INTO doi_table (doi, owner, icatref) VALUES ('%s', '%s','%s'); END;",
                    work, owner, icatRef);

            idamLog(LOG_DEBUG, "%s\n", sql);

// execute

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, "SQL Execution Failed!");
                idamLog(LOG_ERROR, "ERROR issueDOI new: SQL Execution Failed\n");
                PQclear(DBQuery);
                break;
            }

            PQclear(DBQuery);

            gettimeofday(&tv_stop, NULL);
            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int) (tv_stop.tv_usec - tv_start.tv_usec);
            idamLog(LOG_DEBUG, "issueDOI: new() SQL Cost = %d (ms), %d (microsecs)\n", msecs, usecs);

// Write Return Structure

            stringLength = strlen(work) + 1;
            data->doi = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->doi, work);
            addMalloc((void*) data->doi, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(owner) + 1;
            data->owner = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->owner, owner);
            addMalloc((void*) data->owner, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(icatRef) + 1;
            data->icatRef = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->icatRef, icatRef);
            addMalloc((void*) data->icatRef, 1, stringLength * sizeof(char), "char");

            data->status = ' ';

            idamLog(LOG_DEBUG, "issueDOI doi: %s\n", data->doi);
            idamLog(LOG_DEBUG, "owner       : %s\n", data->owner);
            idamLog(LOG_DEBUG, "icatRefId   : %s\n", data->icatRef);
            idamLog(LOG_DEBUG, "Status      : %c\n", data->status);

            // the Returned Structure Definition

            data->structVersion = 1;                // This structure's version number

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "ISSUEDOI");
            usertype.size = sizeof(ISSUEDOI);

            strcpy(usertype.source, "issueDOI");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

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

            addUserDefinedType(userdefinedtypelist, usertype);

// Register the Pointer to the Data Structure 	 

            addMalloc((void*) data, 1, sizeof(ISSUEDOI), "ISSUEDOI");

// Pass the Data back	 

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "issue a new DOI");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("ISSUEDOI", 0);

            idamLog(LOG_DEBUG, "issueDOI: exiting function new\n");

            break;

        } else if (!strcasecmp(request_block->function, "put") ||
                   !strcasecmp(request_block->function, "record") || !strcasecmp(request_block->function, "add")) {

//----------------------------------------------------------------------------------------
// Record data access requets in a temporary database and manage the status of the records: 
//
// put( user=user, doi=doi, requestedSignal=requestedSignal, requestedSource=requestedSource, 
//	trueSignal=trueSignal, trueSource=trueSource, trueSourceDOI=trueSourceDOI,
//	logRecord=logRecord, created=created, status=[new|update|close|delete], execMethod=execMethod) 	
//
// DOI_Log table contains the following
// 
// DOI_Log_Key		Primary key
// DOI 			DOI reference
// requestedSignal	Requested data signal name	 
// requestedSource 	Requested data source name
// trueSignal		Signal within the source file
// trueSource		File path and name
// trueSourceDOI	Data source file's DOI
// logRecord		Log record of the data access
// created		date record was written
// status		0=>open, 1=>closed (Closed when requested by client application)
// execMethod = 1	Execute the SQL commands in the background immediately
//			Unreliable as often the log record is missing - perhaps executed before the record is created. 
// execMethod = 2	Collect the SQL commands together into a file and execute at the end - when 
//			the log record is written. This method is only employed for ADD and UPDATE.
//			Require automatic purge of sql command files
// execMethod = 3	Collect the SQL commands together into a static string buffer and execute at the end - when 
//			the log record is written. This method is only employed for ADD and UPDATE. No files to delete!
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

            static struct timeval tv_start, tv_stop;    // Performance
            int msecs, usecs;

            idamLog(LOG_DEBUG, "issueDOI: entering function record\n");

            char* doi, * requestedSignal, * requestedSource, * trueSignal, * trueSource, * trueSourceDOI, * logRecord;
            char status = '?';
            unsigned short doiOK = 0, requestedSignalOK = 0, requestedSourceOK = 0, trueSignalOK = 0,
                    trueSourceOK = 0, trueSourceDOIOK = 0, logRecordOK = 0, statusOK = 0, execMethodOK = 0;
            unsigned short execMethod = 0;

// Name Value pairs (Keywords have higher priority)

            for (i = 0; i < request_block->nameValueList.pairCount; i++) {
                idamLog(LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                        request_block->nameValueList.nameValue[i].value);

                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "doi")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    doi = request_block->nameValueList.nameValue[i].value;
                    doiOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "requestedSignal")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    requestedSignal = request_block->nameValueList.nameValue[i].value;
                    requestedSignalOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "requestedSource")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    requestedSource = request_block->nameValueList.nameValue[i].value;
                    requestedSourceOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSignal")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    trueSignal = request_block->nameValueList.nameValue[i].value;
                    trueSignalOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSource")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    trueSource = request_block->nameValueList.nameValue[i].value;
                    trueSourceOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSourceDOI")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    trueSourceDOI = request_block->nameValueList.nameValue[i].value;
                    trueSourceDOIOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "logRecord")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    logRecord = request_block->nameValueList.nameValue[i].value;
                    logRecordOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "status")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    status = request_block->nameValueList.nameValue[i].value[0];
                    statusOK = 1;
                    continue;
                }
                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "execMethod")) {
                    execMethod = (short) atoi(request_block->nameValueList.nameValue[i].value);

                    if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != NULL)
                        execMethod = (short) atoi(env);        // server environment has priority

                    execMethodOK = 1;
                    continue;
                }

            }

            if (!execMethodOK && (env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != NULL) {
                execMethod = (short) atoi(env);        // server environment sets an alternative default value
                execMethodOK = 1;
            }

            if (!doiOK || strlen(doi) == 0) {
                err = 999;
                idamLog(LOG_ERROR, "ERROR issueDOI add: The client provenance DOI must be specified!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err,
                             "The client provenance DOI must be specified!");
                break;
            }

            if (!statusOK) {
                err = 999;
                idamLog(LOG_ERROR, "ERROR issueDOI add: The record status must be specified!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err,
                             "The record status must be specified!");
                break;
            }

            idamLog(LOG_DEBUG, "issueDOI: passed parameters\n");
            if (doiOK) idamLog(LOG_DEBUG, "issueDOI: DOI = %s\n", doi);
            if (requestedSignalOK) idamLog(LOG_DEBUG, "issueDOI: requestedSignal = %s\n", requestedSignal);
            if (requestedSourceOK) idamLog(LOG_DEBUG, "issueDOI: requestedSource = %s\n", requestedSource);
            if (trueSignalOK) idamLog(LOG_DEBUG, "issueDOI: trueSignal = %s\n", trueSignal);
            if (trueSourceOK) idamLog(LOG_DEBUG, "issueDOI: trueSource = %s\n", trueSource);
            if (trueSourceDOIOK) idamLog(LOG_DEBUG, "issueDOI: trueSourceDOI = %s\n", trueSourceDOI);
            if (logRecordOK) idamLog(LOG_DEBUG, "issueDOI: logRecord = %s\n", logRecord);
            if (statusOK) idamLog(LOG_DEBUG, "issueDOI: Status = %c\n", status);
            if (execMethodOK) idamLog(LOG_DEBUG, "issueDOI: execMethod = %d\n", execMethod);

// 1> Create a new record if status == new 
// 2> add log data to an existing record if status == update 
// 3> close status for all records if status == close

            static FILE* sqlSet = NULL;
            static char* key = NULL;
            static char* tmpfile = NULL;
            static unsigned short keySeq = 0;

            FILE* ph = NULL;
            char cmd[2048];
            static char sqlBuffer[1024];


            do {            // Transaction Block Error Trap

                if (status == 'n') {    // Create a new record and reset the current primary key in scope
                    idamLog(LOG_DEBUG, "issueDOI: record() Create a new record\n");

                    if (key != NULL) {    // Always renewed for each new record
                        free((void*) key);
                        key = NULL;
                    }

                    if (execMethodOK) {

// No Primary Key returned so must create a unique key - use the randomised component of the file name + User's DOI	    

                        sprintf(work, "%s/%d/%d", doi, initTime, keySeq++);    // Create a unique key
                        key = (char*) malloc((strlen(work) + 1) * sizeof(char));
                        strcpy(key, work);

                        if (execMethod == 1) {

// Login password is stored in .pgpass for POSTGRESQL database so no need to set	       

                            if ((env = getenv("UDA_CLI_SQL")) != NULL)
                                strcpy(cmd, env);                    // Command line sql utility
                            else
                                strcpy(cmd, "psql");

                            sprintf(&cmd[strlen(cmd)],
                                    " -d %s -U %s -h %s -p %s -c \""
                                            "INSERT INTO doi_log "
                                            "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                                            "trueSourceDOI, tmpKey) "
                                            "VALUES ('%s','%s','%s','%s','%s','%s','%s');\" > /dev/null 2>&1 &",
                                    dbname, user, pghost, pgport,
                                    doi, requestedSignal, requestedSource, trueSignal, trueSource,
                                    trueSourceDOI, key);

                            idamLog(LOG_DEBUG, "issueDOI: record() SQL\n%s\n", cmd);

                            gettimeofday(&tv_start, NULL);

                            errno = 0;
                            ph = popen(cmd, "r");
                            //ph = popen(cmd, "e");		// Close on EXEC - fails!

                            if (ph == NULL || errno != 0) {
                                execMethodOK = 0;        // Disable exec method
                                err = 999;
                                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", errno, "");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err,
                                             "Cannot execute background SQL command");
                                break;
                            }

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 1 Cost A = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        } else if (execMethod == 2) {

// File directory 

                            char* tmpdir = getenv("UDA_WORK_DIR");

// Create a temporary file to collect the SQL commands

                            if (tmpdir != NULL) {
                                tmpfile = (char*) malloc((strlen(tmpdir) + 11) * sizeof(char));
                                sprintf(tmpfile, "%s/sqlXXXXXX", tmpdir);
                            } else {
                                tmpfile = (char*) malloc(16 * sizeof(char));
                                strcpy(tmpfile, "/tmp/sqlXXXXXX");
                            }

                            gettimeofday(&tv_start, NULL);

                            errno = 0;
                            if (mkstemp(tmpfile) < 0 || errno != 0) {
                                execMethodOK = 0;        // Disable SQL collection
                                err = 999;
                                if (errno != 0) err = errno;
                                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", err,
                                             "Unable to Obtain a Temporary File Name");
                                idamLog(LOG_ERROR, "ERROR issueDOI: Unable to Obtain a Temporary File Name\n");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, tmpdir);
                                break;
                            }

                            if ((sqlSet = fopen(tmpfile, "w")) == NULL) {
                                execMethodOK = 0;        // Disable collection
                                err = 999;
                                if (errno != 0) err = errno;
                                addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", err,
                                             "Unable to Open a Temporary File");
                                idamLog(LOG_ERROR, "ERROR workflow start: Unable to Open a Temporary File\n");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, tmpfile);
                                break;
                            }

                            fprintf(sqlSet, "BEGIN; INSERT INTO doi_log "
                                            "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                                            "trueSourceDOI, tmpKey) "
                                            "VALUES ('%s','%s','%s','%s','%s','%s','%s');",
                                    doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI, key);

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 2 Cost A = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        } else if (execMethod == 3) {

                            gettimeofday(&tv_start, NULL);

                            sprintf(sqlBuffer, "BEGIN; INSERT INTO doi_log "
                                            "(doi, requestedSignal, requestedSource, trueSignal, trueSource, "
                                            "trueSourceDOI, logRecord) "
                                            "VALUES ('%s','%s','%s','%s','%s','%s',",
                                    doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI);

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 3 Cost A = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        } else {
                            err = 999;
                            idamLog(LOG_ERROR, "ERROR issueDOI add: Incorrect execMethod argument {1|2|3}\n");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err,
                                         "Incorrect execMethod argument {1|2|3}");
                            break;
                        }
                    } else {                // Expensive & blocking database write method

                        sprintf(sql, "BEGIN; "
                                        "INSERT INTO doi_log "
                                        "(doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI) "
                                        "VALUES ('%s','%s','%s','%s','%s','%s');",
                                doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI);

                        idamLog(LOG_DEBUG, "issueDOI: record() SQL\n%s\n", sql);
// Execute the SQL

                        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                            err = 1;        // Roll back transaction
                            break;
                        }

                        PQclear(DBQuery);

// Return the Primary Key

                        strcpy(sql, "SELECT doi_log_id FROM doi_log WHERE "
                                "doi_log_id=currval('doi_log_id_seq');");

                        idamLog(LOG_DEBUG, "%s\n", sql);

                        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
                            err = 1;        // Roll Back transaction
                            break;
                        }

                        nrows = PQntuples(DBQuery);

                        if (nrows != 1) {
                            idamLog(LOG_ERROR, "ERROR issueDOI new: New doi_log record not found!\n");
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err,
                                         "New doi_log record not found!");
                            break;
                        }

// Extract the SQL data

                        stringLength = strlen(PQgetvalue(DBQuery, 0, 0)) + 1;
                        key = (char*) malloc(stringLength * sizeof(char));
                        strcpy(key, PQgetvalue(DBQuery, 0, 0));
                        PQclear(DBQuery);

                        idamLog(LOG_DEBUG, "issueDOI key: %s\n", key);

// Complete the transaction

                        sprintf(sql, "END;");

                        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                            free((void*) key);
                            key = NULL;
                            err = 1;
                            break;
                        }

                        PQclear(DBQuery);
                    }

                } else

                if (status == 'u') {    // update an existing record using the key from the ADD step
                    idamLog(LOG_DEBUG, "issueDOI: record() update an existing record with the Server Log record\n");

                    if (!logRecordOK) {
                        err = 999;
                        idamLog(LOG_ERROR, "ERROR issueDOI add: No Log record!\n");
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err, "No Log record!");
                        break;
                    }

                    if (execMethodOK) {
                        if (execMethod == 1) {

                            if ((env = getenv("UDA_CLI_SQL")) != NULL)
                                strcpy(cmd, env);
                            else
                                strcpy(cmd, "psql");

                            sprintf(&cmd[strlen(cmd)],
                                    " -d %s -U %s -h %s -p %s -c \""
                                            "UPDATE doi_log SET logRecord = '%s' WHERE tmpKey = '%s';\" > /dev/null 2>&1 &",
                                    dbname, user, pghost, pgport, logRecord, key);

                            idamLog(LOG_DEBUG, "issueDOI: update() SQL\n%s\n", cmd);

                            gettimeofday(&tv_start, NULL);

                            errno = 0;
                            ph = popen(cmd, "r");

                            if (ph == NULL || errno != 0) {
                                execMethodOK = 0;        // Disable exec method
                                err = 999;
                                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", errno, "");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err,
                                             "Cannot execute background SQL command");
                                break;
                            }

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 1 Cost B = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        } else if (execMethod == 2) {
                            gettimeofday(&tv_start, NULL);

                            fprintf(sqlSet, "UPDATE doi_log SET logRecord = '%s' WHERE tmpKey = '%s'; END;", logRecord,
                                    key);
                            fclose(sqlSet);

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 2 Cost B = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;

                            if ((env = getenv("UDA_CLI_SQL")) != NULL)
                                strcpy(work, env);
                            else
                                strcpy(work, "psql");

                            sprintf(cmd,
                                    "%s -d %s -U %s -h %s -p %s -f %s > /dev/null 2>&1 &",
                                    work, dbname, user, pghost, pgport, tmpfile);

                            idamLog(LOG_DEBUG, "issueDOI: update() SQL\n%s\n", cmd);

                            gettimeofday(&tv_start, NULL);

                            errno = 0;
                            ph = popen(cmd, "r");

                            if (ph == NULL || errno != 0) {
                                execMethodOK = 0;        // Disable exec method
                                err = 999;
                                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", errno, "");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err,
                                             "Cannot execute background SQL command");
                                break;
                            }

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 2 Cost C = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        } else if (execMethod == 3) {

                            gettimeofday(&tv_start, NULL);

                            if ((env = getenv("UDA_CLI_SQL")) != NULL)
                                strcpy(work, env);
                            else
                                strcpy(work, "psql");

                            sprintf(cmd,
                                    "%s '%s'); END;\" "
                                            "> /dev/null 2>&1 &",
                                    sqlBuffer, logRecord);

                            idamLog(LOG_DEBUG, "issueDOI: update() SQL\n%s\n", cmd);

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 3 Cost B = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;

                            errno = 0;
                            ph = popen(cmd, "r");

                            if (ph == NULL || errno != 0) {
                                execMethodOK = 0;        // Disable exec method
                                err = 999;
                                if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "issueDOI", errno, "");
                                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err,
                                             "Cannot execute background SQL command");
                                break;
                            }

                            gettimeofday(&tv_stop, NULL);
                            msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                            usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                    (int) (tv_stop.tv_usec - tv_start.tv_usec);
                            idamLog(LOG_DEBUG, "issueDOI: update() execMethod 3 Cost C = %d (ms), %d (microsecs)\n",
                                    msecs, usecs);
                            tv_start = tv_stop;
                        }
                    } else {

                        sprintf(sql, "BEGIN; "
                                "UPDATE doi_log SET logRecord = '%s' WHERE doi_log_id = %s;"
                                "END;", logRecord, key);

                        idamLog(LOG_DEBUG, "issueDOI: record() SQL\n%s\n", sql);

// Execute the SQL

                        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                            PQclear(DBQuery);
                            idamLog(LOG_ERROR, "ERROR issueDOI add: doi_log table update failed!\n");
                            err = 999;
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err,
                                         "doi_log table update failed!");
                            break;
                        }
                    }

                } else if (status ==
                           'c') {    // close all records for future deletion and execute collected SQL statements
                    idamLog(LOG_DEBUG, "issueDOI: record() Close all records\n");

                    sprintf(sql, "BEGIN; "
                            "UPDATE doi_log SET status = 1 WHERE doi = '%s';"
                            "END;", doi);

                    idamLog(LOG_DEBUG, "issueDOI: record() SQL\n%s\n", sql);

// Execute the SQL

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        PQclear(DBQuery);
                        idamLog(LOG_ERROR, "ERROR issueDOI add: doi_log status update failed!\n");
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err,
                                     "doi_log status update failed!");
                        break;
                    }

                } else

                if (status == 'd') {    // Delete closed records (Protection against malicious intent? user field?)
                    idamLog(LOG_DEBUG, "issueDOI: record() Delete closed records\n");

                    sprintf(sql, "BEGIN; "
                            "DELETE FROM doi_log WHERE status = 1 AND doi = '%s';" // and user='%s'
                            "END;", doi);

                    idamLog(LOG_DEBUG, "issueDOI: record() SQL\n%s\n", sql);

// Execute the SQL

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        PQclear(DBQuery);
                        idamLog(LOG_ERROR, "ERROR issueDOI add: doi_log deletion failed!\n");
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err, "doi_log deletion failed!");
                        break;
                    }
                }


            } while (0);    // End of SQL Transaction Error Trap

            if (err != 0 && (!execMethodOK || status == 'd')) {    // Rollback the Transaction

                if (err == 1) {
                    err = 999;
                    if (DBQuery == NULL) {
                        idamLog(LOG_ERROR, "ERROR issueDOI add: Database Query Failed!\n");
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err, "Database Query Failed!");
                    } else if (PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        idamLog(LOG_ERROR, "ERROR issueDOI add: %s\n", PQresultErrorMessage(DBQuery));
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err,
                                     PQresultErrorMessage(DBQuery));
                    }
                }

                PQclear(DBQuery);

                sprintf(sql, "ROLLBACK; END;");
                idamLog(LOG_DEBUG, "%s\n", sql);

                DBQuery = PQexec(DBConnect, sql);
                PQclear(DBQuery);

                break;
            }

// Return 

            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) malloc(sizeof(int));
            int* pi = (int*) data_block->data;
            pi[0] = 0;

            strcpy(data_block->data_desc, "data access logged against DOI");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            idamLog(LOG_DEBUG, "issueDOI: exiting function record\n");

            break;

        } else if (!strcasecmp(request_block->function, "list")) {
//----------------------------------------------------------------------------------------
// LIST all data access records from the DOI_Log table

            idamLog(LOG_DEBUG, "issueDOI: entering function list\n");

            char* doi;
            unsigned short doiOK = 0;

// Name Value pairs (Keywords have higher priority) + Protect against SQL Injection

            for (i = 0; i < request_block->nameValueList.pairCount; i++) {
                idamLog(LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                        request_block->nameValueList.nameValue[i].value);

                if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "doi")) {
                    preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                    doi = request_block->nameValueList.nameValue[i].value;
                    doiOK = 1;
                    continue;
                }
            }

            if (!doiOK) {
                err = 999;
                idamLog(LOG_ERROR, "ERROR issueDOI list: The client provenance DOI must be specified!\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI add", err,
                             "The client provenance DOI must be specified!");
                break;
            }

            sprintf(sql, "SELECT doi, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceDOI, "
                    "logRecord, creation FROM doi_log WHERE doi='%s';", doi);

            idamLog(LOG_DEBUG, "issueDOI: list() SQL\n%s\n", sql);

// Execute the SQL

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
                err = 999;
                idamLog(LOG_ERROR, "ERROR issueDOI list: SQL Failed\n");
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI list", err, "SQL Failed!");
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                idamLog(LOG_ERROR, "ERROR issueDOI list: No doi_log records found!\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI new", err, "No doi_log records found");
                break;
            }

// Create the Data Structures to be returned	 

            DOIRECORD* data = (DOIRECORD*) malloc(nrows * sizeof(DOIRECORD));

            addMalloc((void*) data, nrows, sizeof(DOIRECORD), "DOIRECORD");

            LISTDOI* list = (LISTDOI*) malloc(1 * sizeof(LISTDOI));

            list->structVersion = 1;
            list->count = nrows;
            list->doi = (char*) malloc((strlen(doi) + 1) * sizeof(char));
            strcpy(list->doi, doi);
            list->list = data;

            addMalloc((void*) list, 1, sizeof(LISTDOI), "LISTDOI");
            addMalloc((void*) list->doi, 1, (strlen(doi) + 1) * sizeof(char), "char");

// Extract the SQL data

            for (i = 0; i < nrows; i++) {

                stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                data[i].doi = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].doi, PQgetvalue(DBQuery, i, 0));
                addMalloc((void*) data[i].doi, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                data[i].requestedSignal = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].requestedSignal, PQgetvalue(DBQuery, i, 1));
                addMalloc((void*) data[i].requestedSignal, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                data[i].requestedSource = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].requestedSource, PQgetvalue(DBQuery, i, 2));
                addMalloc((void*) data[i].requestedSource, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                data[i].trueSignal = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].trueSignal, PQgetvalue(DBQuery, i, 3));
                addMalloc((void*) data[i].trueSignal, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                data[i].trueSource = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].trueSource, PQgetvalue(DBQuery, i, 4));
                addMalloc((void*) data[i].trueSource, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 5)) + 1;
                data[i].trueSourceDOI = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].trueSourceDOI, PQgetvalue(DBQuery, i, 5));
                addMalloc((void*) data[i].trueSourceDOI, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 6)) + 1;
                data[i].logRecord = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].logRecord, PQgetvalue(DBQuery, i, 6));
                addMalloc((void*) data[i].logRecord, 1, stringLength * sizeof(char), "char");

                stringLength = strlen(PQgetvalue(DBQuery, i, 7)) + 1;
                data[i].creation = (char*) malloc(stringLength * sizeof(char));
                strcpy(data[i].creation, PQgetvalue(DBQuery, i, 7));
                addMalloc((void*) data[i].creation, 1, stringLength * sizeof(char), "char");

                idamLog(LOG_DEBUG, "doi            : %s\n\n", data[i].doi);
                idamLog(LOG_DEBUG, "requestedSignal: %s\n", data[i].requestedSignal);
                idamLog(LOG_DEBUG, "requestedSource: %s\n", data[i].requestedSource);
                idamLog(LOG_DEBUG, "trueSignal     : %s\n", data[i].trueSignal);
                idamLog(LOG_DEBUG, "trueSource     : %s\n", data[i].trueSource);
                idamLog(LOG_DEBUG, "trueSourceDOI  : %s\n", data[i].trueSourceDOI);
                idamLog(LOG_DEBUG, "logRecord      : %s\n", data[i].logRecord);
                idamLog(LOG_DEBUG, "creation date  : %s\n", data[i].creation);
            }
            PQclear(DBQuery);

// the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "DOIRECORD");
            usertype.size = sizeof(DOIRECORD);

            strcpy(usertype.source, "DOI Plugin");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

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

            addUserDefinedType(userdefinedtypelist, usertype);

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "LISTDOI");
            usertype.size = sizeof(LISTDOI);

            strcpy(usertype.source, "DOI Plugin");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "structVersion", "This Data Structure's version number", &offset, SCALARUSHORT);
            addCompoundField(&usertype, field);

            defineField(&field, "count", "The number of records in the list", &offset, SCALARUSHORT);
            addCompoundField(&usertype, field);

            defineField(&field, "doi", "Digital Object Identifier", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);

            initCompoundField(&field);
            strcpy(field.name, "list");
            field.atomictype = TYPE_UNKNOWN;
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

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) list;

            strcpy(data_block->data_desc, "List of Signals accessed with a DOI");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("LISTDOI", 0);

            idamLog(LOG_DEBUG, "issueDOI: Function list called\n");

            if (data_block->opaque_block == NULL) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI:list", err,
                             "failed to locate LISTDOI structure!");
                break;
            }

            break;

        } else {
//----------------------------------------------------------------------------------------
// Not a Known Function!

            idamLog(LOG_ERROR, "ERROR issueDOI: Function %s Not Known.!\n", request_block->function);
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, "Unknown Function requested");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "issueDOI", err, request_block->function);
            break;
        }

    } while (0);

    concatIdamError(idamerrorstack, idamErrorStack);

    return err;
}
