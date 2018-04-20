/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access METADATA from the IDAM Database
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		readMeta	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.

Issues:

	Need to use '/' as the signal name for netCDF. Other formats ? Need to use empty string!

	Scalar integer attributes are returned as pointers - this is cumbersome!
	enum label value pairs are not returned. Why when they should be!
	Need Images of the returned data structures - as documentation.

        include a database flag to ensure scalar integer attributes are returned as scalars and not pointers.
*---------------------------------------------------------------------------------------------------------------*/
#include "readMeta.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <server/udaServer.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/initStructs.h>
#include <server/sqllib.h>
#include <server/getServerEnvironment.h>

// Prevent SQL injection malicious intent
// Not required if the server is Read Only!
// Use escaped strings to isolate SQL injection attemp, e.g. string="LIMIT 10; ' DROP DATABASE; '"

static int preventSQLInjection(PGconn* DBConnect, char** from)
{
    // Replace the passed string with an Escaped String
    // Free the Original string from Heap

    int err = 0;
    size_t fromCount = strlen(*from);
    char* to = (char*)malloc((2 * fromCount + 1) * sizeof(char));
    PQescapeStringConn(DBConnect, to, *from, fromCount, &err);
    if (err != 0) {
        if (to != NULL) free((void*)to);
        return 1;
    }
    free((void*)*from);
    *from = to;
    return 0;
}

extern int readMeta(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    static short init = 0;
    static short sqlPrivate = 0;            // If the SQL connection was opened here, it remains local and open but is not passed back.
    static short context = CONTEXT_DATA;        // Different classes of metadata - context is important
    short newContext = 0;

    int i, j, offset = 0, nrows, ncols, col;

    char* p, * s;
    char work[MAXSQL];
    char sql[MAXSQL];
    int stringLength;
    int wCount = 0;

    int exp_number = 0, pass = -1, shotDependent = -1, sourceDependent = -1, typeDependent = -1,
            versionDependent = -1, revisionDependent = -1, classDependent = -1, descDependent = -1, tableDependent = -1,
            systemDependent = -1, configDependent = -1, deviceDependent = -1, nameDependent = -1, whereDependent = -1,
            limitDependent = -1, fileDependent = -1, signalMatchDependent = -1;
    unsigned short isListDevices = 0, isListClasses = 0, isListSources = 0, isLastShot = 0, isShotDateTime = 0,
            isStructure = 0, isLatest = 0, isLastPass = 0;

    unsigned short castTypeId = CASTCOLUMN;    // Data orientation, default: Row
    unsigned short stringTypeId = SCALARSTRING;    // String type, default: scalar
    unsigned short intTypeId = SCALARINT;    // Integer type, default: scalar
    unsigned short uintTypeId = SCALARUINT;    // Unsigned Integer type, default: scalar
    void* structData = NULL;

    const char* signal_match = NULL;
    const char* description = NULL;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    unsigned short housekeeping;

    static unsigned short DBType = PLUGINSQLNOTKNOWN;
    static PGconn* DBConnect = NULL;

    PGresult* DBQuery = NULL;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = 1;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;

    housekeeping = idam_plugin_interface->housekeeping;

    if (!sqlPrivate) {                        // Use External SQL Connection
        DBType = idam_plugin_interface->sqlConnectionType;
        DBConnect = idam_plugin_interface->sqlConnection;
    }

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        UDA_LOG(UDA_LOG_DEBUG, "Meta: reset function called.\n");

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        if (sqlPrivate && DBConnect != NULL && DBType == PLUGINSQLPOSTGRES) {
            PQfinish(DBConnect);
            sqlPrivate = 0;
            context = CONTEXT_DATA;
            DBConnect = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }
        init = 0;        // Ready to re-initialise
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise if requested or if the context has changed (the previous private SQL connection must be closed)

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "context")) {
            if (STR_IEQUALS(request_block->nameValueList.nameValue[i].value, "data")) {
                if (context != CONTEXT_DATA) {
                    newContext = CONTEXT_DATA;
                    break;
                }
            } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i].value, "meta")) {
                if (context != CONTEXT_META) {
                    newContext = CONTEXT_META;
                    break;
                }
            } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i].value, "cpf")) {
                if (context != CONTEXT_CPF) {
                    newContext = CONTEXT_CPF;
                    break;
                }
            }
        }
    }

    if (newContext > 0) {
        if ((context == CONTEXT_CPF || newContext == CONTEXT_CPF) &&
            sqlPrivate) {    // Close previous local private SQL connection
            if (DBConnect != NULL && DBType == PLUGINSQLPOSTGRES) {
                PQfinish(DBConnect);
                sqlPrivate = 0;
                DBConnect = NULL;
                DBType = PLUGINSQLNOTKNOWN;
            }
            init = 0;                        // Need a new or use the passed connection!
        }
        context = newContext;
    }

    if (!STR_IEQUALS(request_block->function, "help") &&
        (!init || STR_IEQUALS(request_block->function, "init")
         || STR_IEQUALS(request_block->function, "initialise"))) {

        UDA_LOG(UDA_LOG_DEBUG, "Meta: init function called.\n");

        // Is there an Open SQL Connection? If not then open a private connection
        // If the context is CPF then target the CPF rather than the IDAM database
        // **** ToDo: make the CPF a separate schema of the IDAM database

        if (context != CONTEXT_CPF) {
            if (DBConnect == NULL && (DBType == PLUGINSQLPOSTGRES || DBType == PLUGINSQLNOTKNOWN)) {
                DBConnect = startSQL(idam_plugin_interface->environment);        // No prior connection to IDAM Postgres SQL Database
                if (DBConnect != NULL) {
                    DBType = PLUGINSQLPOSTGRES;
                    sqlPrivate = 1;
                    UDA_LOG(UDA_LOG_DEBUG, "Meta: Private regular database connection made.\n");
                }
            }
        } else {

            // Highjack the database name from the server's environment data structure

            UDA_LOG(UDA_LOG_DEBUG, "Meta: Connecting to the CPF database\n");

            ENVIRONMENT environment = *idam_plugin_interface->environment;

            int lstr = strlen(environment.sql_dbname) + 1;
            char* env;
            char* old_dbname = (char*)malloc(lstr * sizeof(char));
            strcpy(old_dbname, environment.sql_dbname);

            strcpy(environment.sql_dbname, "cpf");        // Case Sensitive!!!
            if ((env = getenv("UDA_CPFDBNAME")) != NULL) strcpy(environment.sql_dbname, env);
            if ((env = getenv("CPF_SQLDBNAME")) != NULL) strcpy(environment.sql_dbname, env);

            //DBConnect = (PGconn *)startSQL();		// Picking up startSQL from somewhere!
            // preload of liblastshot.so & libidamNotify.so for MDS+ sandbox !!!!

            DBConnect = startSQL_CPF(&environment);        // Ignore prior connection to IDAM Postgres SQL Database
            if (DBConnect != NULL) {
                DBType = PLUGINSQLPOSTGRES;
                sqlPrivate = 1;
                UDA_LOG(UDA_LOG_DEBUG, "Meta: Private CPF database connection made.\n");
            }
            strcpy(environment.sql_dbname, old_dbname);
            free((void*)old_dbname);
        }
        if (DBConnect == NULL) {        // No connection!
            RAISE_PLUGIN_ERROR("SQL Database Server Connect Error");
        }

        init = 1;

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Standard metadata specific Name Value pairs
    // Escape all strings to protect against SQL Injection

    exp_number = request_block->exp_number;
    pass = request_block->pass;        // 'Pass' as a text string (tpass) is not used

    // Keyword have higher priority

    for (i = 0; i < request_block->nameValueList.pairCount; i++) {

        UDA_LOG(UDA_LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                request_block->nameValueList.nameValue[i].value);

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "exp_number") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "shot") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "pulno")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                exp_number = atoi(request_block->nameValueList.nameValue[i].value);
                shotDependent = i;
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "pass")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                pass = atoi(request_block->nameValueList.nameValue[i].value);
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "version")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                versionDependent = i;
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "revision")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value)) {
                revisionDependent = i;
                continue;
            } else {
                err = 888;
                break;
            }
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "type")) {
            typeDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[typeDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "alias") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "source") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "source_alias")) {
            sourceDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[sourceDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "class")) {
            classDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[classDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "description")) {
            descDependent = i;
            FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, description);
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "signal_match")) {
            signalMatchDependent = i;
            FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, signal_match);
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "table")) {
            tableDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[tableDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "names") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "name")) {
            nameDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[nameDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "where")) {
            whereDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[whereDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "limit")) {
            limitDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[limitDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "system")) {
            systemDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[systemDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "Config") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "configuration")) {
            configDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[configDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "device")) {
            deviceDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[deviceDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "cast") ||
            STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "align")) {
            if (STR_IEQUALS(request_block->nameValueList.nameValue[i].value, "row")) {
                castTypeId = CASTROW;
                stringTypeId = SCALARSTRING;
                intTypeId = SCALARINT;
                uintTypeId = SCALARUINT;
            } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i].value, "column")) {
                castTypeId = CASTCOLUMN;
                stringTypeId = ARRAYSTRING;
                intTypeId = ARRAYINT;
                uintTypeId = ARRAYUINT;
            }
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "file")) {
            fileDependent = i;
            if (preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[fileDependent].value)) {
                err = 888;
                break;
            }
            continue;
        }

        // Keywords

        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "listdevices")) {
            isListDevices = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "listsources")) {
            isListSources = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "lastshot")) {
            isLastShot = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "shotdatetime")) {
            isShotDateTime = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "structure")) {
            isStructure = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "listclasses")) {
            isListClasses = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "latest")) {
            isLatest = 1;
            continue;
        }
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "lastpass")) {
            isLastPass = 1;
            continue;
        }
    }

    // Test Result for malicious SQL Injection

    if (err == 888) {
        RAISE_PLUGIN_ERROR("Invalid Keyword Content!");
    }

    //----------------------------------------------------------------------------------------
    // Functions

    err = 0;

    do {

        //----------------------------------------------------------------------------------------
        // Help: A Description of library functionality
        //
        // meta::help()		No context required
        // Information is returned as a single string containing all required format control characters.

        if (STR_IEQUALS(request_block->function, "help")) {

            strcpy(work, "\nMETA: The META server side function library provides access to the IDAM Metadata registry. "
                    "This registry records different classes of metadata. "
                    "Access to these classes depends on the data request context. "
                    "There are three contexts: META means the metadata recorded in the registy. DATA means the metadata "
                    "recorded in the data respository. And CPF means the high level plasma classification data recorded in "
                    "the CPF database. The default context is DATA. "
                    "The library provides functions (list and get methods) to retrieve the required information as well as describing "
                    "what information is available, enabling Data Discovery.\n\n"

                    "Function: list(context={meta|data|cpf} [,{cast|align}={row|column}] [,class=class] [,system=system]\n"
                    "               [,{Config|configuration}=configuration] [,device=device]\n"
                    "               [,{shot|pulno|exp_number}=shot]\n"
                    "               [,version=version] [,revision=revision]\n"
                    "               [,{source|alias|source_alias}=source_alias] [,type={A|R|M|P|I|*}] [,pass=pass]\n"
                    "               [,description=description] [,table=table]\n"
                    "               [,/listclasses] [,/listsources] [,/structure])\n\n"
                    "This function is used for data discovery and returns a list of the meta data available "
                    "for a specific shot and/or class etc.\n"
                    "The meanings of keywords are:\n"
                    "\tlistdevices (with context=meta}) - return a list of device names for which metadata exists.\n"
                    "\tlistclasses (with context={meta|cpf}) - return a list of metadata classes available.\n"
                    "\tlistsources (with context=data) - return a list of data sources metadata available for a specific shot.\n"
                    "\tstructure (with context=meta) - return the associated data structure definitions (work in progress!).\n\n"

                    "IDL Examples:\n"
                    "\tList all available data sources for a given MAST shot\n"
                    "\t\tstr=getdata('meta::list(context=data, /listSources)', '17300') or\n"
                    "\t\tstr=getdata('meta::list(context=data, shot=17300, /listSources)', '')\n"
                    "\tList all available data signals (Raw, Analysed, Modelled, etc.) for a given MAST shot\n"
                    "\t\tstr=getdata('meta::list(context=data, shot=23456), type=A', 'MAST::')\n"
                    "\tList metadata classes for a specific context\n"
                    "\t\tstr=getdata('meta::list(context=meta, /listClasses)','') or\n"
                    "\t\tstr=getdata('meta::list(context=cpf, /listClasses)','')\n"
                    "\tList all available metadata\n"
                    "\t\tstr=getdata('meta::list(context=meta)','')\n"

                    "\n\nFunction: get(context={meta|cpf|data} [,{cast|align}={row|column}] [,class=class] [,system=system]\n"
                    "                  [,configuration=configuration] [,device=device]\n"
                    "                  [,{shot|pulno|exp_number}=shot]\n"
                    "                  [,version=version] [,revision=revision]\n"
                    "                  [,names=\"var1,var2,var3,...\"] [,table=table] [,where=where] [,limit=limit]\n"
                    "                  [,/lastshot] [,/shotdatetime] [,/latest])\n\n"
                    "This function is used to return a data structure populated with the metadata requested. If the context is 'meta', "
                    "the selection criteria must be sufficient to identify a single metadata registry record, "
                    "otherwise the ambiguity will cause an error.\n"
                    "The meanings of keywords are:\n"
                    "\tlastshot (with context=data) - return the last shot number recorded for the default device.\n"
                    "\tshotdatetime (with context=data) - return the date and time of a specific plasma shot for the default device.\n"
                    "\tlatest (with context=meta) - return the latest metadata version and revision.\n\n"
                    "IDL Examples:\n"
                    "\tGet the Soft X-Ray camera configuration data\n"
                    "\t\tstr=getdata('meta::get(context=meta,class=diagnostic,system=sxr)', '')\n"
                    "\tGet the Omaha magnetic coil names and locations\n"
                    "\t\tstr=getdata('meta::get(context=meta,class=magnetic,system=omaha,configuration=specview,version=1,revision=1,shot=17301)', '')\n"
                    "\tGet the set of coordinates that map out the limiting surfaces inside the MAST vessel\n"
                    "\t\tstr=getdata('meta::get(context=meta,class=structure,configuration=limiter)','')\n"
                    "\tGet the last shot number\n"
                    "\t\tstr=getdata('meta::get(context=data,/lastshot)','')\n"
                    "\tGet the data and time of a shot\n"
                    "\t\tstr=getdata('meta::get(context=data,shot=13500,/shotdatetime)','')\n"
                    "\tGet plasma classification data from the CPF\n"
                    "\t\tstr=getdata('meta::get(context=cpf,names=\"exp_number,tstart,tend\",table=mast0,"
                    "where=\"tend>=0.4 ORDER BY exp_number desc\",limit=99,cast=column)','')\n"
            );

            UDA_LOG(UDA_LOG_DEBUG, "readMeta:\n%s\n", work);

            // Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "METAHELP");
            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(METAHELP);        // Structure size
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "help", "Information about the META plugin functions", &offset,
                        SCALARSTRING);    // Single string, arbitrary length
            addCompoundField(&usertype, field);
            addUserDefinedType(userdefinedtypelist, usertype);

            // Create Data

            METAHELP* data;
            stringLength = strlen(work) + 1;
            data = (METAHELP*)malloc(sizeof(METAHELP));
            data->value = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->value, work);
            addMalloc(logmalloclist, (void*)data, 1, sizeof(METAHELP), "METAHELP");
            addMalloc(logmalloclist, (void*)data->value, 1, stringLength * sizeof(char), "char");

            // Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "META Plugin help");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METAHELP", 0);

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function help called\n");

            break;

        } else if (STR_IEQUALS(request_block->function, "version")) {
            err = setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION,
                                         "Plugin version number");
            break;
        } else if (STR_IEQUALS(request_block->function, "builddate")) {
            err = setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
            break;
        } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
            err = setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD,
                                      "Plugin default method");
            break;
        } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
            err = setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                         "Maximum Interface Version");
            break;
        } else

            //----------------------------------------------------------------------------------------
            // META Context
            //----------------------------------------------------------------------------------------
            // List all Metadata Device names
            // e.g. meta::listData(context={meta}, /listdevices [,cast={row|column}] [,device=device])
            // Return options:
            // 1> Row orientation - array of structures with scalar strings	(default)
            // 2> Column orientation - scalar structure with arrays of strings

        if (context == CONTEXT_META && isListDevices &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            strcpy(sql, "SELECT DISTINCT device FROM Meta_Alias ORDER BY device ASC");

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: listDevices SQL\n%s\n", sql);

            // Execute the SQL
            UDA_LOG(UDA_LOG_DEBUG, "Query (meta) : %s\n", sql);

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listDevices: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta listDevices", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listDevices: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta listDevices", err,
                             PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listDevices: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta listDevices", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "METADEVICE_R");        // Default is Row Oriented
                usertype.size = sizeof(METADEVICE_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "METADEVICE_C");
                usertype.size = sizeof(METADEVICE_C);            // Selected return structure size
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "device", "experimental device", &offset, stringTypeId);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

            if (castTypeId == CASTROW) {                // Row oriented

                METADEVICE_R* data;
                data = (METADEVICE_R*)malloc(
                        nrows * sizeof(METADEVICE_R));    // Structured Data Must be a heap variable
                addMalloc(logmalloclist, (void*)data, nrows, sizeof(METADEVICE_R), "METADEVICE_R");
                structData = (void*)data;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].name, PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data[i].name, 1, stringLength * sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "listDevices: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "device     : %s\n", data[i].name);
                }
            } else if (castTypeId == CASTCOLUMN) {
                // Column oriented
                METADEVICE_C* data;
                data = (METADEVICE_C*)malloc(sizeof(METADEVICE_C));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(METADEVICE_C), "METADEVICE_C");
                structData = (void*)data;

                data->name = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->name, nrows, sizeof(char*), "STRING *");

                data->count = nrows;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data->name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->name[i], PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data->name[i], stringLength, sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "listDevices: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "device     : %s\n", data->name[i]);
                }
            }

            PQclear(DBQuery);

            // Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listDevices");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADEVICE_R", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADEVICE_C", 0);
            }
            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listDevices called\n");

            break;

        } else

            //----------------------------------------------------------------------------------------
            // List all Classes of Metadata
            // e.g. meta::listData(context={meta|cpf}, /listclasses [,cast={row|column}] [,device=device])
            // Return options:
            // 1> Row orientation - array of structures with scalar strings	(default)
            // 2> Column orientation - scalar structure with arrays of strings

        if (context == CONTEXT_META && isListClasses &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            if (deviceDependent >= 0) {
                sprintf(sql, "SELECT class,system,device,ro,description FROM Meta_Alias "
                        "WHERE device ILIKE '%s' ORDER BY device, class, system ASC",
                        request_block->nameValueList.nameValue[deviceDependent].value);
            } else {
                strcpy(sql,
                       "SELECT class, system, device, ro, description FROM Meta_Alias ORDER BY device, class, system ASC");
            }

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: listClasses SQL\n%s\n", sql);

            // Execute the SQL

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta list/listClasses", err,
                             "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta list/listClasses", err,
                             PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta list/listClasses", err,
                             "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "METALIST_R");        // Default is Row Oriented
                usertype.size = sizeof(METALIST_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "METALIST_C");
                usertype.size = sizeof(METALIST_C);            // Selected return structure size
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "class", "metadata class", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "system", "system component", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "device", "experimental device", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "ro", "responsible officer", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "description", "description of content", &offset, stringTypeId);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

            // Create Data

            if (castTypeId == CASTROW) {                // Row oriented

                METALIST_R* data;
                data = (METALIST_R*)malloc(nrows * sizeof(METALIST_R));    // Structured Data Must be a heap variable
                addMalloc(logmalloclist, (void*)data, nrows, sizeof(METALIST_R), "METALIST_R");
                structData = (void*)data;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data[i].class = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].class, PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data[i].class, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data[i].system = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].system, PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data[i].system, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data[i].device = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].device, PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data[i].device, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                    data[i].ro = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].ro, PQgetvalue(DBQuery, i, 3));
                    addMalloc(logmalloclist, (void*)data[i].ro, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data[i].description = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].description, PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data[i].description, 1, stringLength * sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "listClasses: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data[i].class);
                    UDA_LOG(UDA_LOG_DEBUG, "system     : %s\n", data[i].system);
                    UDA_LOG(UDA_LOG_DEBUG, "device     : %s\n", data[i].device);
                    UDA_LOG(UDA_LOG_DEBUG, "ro         : %s\n", data[i].ro);
                    UDA_LOG(UDA_LOG_DEBUG, "description: %s\n", data[i].description);
                }

            } else if (castTypeId == CASTCOLUMN) {
                // Column oriented
                METALIST_C* data;
                data = (METALIST_C*)malloc(sizeof(METALIST_C));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(METALIST_C), "METALIST_C");
                structData = (void*)data;

                data->class = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->class, nrows, sizeof(char*), "STRING *");
                data->system = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->system, nrows, sizeof(char*), "STRING *");
                data->device = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->device, nrows, sizeof(char*), "STRING *");
                data->ro = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->ro, nrows, sizeof(char*), "STRING *");
                data->description = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->description, nrows, sizeof(char*), "STRING *");

                data->count = nrows;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data->class[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->class[i], PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data->class[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data->system[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->system[i], PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data->system[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data->device[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->device[i], PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data->device[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                    data->ro[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->ro[i], PQgetvalue(DBQuery, i, 3));
                    addMalloc(logmalloclist, (void*)data->ro[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data->description[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->description[i], PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "listClasses: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data->class[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "system     : %s\n", data->system[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "device     : %s\n", data->device[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "ro         : %s\n", data->ro[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "description: %s\n", data->description[i]);
                }
            }

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listClasses");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METALIST_R", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METALIST_C", 0);
            }
            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function list or listClasses called\n");

            break;

        } else

            //----------------------------------------------------------------------------------------
            // List the Configuration Metadata available
            // e.g. meta::listData( context=meta [,cast={row|column}] [,class=class] [,system=system]
            //			[,configuration=configuration] [,device=device]
            //			[,shot=shot][,version=version][,revision=revision][,/structure])

        if (context == CONTEXT_META && !isListClasses &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            if (isStructure) {
                strcpy(sql,
                       "SELECT class, system, device, configuration, version, revision, status, description, comment, "
                               "range_start, range_stop, type_name, structure_description, definition FROM "
                               "Meta_Source_Structure_View ");
            } else {
                strcpy(sql,
                       "SELECT class, system, device, configuration, version, revision, status, description, comment, "
                               "range_start, range_stop FROM Meta_Source_View ");
            }

            wCount = 0;
            if (classDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE class = '");
                else
                    strcat(sql, "AND class = '");
                strcat(sql, request_block->nameValueList.nameValue[classDependent].value);
                strcat(sql, "' ");
            }
            if (systemDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE system = '");
                else
                    strcat(sql, "AND system = '");
                strcat(sql, request_block->nameValueList.nameValue[systemDependent].value);
                strcat(sql, "' ");
            }
            if (configDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE configuration = '");
                else
                    strcat(sql, "AND configuration = '");
                strcat(sql, request_block->nameValueList.nameValue[configDependent].value);
                strcat(sql, "' ");
            }
            if (deviceDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE device = '");
                else
                    strcat(sql, "AND device = '");
                strcat(sql, request_block->nameValueList.nameValue[deviceDependent].value);
                strcat(sql, "' ");
            } else {
                if (wCount++ == 0)
                    strcat(sql, "WHERE device = '");
                else
                    strcat(sql, "AND device = '");
                strcat(sql, API_DEVICE);
                strcat(sql, "' ");
            }
            if (shotDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE (range_start = 0 OR range_start <= ");
                else
                    strcat(sql, "AND (range_start = 0 OR range_start <= ");
                strcat(sql, request_block->nameValueList.nameValue[shotDependent].value);
                strcat(sql, ") AND (range_stop = 0 OR range_stop >= ");
                strcat(sql, request_block->nameValueList.nameValue[shotDependent].value);
                strcat(sql, ") ");

            } else {
                if (exp_number > 0) {
                    sprintf(work, "%d", exp_number);
                    if (wCount++ == 0)
                        strcat(sql, "WHERE (range_start = 0 OR range_start <= ");
                    else
                        strcat(sql, "AND (range_start = 0 OR range_start <= ");
                    strcat(sql, work);
                    strcat(sql, ") AND (range_stop = 0 OR range_stop >= ");
                    strcat(sql, work);
                    strcat(sql, ") ");
                }
            }
            if (versionDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE version = ");
                else
                    strcat(sql, "AND version = ");
                strcat(sql, request_block->nameValueList.nameValue[versionDependent].value);
                strcat(sql, " ");
            }
            if (revisionDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE revision = ");
                else
                    strcat(sql, "AND revision = ");
                strcat(sql, request_block->nameValueList.nameValue[revisionDependent].value);
                strcat(sql, " ");
            }
            strcat(sql, " ORDER BY device, class, system ASC");

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: SQL\n%s\n", sql);

// Execute the SQL

            UDA_LOG(UDA_LOG_DEBUG, "Query (meta) : %s\n", sql);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {                // Default is Row Oriented
                if (isStructure) {
                    strcpy(usertype.name, "METADATA_RS");
                    usertype.size = sizeof(METADATA_RS);
                } else {
                    strcpy(usertype.name, "METADATA_R");
                    usertype.size = sizeof(METADATA_R);
                }
            } else if (castTypeId == CASTCOLUMN) {
                if (isStructure) {
                    strcpy(usertype.name, "METADATA_CS");
                    usertype.size = sizeof(METADATA_CS);
                } else {
                    strcpy(usertype.name, "METADATA_C");
                    usertype.size = sizeof(METADATA_C);
                }
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "class", "metadata class", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "system", "system component", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "device", "experimental device", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "configuration", "configuration name", &offset, stringTypeId);
            addCompoundField(&usertype, field);

            defineField(&field, "version", "metadata document version number", &offset, uintTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "revision", "metadata document version revision number", &offset, uintTypeId);
            addCompoundField(&usertype, field);

            defineField(&field, "status", "document status", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "description", "description of the Metadata", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "comment", "comment on this Version and Revision", &offset, stringTypeId);
            addCompoundField(&usertype, field);

            defineField(&field, "range_start", "start of shot number range when valid", &offset, uintTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "range_stop", "end of shot number range when valid", &offset, uintTypeId);
            addCompoundField(&usertype, field);

            if (isStructure) {
                defineField(&field, "type_name", "data structure type name", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "structure_description", "data structure description", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "definition", "data structure definition", &offset, stringTypeId);
                addCompoundField(&usertype, field);
            }

            addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

            if (castTypeId == CASTROW) {                // Row oriented

                if (isStructure) {
                    METADATA_RS* data;
                    data = (METADATA_RS*)malloc(nrows * sizeof(METADATA_RS));
                    addMalloc(logmalloclist, (void*)data, nrows, sizeof(METADATA_RS), "METADATA_RS");
                    structData = (void*)data;

                    for (i = 0; i < nrows; i++) {

                        col = 0;
                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].class = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].class, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].class, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].system = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].system, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].system, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].device = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].device, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].device, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].configuration = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].configuration, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].configuration, 1, stringLength * sizeof(char), "char");

                        data[i].version = atoi(PQgetvalue(DBQuery, i, col++));
                        data[i].revision = atoi(PQgetvalue(DBQuery, i, col++));

                        stringLength = 12;                        // Test, Development or Production 4, 11 or 10
                        data[i].status = (char*)malloc(stringLength * sizeof(char));
                        addMalloc(logmalloclist, (void*)data[i].status, 1, stringLength * sizeof(char), "char");
                        if (PQgetvalue(DBQuery, i, col)[0] == 'T') {
                            strcpy(data[i].status, "Test");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'D') {
                            strcpy(data[i].status, "Development");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'P') {
                            strcpy(data[i].status, "Production");
                            col++;
                        }

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].description = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].description, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].description, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].comment = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].comment, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].comment, 1, stringLength * sizeof(char), "char");

                        data[i].range_start = atoi(PQgetvalue(DBQuery, i, col++));
                        data[i].range_stop = atoi(PQgetvalue(DBQuery, i, col++));

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].type_name = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].type_name, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].type_name, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].structure_description = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].structure_description, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].structure_description, 1, stringLength * sizeof(char),
                                  "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].definition = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].definition, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].definition, 1, stringLength * sizeof(char), "char");

                        UDA_LOG(UDA_LOG_DEBUG, "listData     : [%d]\n", i);
                        UDA_LOG(UDA_LOG_DEBUG, "class        : %s\n", data[i].class);
                        UDA_LOG(UDA_LOG_DEBUG, "system       : %s\n", data[i].system);
                        UDA_LOG(UDA_LOG_DEBUG, "device       : %s\n", data[i].device);
                        UDA_LOG(UDA_LOG_DEBUG, "configuration: %s\n", data[i].configuration);
                        UDA_LOG(UDA_LOG_DEBUG, "version      : %d\n", data[i].version);
                        UDA_LOG(UDA_LOG_DEBUG, "revision     : %d\n", data[i].revision);
                        UDA_LOG(UDA_LOG_DEBUG, "status       : %s\n", data[i].status);
                        UDA_LOG(UDA_LOG_DEBUG, "description  : %s\n", data[i].description);
                        UDA_LOG(UDA_LOG_DEBUG, "comment      : %s\n", data[i].comment);
                        UDA_LOG(UDA_LOG_DEBUG, "range_start  : %d\n", data[i].range_start);
                        UDA_LOG(UDA_LOG_DEBUG, "range_stop   : %d\n", data[i].range_stop);
                        UDA_LOG(UDA_LOG_DEBUG, "type_name            : %s\n", data[i].type_name);
                        UDA_LOG(UDA_LOG_DEBUG, "structure_description: %s\n", data[i].structure_description);
                        UDA_LOG(UDA_LOG_DEBUG, "definition           : %s\n", data[i].definition);
                    }
                } else {
                    METADATA_R* data;
                    data = (METADATA_R*)malloc(nrows * sizeof(METADATA_R));
                    addMalloc(logmalloclist, (void*)data, nrows, sizeof(METADATA_R), "METADATA_R");
                    structData = (void*)data;

                    for (i = 0; i < nrows; i++) {
                        col = 0;
                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].class = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].class, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].class, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].system = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].system, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].system, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].device = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].device, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].device, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].configuration = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].configuration, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].configuration, 1, stringLength * sizeof(char), "char");

                        data[i].version = atoi(PQgetvalue(DBQuery, i, col++));
                        data[i].revision = atoi(PQgetvalue(DBQuery, i, col++));

                        stringLength = 12;                        // Test, Development or Production 4, 11 or 10
                        data[i].status = (char*)malloc(stringLength * sizeof(char));
                        addMalloc(logmalloclist, (void*)data[i].status, 1, stringLength * sizeof(char), "char");
                        if (PQgetvalue(DBQuery, i, col)[0] == 'T') {
                            strcpy(data[i].status, "Test");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'D') {
                            strcpy(data[i].status, "Development");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'P') {
                            strcpy(data[i].status, "Production");
                            col++;
                        }

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].description = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].description, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].description, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data[i].comment = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data[i].comment, PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data[i].comment, 1, stringLength * sizeof(char), "char");

                        data[i].range_start = atoi(PQgetvalue(DBQuery, i, col++));
                        data[i].range_stop = atoi(PQgetvalue(DBQuery, i, col++));

                        UDA_LOG(UDA_LOG_DEBUG, "listData     : [%d]\n", i);
                        UDA_LOG(UDA_LOG_DEBUG, "class        : %s\n", data[i].class);
                        UDA_LOG(UDA_LOG_DEBUG, "system       : %s\n", data[i].system);
                        UDA_LOG(UDA_LOG_DEBUG, "device       : %s\n", data[i].device);
                        UDA_LOG(UDA_LOG_DEBUG, "configuration: %s\n", data[i].configuration);
                        UDA_LOG(UDA_LOG_DEBUG, "version      : %d\n", data[i].version);
                        UDA_LOG(UDA_LOG_DEBUG, "revision     : %d\n", data[i].revision);
                        UDA_LOG(UDA_LOG_DEBUG, "status       : %s\n", data[i].status);
                        UDA_LOG(UDA_LOG_DEBUG, "description  : %s\n", data[i].description);
                        UDA_LOG(UDA_LOG_DEBUG, "comment      : %s\n", data[i].comment);
                        UDA_LOG(UDA_LOG_DEBUG, "range_start  : %d\n", data[i].range_start);
                        UDA_LOG(UDA_LOG_DEBUG, "range_stop   : %d\n", data[i].range_stop);
                    }
                }
            } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                if (isStructure) {
                    METADATA_CS* data;
                    data = (METADATA_CS*)malloc(sizeof(METADATA_CS));
                    addMalloc(logmalloclist, (void*)data, 1, sizeof(METADATA_CS), "METADATA_CS");
                    structData = (void*)data;

                    data->class = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->class, nrows, sizeof(char*), "STRING *");
                    data->system = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->system, nrows, sizeof(char*), "STRING *");
                    data->device = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->device, nrows, sizeof(char*), "STRING *");
                    data->configuration = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->configuration, nrows, sizeof(char*), "STRING *");
                    data->status = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->status, nrows, sizeof(char*), "STRING *");
                    data->description = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->description, nrows, sizeof(char*), "STRING *");
                    data->comment = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->comment, nrows, sizeof(char*), "STRING *");

                    data->version = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->version, nrows, sizeof(unsigned int*), "unsigned int");
                    data->revision = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->revision, nrows, sizeof(unsigned int*), "unsigned int");
                    data->range_start = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->range_start, nrows, sizeof(unsigned int*), "unsigned int");
                    data->range_stop = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->range_stop, nrows, sizeof(unsigned int*), "unsigned int");

                    data->type_name = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->type_name, nrows, sizeof(char*), "STRING *");
                    data->structure_description = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->structure_description, nrows, sizeof(char*), "STRING *");
                    data->definition = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->definition, nrows, sizeof(char*), "STRING *");

                    data->count = nrows;

                    for (i = 0; i < nrows; i++) {
                        col = 0;
                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->class[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->class[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->class[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->system[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->system[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->system[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->device[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->device[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->device[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->configuration[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->configuration[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->configuration[i], stringLength, sizeof(char), "char");

                        data->version[i] = atoi(PQgetvalue(DBQuery, i, col++));
                        data->revision[i] = atoi(PQgetvalue(DBQuery, i, col++));

                        stringLength = 12;                        // Test, Development or Production 4, 11 or 10
                        data->status[i] = (char*)malloc(stringLength * sizeof(char));
                        addMalloc(logmalloclist, (void*)data->status[i], stringLength, sizeof(char), "char");
                        if (PQgetvalue(DBQuery, i, col)[0] == 'T') {
                            strcpy(data->status[i], "Test");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'D') {
                            strcpy(data->status[i], "Development");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'P') {
                            strcpy(data->status[i], "Production");
                            col++;
                        }

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->description[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->description[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->comment[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->comment[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->comment[i], stringLength, sizeof(char), "char");
//p = PQfname(DBQuery, col);
                        data->range_start[i] = atoi(PQgetvalue(DBQuery, i, col++));
//p = PQfname(DBQuery, col);
                        data->range_stop[i] = atoi(PQgetvalue(DBQuery, i, col++));
//p = PQfname(DBQuery, col);

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->type_name[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->type_name[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->type_name[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->structure_description[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->structure_description[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->structure_description[i], stringLength, sizeof(char),
                                  "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->definition[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->definition[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->definition[i], stringLength, sizeof(char), "char");

                        UDA_LOG(UDA_LOG_DEBUG, "listData     : [%d]\n", i);
                        UDA_LOG(UDA_LOG_DEBUG, "class        : %s\n", data->class[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "system       : %s\n", data->system[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "device       : %s\n", data->device[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "configuration: %s\n", data->configuration[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "version      : %d\n", data->version[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "revision     : %d\n", data->revision[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "status       : %s\n", data->status[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "description  : %s\n", data->description[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "comment      : %s\n", data->comment[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "range_start  : %d\n", data->range_start[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "range_stop   : %d\n", data->range_stop[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "type_name            : %s\n", data->type_name[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "structure_description: %s\n", data->structure_description[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "definition           : %s\n", data->definition[i]);
                    }
                } else {
                    METADATA_C* data;
                    data = (METADATA_C*)malloc(sizeof(METADATA_C));
                    addMalloc(logmalloclist, (void*)data, 1, sizeof(METADATA_C), "METADATA_C");
                    structData = (void*)data;

                    data->class = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->class, nrows, sizeof(char*), "STRING *");
                    data->system = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->system, nrows, sizeof(char*), "STRING *");
                    data->device = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->device, nrows, sizeof(char*), "STRING *");
                    data->configuration = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->configuration, nrows, sizeof(char*), "STRING *");
                    data->status = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->status, nrows, sizeof(char*), "STRING *");
                    data->description = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->description, nrows, sizeof(char*), "STRING *");
                    data->comment = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)data->comment, nrows, sizeof(char*), "STRING *");

                    data->version = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->version, nrows, sizeof(unsigned int*), "unsigned int");
                    data->revision = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->revision, nrows, sizeof(unsigned int*), "unsigned int");
                    data->range_start = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->range_start, nrows, sizeof(unsigned int*), "unsigned int");
                    data->range_stop = (unsigned int*)malloc(nrows * sizeof(unsigned int));
                    addMalloc(logmalloclist, (void*)data->range_stop, nrows, sizeof(unsigned int*), "unsigned int");

                    data->count = nrows;

                    for (i = 0; i < nrows; i++) {
                        col = 0;
                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->class[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->class[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->class[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->system[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->system[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->system[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->device[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->device[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->device[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->configuration[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->configuration[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->configuration[i], stringLength, sizeof(char), "char");

                        data->version[i] = atoi(PQgetvalue(DBQuery, i, col++));
                        data->revision[i] = atoi(PQgetvalue(DBQuery, i, col++));

                        stringLength = 12;                        // Test, Development or Production 4, 11 or 10
                        data->status[i] = (char*)malloc(stringLength * sizeof(char));
                        addMalloc(logmalloclist, (void*)data->status[i], stringLength, sizeof(char), "char");
                        if (PQgetvalue(DBQuery, i, col)[0] == 'T') {
                            strcpy(data->status[i], "Test");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'D') {
                            strcpy(data->status[i], "Development");
                            col++;
                        } else if (PQgetvalue(DBQuery, i, col)[0] == 'P') {
                            strcpy(data->status[i], "Production");
                            col++;
                        }

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->description[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->description[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, col)) + 1;
                        data->comment[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->comment[i], PQgetvalue(DBQuery, i, col++));
                        addMalloc(logmalloclist, (void*)data->comment[i], stringLength, sizeof(char), "char");

                        data->range_start[i] = atoi(PQgetvalue(DBQuery, i, col++));
                        data->range_stop[i] = atoi(PQgetvalue(DBQuery, i, col++));

                        UDA_LOG(UDA_LOG_DEBUG, "listData     : [%d]\n", i);
                        UDA_LOG(UDA_LOG_DEBUG, "class        : %s\n", data->class[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "system       : %s\n", data->system[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "device       : %s\n", data->device[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "configuration: %s\n", data->configuration[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "version      : %d\n", data->version[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "revision     : %d\n", data->revision[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "status       : %s\n", data->status[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "description  : %s\n", data->description[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "comment      : %s\n", data->comment[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "range_start  : %d\n", data->range_start[i]);
                        UDA_LOG(UDA_LOG_DEBUG, "range_stop   : %d\n", data->range_stop[i]);
                    }
                }
            }

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listData");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                if (isStructure) {
                    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADATA_RS", 0);
                } else {
                    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADATA_R", 0);
                }
            } else if (castTypeId == CASTCOLUMN) {
                if (isStructure) {
                    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADATA_CS", 0);
                } else {
                    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "METADATA_C", 0);
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listData called\n");

            break;

        }

        //----------------------------------------------------------------------------------------
        // Get the Configuration Metadata
        // e.g. meta::getdata(  context=meta [,class=class] [,system=system] [,configuration=configuration]
        //			[,device=device] [,shot=shot]
        //			[,version=version] [,revision=revision])

        if (context == CONTEXT_META &&
            (STR_IEQUALS(request_block->function, "getdata") || STR_IEQUALS(request_block->function, "get"))) {

            strcpy(sql, "SELECT format, path, filename, version, revision FROM Meta_Source_View ");

            wCount = 0;
            if (classDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE class = '");
                else
                    strcat(sql, "AND class = '");
                strcat(sql, request_block->nameValueList.nameValue[classDependent].value);
                strcat(sql, "' ");
            }
            if (systemDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE system = '");
                else
                    strcat(sql, "AND system = '");
                strcat(sql, request_block->nameValueList.nameValue[systemDependent].value);
                strcat(sql, "' ");
            }
            if (configDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE configuration = '");
                else
                    strcat(sql, "AND configuration = '");
                strcat(sql, request_block->nameValueList.nameValue[configDependent].value);
                strcat(sql, "' ");
            }
            if (deviceDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE device = '");
                else
                    strcat(sql, "AND device = '");
                strcat(sql, request_block->nameValueList.nameValue[deviceDependent].value);
                strcat(sql, "' ");
            } else {
                if (wCount++ == 0)
                    strcat(sql, "WHERE device = '");
                else
                    strcat(sql, "AND device = '");
                strcat(sql, API_DEVICE);
                strcat(sql, "' ");
            }
            if (shotDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE (range_start = 0 OR range_start <= ");
                else
                    strcat(sql, "AND (range_start = 0 OR range_start <= ");
                strcat(sql, request_block->nameValueList.nameValue[shotDependent].value);
                strcat(sql, ") AND (range_stop = 0 OR range_stop >= ");
                strcat(sql, request_block->nameValueList.nameValue[shotDependent].value);
                strcat(sql, ") ");
            } else {
                if (exp_number > 0) {
                    sprintf(work, "%d", exp_number);
                    if (wCount++ == 0)
                        strcat(sql, "WHERE (range_start = 0 OR range_start <= ");
                    else
                        strcat(sql, "AND (range_start = 0 OR range_start <= ");
                    strcat(sql, work);
                    strcat(sql, ") AND (range_stop = 0 OR range_stop >= ");
                    strcat(sql, work);
                    strcat(sql, ") ");
                }
            }
            if (versionDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE version = ");
                else
                    strcat(sql, "AND version = ");
                strcat(sql, request_block->nameValueList.nameValue[versionDependent].value);
                strcat(sql, " ");
            }
            if (revisionDependent >= 0) {
                if (wCount++ == 0)
                    strcat(sql, "WHERE revision = ");
                else
                    strcat(sql, "AND revision = ");
                strcat(sql, request_block->nameValueList.nameValue[revisionDependent].value);
                strcat(sql, " ");
            }

            if (isLatest) strcat(sql, "ORDER BY version DESC, revision DESC limit 1");

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: SQL\n%s\n", sql);

// Execute the SQL

            UDA_LOG(UDA_LOG_DEBUG, "Query (meta) : %s\n", sql);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

            if (nrows > 1) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Too many Meta Data files identified.!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "Too many Meta Data files identified. "
                        "Please refine your selection");
                break;
            }

// Identify the Data Source file

            sprintf(signal_desc->signal_name, "/");            // *** ONLY TRUE FOR NETCDF
            sprintf(data_source->format, "%s", PQgetvalue(DBQuery, 0, 0));
            sprintf(data_source->path, "%s", PQgetvalue(DBQuery, 0, 1));
            sprintf(data_source->filename, "%s", PQgetvalue(DBQuery, 0, 2));

            UDA_LOG(UDA_LOG_DEBUG, "getData\n");
            UDA_LOG(UDA_LOG_DEBUG, "signal_name: %s\n", signal_desc->signal_name);
            UDA_LOG(UDA_LOG_DEBUG, "format     : %s\n", data_source->format);
            UDA_LOG(UDA_LOG_DEBUG, "path       : %s\n", data_source->path);
            UDA_LOG(UDA_LOG_DEBUG, "file       : %s\n", data_source->filename);

// Flag access is via a different server plugin

            idam_plugin_interface->changePlugin = 1;

// Housekeeping

            PQclear(DBQuery);

            break;
        } else

//----------------------------------------------------------------------------------------
// DATA Context
//----------------------------------------------------------------------------------------
// Last Plasma Shot Number
// e.g. meta::getData(context=data, /lastshot)

        if (context == CONTEXT_DATA && isLastShot &&
            (STR_IEQUALS(request_block->function, "getdata") || STR_IEQUALS(request_block->function, "get"))) {

            strcpy(sql, "SELECT max(exp_number) FROM ExpDateTime;");

// Execute the SQL

            UDA_LOG(UDA_LOG_DEBUG, "Query (lastshot) : %s\n", sql);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "DATALASTSHOT");
            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(DATALASTSHOT);        // Structure size
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;
            defineField(&field, "lastshot", "last shot number", &offset, SCALARUINT);
            addCompoundField(&usertype, field);        // Single Structure element
            addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

            DATALASTSHOT* data;
            data = (DATALASTSHOT*)malloc(sizeof(DATALASTSHOT));    // Structured Data Must be a heap variable
            addMalloc(logmalloclist, (void*)data, 1, sizeof(DATALASTSHOT), "DATALASTSHOT");

            data->lastshot = atoi(PQgetvalue(DBQuery, 0, 0));

            UDA_LOG(UDA_LOG_DEBUG, "getLastShot: %d\n", data->lastshot);

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "getLastShot");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATALASTSHOT", 0);

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function getLastShot called\n");

            break;

        } else if (context == CONTEXT_DATA && isLastPass && sourceDependent >= 0 && exp_number > 0
                   &&
                   (STR_IEQUALS(request_block->function, "getdata") || STR_IEQUALS(request_block->function, "get"))) {
            sprintf(sql,
                    "SELECT pass FROM data_source WHERE source_alias='%s' AND exp_number='%d' ORDER BY pass DESC LIMIT 1",
                    strlwr(request_block->nameValueList.nameValue[sourceDependent].value),
                    exp_number);

            UDA_LOG(UDA_LOG_DEBUG, "Meta: sql query %s\n", sql);

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

            if (nrows > 1) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Too many Meta Data records returned!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "Too many Meta Data records returned!");
                break;
            }

            // Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "DATALASTPASS");
            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(DATALASTPASS);        // Structure size
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;
            defineField(&field, "lastpass", "last pass number", &offset, SCALARUINT);
            addCompoundField(&usertype, field);        // Single Structure element
            addUserDefinedType(userdefinedtypelist, usertype);

            // Create Data

            DATALASTPASS* data;
            data = (DATALASTPASS*)malloc(sizeof(DATALASTPASS));    // Structured Data Must be a heap variable
            addMalloc(logmalloclist, (void*)data, 1, sizeof(DATALASTPASS), "DATALASTPASS");

            data->lastpass = atoi(PQgetvalue(DBQuery, 0, 0));

            PQclear(DBQuery);

            // Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "getLastPass");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATALASTPASS", 0);

            break;

        } else

            //--------------------------------------------------------------------------------------------------------------------------
            // Shot Date and Time (shot=shot)
            // e.g. meta::getData(context=data, shot=shot, /shotdatetime)

        if (context == CONTEXT_DATA && isShotDateTime &&
            (STR_IEQUALS(request_block->function, "getdata") || STR_IEQUALS(request_block->function, "get"))) {

            if (exp_number > 0) {
                sprintf(sql, "SELECT exp_date, exp_time FROM ExpDateTime WHERE exp_number=%d", exp_number);
            } else {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Shot Number passed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Shot Number passed!");
                break;
            }

            // Execute the SQL

            UDA_LOG(UDA_LOG_DEBUG, "Query (isshotdatetime) %s\n", sql);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

            if (nrows > 1) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Too many Meta Data records returned!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "Too many Meta Data records returned!");
                break;
            }

            // Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "DATASHOTDATETIME");
            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.size = sizeof(DATASHOTDATETIME);    // Structure size
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "shot", "shot number", &offset, SCALARUINT);
            addCompoundField(&usertype, field);
            defineField(&field, "date", "shot date", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "time", "shot time", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

            // Create Data

            DATASHOTDATETIME* data;
            data = (DATASHOTDATETIME*)malloc(sizeof(DATASHOTDATETIME));    // Structured Data Must be a heap variable
            addMalloc(logmalloclist, (void*)data, 1, sizeof(DATASHOTDATETIME), "DATASHOTDATETIME");

            data->shot = exp_number;

            stringLength = strlen(PQgetvalue(DBQuery, 0, 0)) + 1;
            data->date = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->date, PQgetvalue(DBQuery, 0, 0));
            addMalloc(logmalloclist, (void*)data->date, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 1)) + 1;
            data->time = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->time, PQgetvalue(DBQuery, 0, 1));
            addMalloc(logmalloclist, (void*)data->time, 1, stringLength * sizeof(char), "char");

            UDA_LOG(UDA_LOG_DEBUG, "getShotDateTime:\n");
            UDA_LOG(UDA_LOG_DEBUG, "Shot: %d\n", data->shot);
            UDA_LOG(UDA_LOG_DEBUG, "Date: %s\n", data->date);
            UDA_LOG(UDA_LOG_DEBUG, "Time: %s\n", data->time);

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "getShotDateTime");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASHOTDATETIME", 0);

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function getShotDateTime called\n");

            break;

        } else

            //--------------------------------------------------------------------------------------------------------------------------
            // List Data Signals (or dump if a private file has been specified)
            // e.g. meta::listData(context=data [,cast={row|column}][,source_alias=source_alias][,type={A|R|M|I}]
            //                     [,shot=shot][,pass=pass][file=file])

            // Increase Rank of string arrays and reduce rank of passed structure for improved performance

        if (context == CONTEXT_DATA && !isListClasses && !isListSources &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            if (0 && fileDependent >= 0) {        // difficult as need a new request_block

                // DUMP the contents of a private file

                strcpy(request_block->archive, "DUMP");
                strcpy(request_block->path, request_block->nameValueList.nameValue[fileDependent].value);

                data_source->filename[0] = '\0';
                data_source->format[0] = '\0';

                UDA_LOG(UDA_LOG_DEBUG, "listData\n");
                UDA_LOG(UDA_LOG_DEBUG, "signal_name: %s\n", signal_desc->signal_name);
                UDA_LOG(UDA_LOG_DEBUG, "path       : %s\n", data_source->path);
                UDA_LOG(UDA_LOG_DEBUG, "file       : %s\n", data_source->filename);

                // Flag access is via a different server plugin

                idam_plugin_interface->changePlugin = 1;
                return 0;
            }

            sql[0] = '\0';
            if (sourceDependent >= 0) {
                strcat(sql, "AND source_alias = '");
                strcat(sql, strlwr(request_block->nameValueList.nameValue[sourceDependent].value));
                strcat(sql, "' ");
            }
            if (typeDependent >= 0) {
                work[0] = toupper(request_block->nameValueList.nameValue[typeDependent].value[0]);
                work[1] = '\0';
                strcat(sql, "AND type = '");
                strcat(sql, work);
                strcat(sql, "' ");
            }
            strcpy(work, sql);

            // Signal match string
            char* signal_match_escaped = NULL;
            if (signalMatchDependent >= 0) {
                signal_match_escaped = (char*)malloc((2 * strlen(signal_match) + 1) * sizeof(char));
                int err_inj = 0;
                PQescapeStringConn(DBConnect, signal_match_escaped, signal_match, strlen(signal_match), &err_inj);
            }

            // Desc match string
            char* desc_match_escaped = NULL;
            if (descDependent >= 0) {
                desc_match_escaped = (char*)malloc((2 * strlen(description) + 1) * sizeof(char));
                int err_inj = 0;
                PQescapeStringConn(DBConnect, desc_match_escaped, description, strlen(description), &err_inj);
            }

// *** status ***
            if (exp_number > 0) {
                if (pass > -1) {
                    sprintf(sql,
                            "SELECT signal_alias, generic_name, source_alias, type, description, signal_status, mds_name FROM Signal_Desc as D, "
                                    "(SELECT DISTINCT signal_desc_id, signal_status from Signal as A, (SELECT source_id FROM Data_Source WHERE "
                                    "exp_number = %d AND pass = %d %s) as B WHERE A.source_id = B.source_id) as C WHERE "
                                    "D.signal_desc_id = C.signal_desc_id ", exp_number, pass, work);

                    if (signalMatchDependent >= 0) {
                        strcat(sql, " AND D.signal_alias ILIKE '%");
                        strcat(sql, signal_match_escaped);
                        strcat(sql, "%'");
                    }
                    if (descDependent >= 0) {
                        strcat(sql, " AND D.description ILIKE '%");
                        strcat(sql, desc_match_escaped);
                        strcat(sql, "%'");
                    }

                    strcat(sql, " ORDER BY signal_alias ASC");

                } else {
                    sprintf(sql,
                            "SELECT signal_alias, generic_name, source_alias, type, description, signal_status, mds_name FROM Signal_Desc as D, "
                                    "(SELECT DISTINCT signal_desc_id, signal_status from Signal as A, "
                                    " (SELECT source_id FROM Data_Source WHERE exp_number = %d %s) "
                                    " as B WHERE A.source_id = B.source_id) as C "
                                    " WHERE D.signal_desc_id = C.signal_desc_id ", exp_number, work);

                    if (signalMatchDependent >= 0) {
                        strcat(sql, " AND D.signal_alias ILIKE '%");
                        strcat(sql, signal_match_escaped);
                        strcat(sql, "%'");
                    }
                    if (descDependent >= 0) {
                        strcat(sql, " AND D.description ILIKE '%");
                        strcat(sql, desc_match_escaped);
                        strcat(sql, "%'");
                    }

                    strcat(sql, " ORDER BY signal_alias ASC");

                }
            } else {
                if (work[0] == '\0') {
                    sprintf(sql, "SELECT signal_alias, generic_name, source_alias, type, description, mds_name FROM Signal_Desc");
                    if (signalMatchDependent >= 0) {
                        strcat(sql, " AND signal_alias ILIKE '%");
                        strcat(sql, signal_match_escaped);
                        strcat(sql, "%'");
                    }
                    if (descDependent >= 0) {
                        strcat(sql, " AND description ILIKE '%");
                        strcat(sql, desc_match_escaped);
                        strcat(sql, "%'");
                    }
                    strcat(sql, " ORDER BY signal_alias ASC");

                } else {
                    sprintf(sql,
                            "SELECT signal_alias, generic_name, source_alias, type, description, mds_name FROM Signal_Desc WHERE "
                                    "%s", &work[4]);  // skip the AND
                    if (signalMatchDependent >= 0) {
                        strcat(sql, " AND signal_alias ILIKE '%");
                        strcat(sql, signal_match_escaped);
                        strcat(sql, "%'");
                    }
                    if (descDependent >= 0) {
                        strcat(sql, " AND description ILIKE '%");
                        strcat(sql, desc_match_escaped);
                        strcat(sql, "%'");
                    }
                    strcat(sql, " ORDER BY signal_alias ASC");

                }
            }

// Execute the SQL
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR DATA::listData: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR DATA::listData: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR META::listData: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "DATALISTSIGNALS_R");        // Default is Row Oriented
                usertype.size = sizeof(DATALISTSIGNALS_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "DATALISTSIGNALS_C");
                usertype.size = sizeof(DATALISTSIGNALS_C);        // Selected return structure size
            }

            strcpy(usertype.source, "listData");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
                defineField(&field, "shot", "shot number", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
                defineField(&field, "pass", "pass number", &offset, SCALARINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "signal_name", "signal name", &offset,
                        stringTypeId);    // Array or Single string, arbitrary length
// **** NameSpace collision: remove from libidamr.so
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);
            defineField(&field, "generic_name", "generic name", &offset, stringTypeId);
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);
            defineField(&field, "source_alias", "source alias name", &offset, stringTypeId);
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);
            defineField(&field, "type", "data type classification", &offset, stringTypeId);
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);
            defineField(&field, "description", "data description", &offset, stringTypeId);
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);
            defineField(&field, "signal_status", "signal status", &offset, intTypeId);
            field.atomictype = UDA_TYPE_INT;
            addCompoundField(&usertype, field);
            defineField(&field, "mds_name", "MDSPLUS name", &offset, stringTypeId);
            field.atomictype = UDA_TYPE_STRING;
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

            if (castTypeId == CASTROW) {
                initUserDefinedType(&usertype);
                strcpy(usertype.name, "DATALISTSIGNALS_RR");
                usertype.size = sizeof(DATALISTSIGNALS_RR);
                strcpy(usertype.source, "listData");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = UDA_TYPE_COMPOUND;
                offset = 0;
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
                defineField(&field, "shot", "shot number", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
                defineField(&field, "pass", "pass number", &offset, SCALARINT);
                addCompoundField(&usertype, field);

                initCompoundField(&field);
                strcpy(field.name, "datalistsignals");
                field.atomictype = UDA_TYPE_UNKNOWN;
                strcpy(field.type, "DATALISTSIGNALS_R");
                strcpy(field.desc, "[DATALISTSIGNALS_R *datalistsignals] Metadata records");
                field.pointer = 1;
                field.count = 1;
                field.rank = 0;
                field.shape = NULL;
                field.size = field.count * sizeof(DATALISTSIGNALS_R*);
                field.offset = newoffset(offset, field.type);
                field.offpad = padding(offset, field.type);
                field.alignment = getalignmentof(field.type);
                offset = field.offset + field.size;    // Next Offset
                addCompoundField(&usertype, field);        // Single Structure element

                addUserDefinedType(userdefinedtypelist, usertype);
            }

// Create Data

            UDA_LOG(UDA_LOG_DEBUG, "listData:\n");
            UDA_LOG(UDA_LOG_DEBUG, "Shot: %d\n", exp_number);
            UDA_LOG(UDA_LOG_DEBUG, "Pass: %d\n", pass);

            if (castTypeId == CASTROW) {                // Row oriented

                DATALISTSIGNALS_RR* dataR;
                dataR = (DATALISTSIGNALS_RR*)malloc(sizeof(DATALISTSIGNALS_RR));
                addMalloc(logmalloclist, (void*)dataR, 1, sizeof(DATALISTSIGNALS_RR), "DATALISTSIGNALS_RR");
                structData = (void*)dataR;

                DATALISTSIGNALS_R* data;
                data = (DATALISTSIGNALS_R*)malloc(nrows * sizeof(DATALISTSIGNALS_R));
                addMalloc(logmalloclist, (void*)data, nrows, sizeof(DATALISTSIGNALS_R), "DATALISTSIGNALS_R");

                dataR->list = data;
                dataR->count = nrows;
                dataR->shot = exp_number;
                dataR->pass = pass;

                for (i = 0; i < nrows; i++) {

                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data[i].signal_name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].signal_name, PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data[i].signal_name, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data[i].generic_name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].generic_name, PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data[i].generic_name, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data[i].source_alias = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].source_alias, PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data[i].source_alias, 1, stringLength * sizeof(char), "char");

                    p = PQgetvalue(DBQuery, i, 3);
                    if (p[0] == 'A') {
                        strcpy(work, "Analysed");
                    } else if (p[0] == 'R') {
                        strcpy(work, "Raw");
                    } else if (p[0] == 'I') {
                        strcpy(work, "Image");
                    } else if (p[0] == 'M') {
                        strcpy(work, "Modelled");
                    } else
                        strcpy(work, p);

                    stringLength = strlen(work) + 1;
                    data[i].type = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].type, work);
                    addMalloc(logmalloclist, (void*)data[i].type, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data[i].description = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].description, PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data[i].description, 1, stringLength * sizeof(char), "char");

                    int next_num = 5;
                    if (exp_number > 0) {
                        data[i].signal_status = (int)atoi(PQgetvalue(DBQuery, i, 5));
                        next_num = next_num + 1;

                    } else {
                        data[i].signal_status = 1;
                    }

                    if (!PQgetisnull(DBQuery, i, next_num)) {
                        stringLength = strlen(PQgetvalue(DBQuery, i, next_num)) + 1;
                        data[i].mds_name = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].mds_name, PQgetvalue(DBQuery, i, next_num));
                        addMalloc(logmalloclist, (void *) data[i].mds_name, 1, stringLength * sizeof(char), "char");
                    } else {
                        data[i].mds_name = (char *) malloc(sizeof(char));
                        strcpy(data[i].mds_name, "");
                        addMalloc(logmalloclist, (void *) data[i].mds_name, 1, sizeof(char), "char");
                    }


                    UDA_LOG(UDA_LOG_DEBUG, "signal_name : %s\n", data[i].signal_name);
                    UDA_LOG(UDA_LOG_DEBUG, "generic_name: %s\n", data[i].generic_name);
                    UDA_LOG(UDA_LOG_DEBUG, "source_alias: %s\n", data[i].source_alias);
                    UDA_LOG(UDA_LOG_DEBUG, "type        : %s\n", data[i].type);
                    UDA_LOG(UDA_LOG_DEBUG, "description : %s\n", data[i].description);
                }

            } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                DATALISTSIGNALS_C* data;
                data = (DATALISTSIGNALS_C*)malloc(sizeof(DATALISTSIGNALS_C));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(DATALISTSIGNALS_C), "DATALISTSIGNALS_C");
                structData = (void*)data;

                data->signal_name = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->signal_name, nrows, sizeof(char*), "STRING *");
                data->generic_name = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->generic_name, nrows, sizeof(char*), "STRING *");
                data->source_alias = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->source_alias, nrows, sizeof(char*), "STRING *");
                data->type = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->type, nrows, sizeof(char*), "STRING *");
                data->description = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->description, nrows, sizeof(char*), "STRING *");
                data->signal_status = (int*)malloc(nrows * sizeof(int));
                addMalloc(logmalloclist, (void*)data->signal_status, nrows, sizeof(int), "INT");
                data->mds_name = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->mds_name, nrows, sizeof(char*), "STRING *");

                data->shot = exp_number;
                data->pass = pass;
                data->count = nrows;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data->signal_name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->signal_name[i], PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data->signal_name[i], stringLength, sizeof(char), "STRING");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data->generic_name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->generic_name[i], PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data->generic_name[i], stringLength, sizeof(char), "STRING");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data->source_alias[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->source_alias[i], PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data->source_alias[i], stringLength, sizeof(char), "STRING");

                    p = PQgetvalue(DBQuery, i, 3);
                    if (p[0] == 'A') {
                        strcpy(work, "Analysed");
                    } else if (p[0] == 'R') {
                        strcpy(work, "Raw");
                    } else if (p[0] == 'I') {
                        strcpy(work, "Image");
                    } else if (p[0] == 'M') {
                        strcpy(work, "Modelled");
                    } else
                        strcpy(work, p);

                    stringLength = strlen(work) + 1;
                    data->type[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->type[i], work);
                    addMalloc(logmalloclist, (void*)data->type[i], stringLength, sizeof(char), "STRING");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data->description[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->description[i], PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "STRING");

                    int next_num = 5;
                    if (exp_number > 0) {
                        data->signal_status[i] = (int)atoi(PQgetvalue(DBQuery, i, 5));

                        next_num = next_num + 1;
                    } else {
                        data->signal_status[i] = 1;
                    }

                    if (!PQgetisnull(DBQuery, i, next_num)) {
                        stringLength = strlen(PQgetvalue(DBQuery, i, next_num)) + 1;
                        data->mds_name[i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(data->mds_name[i], PQgetvalue(DBQuery, i, next_num));
                        addMalloc(logmalloclist, (void*)data->mds_name[i], stringLength, sizeof(char), "STRING");
                    } else {
                        data->mds_name[i] = (char*)malloc(sizeof(char));
                        strcpy(data->mds_name[i], "");
                        addMalloc(logmalloclist, (void*)data->mds_name[i], 1, sizeof(char), "STRING");
                    }

//                    UDA_LOG(UDA_LOG_DEBUG, "signal_name : %s\n", data->signal_name[i]);
//                    UDA_LOG(UDA_LOG_DEBUG, "generic_name: %s\n", data->generic_name[i]);
//                    UDA_LOG(UDA_LOG_DEBUG, "source_alias: %s\n", data->source_alias[i]);
//                    UDA_LOG(UDA_LOG_DEBUG, "type        : %s\n", data->type[i]);
//                    UDA_LOG(UDA_LOG_DEBUG, "description : %s\n", data->description[i]);
//                    UDA_LOG(UDA_LOG_DEBUG, "mds_name : %s\n", data->mds_name[i]);
                }
            }

            PQclear(DBQuery);

            if (signalMatchDependent >= 0) {
                free(signal_match_escaped);
            }
            if (descDependent >= 0) {
                free(desc_match_escaped);
            }

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listData");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATALISTSIGNALS_RR", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATALISTSIGNALS_C", 0);
            }

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listData called\n");

            break;
        } else

            //--------------------------------------------------------------------------------------------------------------------------
            // Experimental and Modelled Data Signals and Sources
            // e.g. meta::listData(context=data, shot=shot, /listsources [,cast={row|column}][,source_alias=source_alias][,type=type][,pass=pass])

        if (context == CONTEXT_DATA && !isListClasses && isListSources &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            if (exp_number > 0) {
                sql[0] = '\0';
                if (sourceDependent >= 0) {
                    strcat(sql, "AND source_alias = '");
                    strcat(sql, strlwr(request_block->nameValueList.nameValue[sourceDependent].value));
                    strcat(sql, "' ");
                }
                if (typeDependent >= 0) {
                    work[0] = toupper(request_block->nameValueList.nameValue[typeDependent].value[0]);
                    work[1] = '\0';
                    strcat(sql, "AND type = '");
                    strcat(sql, work);
                    strcat(sql, "' ");
                }
                strcpy(work, sql);

                if (pass > -1) {
                    sprintf(sql,
                            "SELECT source_alias, pass, source_status, format, filename, type, run_id FROM Data_Source WHERE "
                                    "exp_number=%d AND pass=%d %s ORDER BY source_alias ASC, pass DESC", exp_number,
                            pass, work);
                } else {
                    sprintf(sql,
                            "SELECT source_alias, pass, source_status, format, filename, type, run_id FROM Data_Source WHERE "
                                    "exp_number=%d %s ORDER BY source_alias ASC, pass DESC", exp_number, work);
                }

            // Execute the SQL
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

            // Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "DATALISTSOURCES_R");        // Default is Row Oriented
                usertype.size = sizeof(DATALISTSOURCES_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "DATALISTSOURCES_C");
                usertype.size = sizeof(DATALISTSOURCES_C);
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

                if (castTypeId == CASTCOLUMN) {
                    defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);
                    defineField(&field, "shot", "shot number", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);
                }

                defineField(&field, "pass", "pass number", &offset, intTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "status", "source status value", &offset, intTypeId);
                addCompoundField(&usertype, field);

                defineField(&field, "source_alias", "data source alias name", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "format", "data source file format", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "filename", "data source filename", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "type", "data type classification", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "run_id", "run_id for eg. TRANSP", &offset, intTypeId);
                addCompoundField(&usertype, field);

                addUserDefinedType(userdefinedtypelist, usertype);

                if (castTypeId == CASTROW) {
                    initUserDefinedType(&usertype);
                    strcpy(usertype.name, "DATALISTSOURCES_RR");
                    usertype.size = sizeof(DATALISTSOURCES_RR);
                    strcpy(usertype.source, "listData");
                    usertype.ref_id = 0;
                    usertype.imagecount = 0;                // No Structure Image data
                    usertype.image = NULL;
                    usertype.idamclass = UDA_TYPE_COMPOUND;

                    offset = 0;
                    defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);
                    defineField(&field, "shot", "shot number", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);

                    initCompoundField(&field);
                    strcpy(field.name, "datalistsources");
                    field.atomictype = UDA_TYPE_UNKNOWN;
                    strcpy(field.type, "DATALISTSOURCES_R");
                    strcpy(field.desc, "[DATALISTSOURCES_R *datalistsources] Metadata records");
                    field.pointer = 1;
                    field.count = 1;
                    field.rank = 0;
                    field.shape = NULL;
                    field.size = field.count * sizeof(DATALISTSOURCES_R *);
                    field.offset = newoffset(offset, field.type);
                    field.offpad = padding(offset, field.type);
                    field.alignment = getalignmentof(field.type);
                    offset = field.offset + field.size;
                    addCompoundField(&usertype, field);

                    addUserDefinedType(userdefinedtypelist, usertype);
                }

                USERDEFINEDTYPE *udt = findUserDefinedType(userdefinedtypelist, "DATALISTSOURCES_R", 0);
                int size = getStructureSize(userdefinedtypelist, udt);
                UDA_LOG(UDA_LOG_DEBUG, "sizeof(DATALISTSOURCES_R) = %zu [%d]\n", sizeof(DATALISTSOURCES_R), size);
                printUserDefinedTypeListTable(*userdefinedtypelist);

                // Create Data

                UDA_LOG(UDA_LOG_DEBUG, "listSources:\n");
                UDA_LOG(UDA_LOG_DEBUG, "Shot: %d\n", exp_number);

                if (castTypeId == CASTROW) {                // Row oriented

                    DATALISTSOURCES_RR *dataR;
                    dataR = (DATALISTSOURCES_RR *) malloc(sizeof(DATALISTSOURCES_RR));
                    addMalloc(logmalloclist, (void *) dataR, 1, sizeof(DATALISTSOURCES_RR), "DATALISTSOURCES_RR");
                    structData = (void *) dataR;

                    DATALISTSOURCES_R *data;
                    data = (DATALISTSOURCES_R *) malloc(nrows * sizeof(DATALISTSOURCES_R));
                    addMalloc(logmalloclist, (void *) data, nrows, sizeof(DATALISTSOURCES_R), "DATALISTSOURCES_R");

                    dataR->list = data;
                    dataR->count = nrows;
                    dataR->shot = exp_number;

                    for (i = 0; i < nrows; i++) {

                        stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                        data[i].source_alias = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].source_alias, PQgetvalue(DBQuery, i, 0));
                        addMalloc(logmalloclist, (void *) data[i].source_alias, 1, stringLength * sizeof(char), "char");

                        data[i].pass = (int) atoi(PQgetvalue(DBQuery, i, 1));
                        data[i].status = (short) atoi(PQgetvalue(DBQuery, i, 2));

                        stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                        data[i].format = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].format, PQgetvalue(DBQuery, i, 3));
                        addMalloc(logmalloclist, (void *) data[i].format, 1, stringLength * sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                        data[i].filename = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].filename, PQgetvalue(DBQuery, i, 4));
                        addMalloc(logmalloclist, (void *) data[i].filename, 1, stringLength * sizeof(char), "char");

                        p = PQgetvalue(DBQuery, i, 5);
                        if (p[0] == 'A') {
                            strcpy(work, "Analysed");
                        } else if (p[0] == 'R') {
                            strcpy(work, "Raw");
                        } else if (p[0] == 'I') {
                            strcpy(work, "Image");
                        } else if (p[0] == 'M') {
                            strcpy(work, "Modelled");
                        } else
                            strcpy(work, p);

                        stringLength = strlen(work) + 1;
                        data[i].type = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].type, work);
                        addMalloc(logmalloclist, (void *) data[i].type, 1, stringLength * sizeof(char), "char");

                        if (!PQgetisnull(DBQuery, i, 6)) {
                            data[i].run_id = (int) atoi(PQgetvalue(DBQuery, i, 6));
                        } else {
                            data[i].run_id = -1;
                        }

                        UDA_LOG(UDA_LOG_DEBUG, "Pass        : %d\n", data[i].pass);
                        UDA_LOG(UDA_LOG_DEBUG, "source_alias: %s\n", data[i].source_alias);
                        UDA_LOG(UDA_LOG_DEBUG, "Status      : %d\n", data[i].status);
                        UDA_LOG(UDA_LOG_DEBUG, "Format      : %s\n", data[i].format);
                        UDA_LOG(UDA_LOG_DEBUG, "Filename    : %s\n", data[i].filename);
                        UDA_LOG(UDA_LOG_DEBUG, "Type        : %s\n", data[i].type);
                    }
                } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                    UDA_LOG(UDA_LOG_DEBUG, "Cast as column\n");

                    DATALISTSOURCES_C *data;
                    data = (DATALISTSOURCES_C *) malloc(sizeof(DATALISTSOURCES_C));
                    addMalloc(logmalloclist, (void *) data, 1, sizeof(DATALISTSOURCES_C), "DATALISTSOURCES_C");
                    structData = (void *) data;

                    data->pass = (int *) malloc(nrows * sizeof(int));
                    addMalloc(logmalloclist, (void *) data->pass, nrows, sizeof(int), "int");
                    data->status = (short *) malloc(nrows * sizeof(short));
                    addMalloc(logmalloclist, (void *) data->status, nrows, sizeof(short), "short");

                    data->source_alias = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->source_alias, nrows, sizeof(char *), "STRING *");
                    data->format = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->format, nrows, sizeof(char *), "STRING *");
                    data->filename = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->filename, nrows, sizeof(char *), "STRING *");
                    data->type = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->type, nrows, sizeof(char *), "STRING *");

                    data->run_id = (int *) malloc(nrows * sizeof(int));
                    addMalloc(logmalloclist, (void *) data->run_id, nrows, sizeof(int), "int");

                    data->count = nrows;
                    data->shot = exp_number;

                    for (i = 0; i < nrows; i++) {

                        stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                        data->source_alias[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->source_alias[i], PQgetvalue(DBQuery, i, 0));
                        addMalloc(logmalloclist, (void *) data->source_alias[i], stringLength, sizeof(char), "char");

                        data->pass[i] = (unsigned int) atoi(PQgetvalue(DBQuery, i, 1));
                        data->status[i] = (short) atoi(PQgetvalue(DBQuery, i, 2));


                        stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                        data->format[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->format[i], PQgetvalue(DBQuery, i, 3));
                        addMalloc(logmalloclist, (void *) data->format[i], stringLength, sizeof(char), "char");

                        stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                        data->filename[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->filename[i], PQgetvalue(DBQuery, i, 4));
                        addMalloc(logmalloclist, (void *) data->filename[i], stringLength, sizeof(char), "char");

                        p = PQgetvalue(DBQuery, i, 5);
                        if (p[0] == 'A') {
                            strcpy(work, "Analysed");
                        } else if (p[0] == 'R') {
                            strcpy(work, "Raw");
                        } else if (p[0] == 'I') {
                            strcpy(work, "Image");
                        } else if (p[0] == 'M') {
                            strcpy(work, "Modelled");
                        } else {
                            strcpy(work, "");
                        }

                        stringLength = strlen(work) + 1;
                        data->type[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->type[i], work);
                        addMalloc(logmalloclist, (void *) data->type[i], stringLength, sizeof(char), "char");

                        if (!PQgetisnull(DBQuery, i, 6)) {
                            data->run_id[i] = atoi(PQgetvalue(DBQuery, i, 6));
                        } else {
                            data->run_id[i] = -1;
                        }
                    }
                }

                PQclear(DBQuery);

                // Pass Data

                data_block->data_type = UDA_TYPE_COMPOUND;
                data_block->rank = 0;
                data_block->data_n = 1;
                data_block->data = (char *) structData;

                strcpy(data_block->data_desc, "listSources");
                strcpy(data_block->data_label, "");
                strcpy(data_block->data_units, "");

                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                data_block->opaque_count = 1;
                if (castTypeId == CASTROW) {
                    data_block->opaque_block = (void *) findUserDefinedType(userdefinedtypelist, "DATALISTSOURCES_RR",
                                                                            0);
                } else if (castTypeId == CASTCOLUMN) {
                    data_block->opaque_block = (void *) findUserDefinedType(userdefinedtypelist, "DATALISTSOURCES_C",
                                                                            0);
                }

                UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listSources called\n");

                break;
            } else {
                sql[0] = '\0';
                if (typeDependent >= 0) {
                    work[0] = toupper(request_block->nameValueList.nameValue[typeDependent].value[0]);
                    work[1] = '\0';
                    strcat(sql, "WHERE type = '");
                    strcat(sql, work);
                    strcat(sql, "' ");
                }
                strcpy(work, sql);

                sprintf(sql,
                        "SELECT distinct(source_alias), type FROM Data_Source "
                                "%s ORDER BY source_alias ASC", work);

                // Execute the SQL
                if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                    err = 999;
                    UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Database Query Failed!\n");
                    addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                    break;
                }

                if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                    UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                    break;
                }

                nrows = PQntuples(DBQuery);

                if (nrows == 0) {
                    UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                    break;
                }

                // Create the Returned Structure Definition

                initUserDefinedType(&usertype);            // New structure definition

                if (castTypeId == CASTROW) {
                    strcpy(usertype.name, "DATALISTALLSOURCES_R");        // Default is Row Oriented
                    usertype.size = sizeof(DATALISTALLSOURCES_R);
                } else if (castTypeId == CASTCOLUMN) {
                    strcpy(usertype.name, "DATALISTALLSOURCES_C");
                    usertype.size = sizeof(DATALISTALLSOURCES_C);
                }

                strcpy(usertype.source, "readMeta");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = UDA_TYPE_COMPOUND;

                offset = 0;

                if (castTypeId == CASTCOLUMN) {
                    defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);
                }

                defineField(&field, "source_alias", "data source alias name", &offset, stringTypeId);
                addCompoundField(&usertype, field);
                defineField(&field, "type", "data type classification", &offset, stringTypeId);
                addCompoundField(&usertype, field);

                addUserDefinedType(userdefinedtypelist, usertype);

                if (castTypeId == CASTROW) {
                    initUserDefinedType(&usertype);
                    strcpy(usertype.name, "DATALISTALLSOURCES_RR");
                    usertype.size = sizeof(DATALISTALLSOURCES_RR);
                    strcpy(usertype.source, "listData");
                    usertype.ref_id = 0;
                    usertype.imagecount = 0;                // No Structure Image data
                    usertype.image = NULL;
                    usertype.idamclass = UDA_TYPE_COMPOUND;

                    offset = 0;
                    defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                    addCompoundField(&usertype, field);

                    initCompoundField(&field);
                    strcpy(field.name, "datalistallsources");
                    field.atomictype = UDA_TYPE_UNKNOWN;
                    strcpy(field.type, "DATALISTALLSOURCES_R");
                    strcpy(field.desc, "[DATALISTALLSOURCES_R *datalistallsources] Metadata records");
                    field.pointer = 1;
                    field.count = 1;
                    field.rank = 0;
                    field.shape = NULL;
                    field.size = field.count * sizeof(DATALISTALLSOURCES_R *);
                    field.offset = newoffset(offset, field.type);
                    field.offpad = padding(offset, field.type);
                    field.alignment = getalignmentof(field.type);
                    offset = field.offset + field.size;
                    addCompoundField(&usertype, field);

                    addUserDefinedType(userdefinedtypelist, usertype);
                }

                USERDEFINEDTYPE *udt = findUserDefinedType(userdefinedtypelist, "DATALISTALLSOURCES_R", 0);
                int size = getStructureSize(userdefinedtypelist, udt);
                UDA_LOG(UDA_LOG_DEBUG, "sizeof(DATALISTALLSOURCES_R) = %zu [%d]\n", sizeof(DATALISTALLSOURCES_R), size);
                printUserDefinedTypeListTable(*userdefinedtypelist);

                // Create Data

                UDA_LOG(UDA_LOG_DEBUG, "listAllSources:\n");

                if (castTypeId == CASTROW) {                // Row oriented

                    DATALISTALLSOURCES_RR *dataR;
                    dataR = (DATALISTALLSOURCES_RR *) malloc(sizeof(DATALISTALLSOURCES_RR));
                    addMalloc(logmalloclist, (void *) dataR, 1, sizeof(DATALISTALLSOURCES_RR), "DATALISTALLSOURCES_RR");
                    structData = (void *) dataR;

                    DATALISTALLSOURCES_R *data;
                    data = (DATALISTALLSOURCES_R *) malloc(nrows * sizeof(DATALISTALLSOURCES_R));
                    addMalloc(logmalloclist, (void *) data, nrows, sizeof(DATALISTALLSOURCES_R), "DATALISTALLSOURCES_R");

                    dataR->list = data;
                    dataR->count = nrows;

                    for (i = 0; i < nrows; i++) {

                        stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                        data[i].source_alias = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].source_alias, PQgetvalue(DBQuery, i, 0));
                        addMalloc(logmalloclist, (void *) data[i].source_alias, 1, stringLength * sizeof(char), "char");

                        p = PQgetvalue(DBQuery, i, 1);
                        if (p[0] == 'A') {
                            strcpy(work, "Analysed");
                        } else if (p[0] == 'R') {
                            strcpy(work, "Raw");
                        } else if (p[0] == 'I') {
                            strcpy(work, "Image");
                        } else if (p[0] == 'M') {
                            strcpy(work, "Modelled");
                        } else
                            strcpy(work, p);

                        stringLength = strlen(work) + 1;
                        data[i].type = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data[i].type, work);
                        addMalloc(logmalloclist, (void *) data[i].type, 1, stringLength * sizeof(char), "char");

                    }
                } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                    UDA_LOG(UDA_LOG_DEBUG, "Cast as column\n");

                    DATALISTALLSOURCES_C *data;
                    data = (DATALISTALLSOURCES_C *) malloc(sizeof(DATALISTALLSOURCES_C));
                    addMalloc(logmalloclist, (void *) data, 1, sizeof(DATALISTALLSOURCES_C), "DATALISTALLSOURCES_C");
                    structData = (void *) data;

                    data->source_alias = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->source_alias, nrows, sizeof(char *), "STRING *");

                    data->type = (char **) malloc(nrows * sizeof(char *));
                    addMalloc(logmalloclist, (void *) data->type, nrows, sizeof(char *), "STRING *");

                    data->count = nrows;

                    for (i = 0; i < nrows; i++) {

                        stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                        data->source_alias[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->source_alias[i], PQgetvalue(DBQuery, i, 0));
                        addMalloc(logmalloclist, (void *) data->source_alias[i], stringLength, sizeof(char), "char");

                        p = PQgetvalue(DBQuery, i, 1);
                        if (p[0] == 'A') {
                            strcpy(work, "Analysed");
                        } else if (p[0] == 'R') {
                            strcpy(work, "Raw");
                        } else if (p[0] == 'I') {
                            strcpy(work, "Image");
                        } else if (p[0] == 'M') {
                            strcpy(work, "Modelled");
                        } else {
                            strcpy(work, "");
                        }

                        stringLength = strlen(work) + 1;
                        data->type[i] = (char *) malloc(stringLength * sizeof(char));
                        strcpy(data->type[i], work);
                        addMalloc(logmalloclist, (void *) data->type[i], stringLength, sizeof(char), "char");
                    }
                }

                PQclear(DBQuery);

                // Pass Data

                data_block->data_type = UDA_TYPE_COMPOUND;
                data_block->rank = 0;
                data_block->data_n = 1;
                data_block->data = (char *) structData;

                strcpy(data_block->data_desc, "LISTALLSOURCES");
                strcpy(data_block->data_label, "");
                strcpy(data_block->data_units, "");

                data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
                data_block->opaque_count = 1;
                if (castTypeId == CASTROW) {
                    data_block->opaque_block = (void *) findUserDefinedType(userdefinedtypelist, "DATALISTALLSOURCES_RR",
                                                                            0);
                } else if (castTypeId == CASTCOLUMN) {
                    data_block->opaque_block = (void *) findUserDefinedType(userdefinedtypelist, "DATALISTALLSOURCES_C",
                                                                            0);
                }

                UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function LISTALLSOURCES called\n");

                break;
            }

        } else

            //--------------------------------------------------------------------------------------------------------------------------
            // CPF Context
            //----------------------------------------------------------------------------------------
            // List all Classes of CPF Data
            // e.g. meta::listData(context=cpf, /listclasses [,cast={row|column}])

        if (context == CONTEXT_CPF && isListClasses &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            strcpy(sql, "SELECT DISTINCT data_class FROM dictionary ORDER BY data_class ASC");

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: listClasses SQL\n%s\n", sql);


// Execute the SQL
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta listClasses: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "CPFLIST_R");            // Default is Row Oriented
                usertype.size = sizeof(CPFLIST_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "CPFLIST_C");
                usertype.size = sizeof(CPFLIST_C);            // Selected return structure size
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "class", "metadata class", &offset,
                        stringTypeId);    // Array or Single string, arbitrary length
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

            if (castTypeId == CASTROW) {
                initUserDefinedType(&usertype);
                strcpy(usertype.name, "CPFLIST_RR");
                usertype.size = sizeof(CPFLIST_RR);
                strcpy(usertype.source, "listData");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = UDA_TYPE_COMPOUND;
                offset = 0;
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);

                initCompoundField(&field);
                strcpy(field.name, "list");
                field.atomictype = UDA_TYPE_UNKNOWN;
                strcpy(field.type, "CPFLIST_R");
                strcpy(field.desc, "[CPFLIST_R *list] CPF Class list");
                field.pointer = 1;
                field.count = 1;
                field.rank = 0;
                field.shape = NULL;
                field.size = field.count * sizeof(CPFLIST_R*);
                field.offset = newoffset(offset, field.type);
                field.offpad = padding(offset, field.type);
                field.alignment = getalignmentof(field.type);
                offset = field.offset + field.size;    // Next Offset
                addCompoundField(&usertype, field);        // Single Structure element

                addUserDefinedType(userdefinedtypelist, usertype);
            }


// Create Data

            if (castTypeId == CASTROW) {                // Row oriented

                CPFLIST_RR* dataR;
                dataR = (CPFLIST_RR*)malloc(sizeof(CPFLIST_RR));    // Structured Data Must be a heap variable
                addMalloc(logmalloclist, (void*)dataR, 1, sizeof(CPFLIST_RR), "CPFLIST_RR");
                structData = (void*)dataR;

                CPFLIST_R* data;
                data = (CPFLIST_R*)malloc(nrows * sizeof(CPFLIST_R));
                addMalloc(logmalloclist, (void*)data, nrows, sizeof(CPFLIST_R), "CPFLIST_R");

                dataR->list = data;
                dataR->count = nrows;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data[i].class = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].class, PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data[i].class, 1, stringLength * sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "listClasses: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data[i].class);
                }

            } else if (castTypeId == CASTCOLUMN) {
                // Column oriented
                CPFLIST_C* data;
                data = (CPFLIST_C*)malloc(sizeof(CPFLIST_C));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(CPFLIST_C), "CPFLIST_C");
                structData = (void*)data;

                data->class = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->class, nrows, sizeof(char*), "STRING *");

                data->count = nrows;

                for (i = 0; i < nrows; i++) {
                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data->class[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->class[i], PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data->class[i], stringLength, sizeof(char), "STRING");

                    UDA_LOG(UDA_LOG_DEBUG, "listClasses: [%d]\n", i);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data->class[i]);
                }
            }

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listClasses");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFLIST_RR", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFLIST_C", 0);
            }
            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listClasses called with CPF context\n");

            break;

        } else

//--------------------------------------------------------------------------------------------------------------------------
// List CPF variable Names
// e.g. meta::listData(context=cpf [,cast={row|column}][,class=class][,description=description]
//                     [,source=source][,table=table])

        if (context == CONTEXT_CPF && !isListClasses &&
            (STR_IEQUALS(request_block->function, "listdata") || STR_IEQUALS(request_block->function, "list"))) {

            UDA_LOG(UDA_LOG_DEBUG, "Meta: listData function called with CPF context\n");

            // Desc match string
            char* desc_match_escaped = NULL;
            if (descDependent >= 0) {
                desc_match_escaped = (char*)malloc((2 * strlen(description) + 1) * sizeof(char));
                int err_inj = 0;
                PQescapeStringConn(DBConnect, desc_match_escaped, description, strlen(description), &err_inj);
            }

            work[0] = '\0';
            wCount = 0;
            if (classDependent >= 0) {
                if (wCount == 0)
                    sprintf(work, " data_class = '%s'", request_block->nameValueList.nameValue[classDependent].value);
                else
                    sprintf(work, " AND data_class = '%s'",
                            request_block->nameValueList.nameValue[classDependent].value);
                wCount++;
            }
            if (descDependent >= 0) {
                if (wCount == 0)
                    sprintf(work, " description ILIKE '%c%s%c'", '%',
                            desc_match_escaped, '%');
                else
                    sprintf(work, " AND description ILIKE '%c%s%c'", '%',
                            desc_match_escaped, '%');
                wCount++;
            }
            if (sourceDependent >= 0) {
                if (wCount == 0)
                    sprintf(work, " source ILIKE '%c%s%c'", '%',
                            request_block->nameValueList.nameValue[sourceDependent].value, '%');
                else
                    sprintf(work, " AND source ILIKE '%c%s%c'", '%',
                            request_block->nameValueList.nameValue[sourceDependent].value, '%');
                wCount++;
            }
            if (tableDependent >= 0) {
                if (wCount == 0)
                    sprintf(work, " cpf_table = '%s'", request_block->nameValueList.nameValue[tableDependent].value);
                else
                    sprintf(work, " AND cpf_table = '%s'",
                            request_block->nameValueList.nameValue[tableDependent].value);
                wCount++;
            }

            if (wCount > 0)
                sprintf(sql, "SELECT name, cpf_table, data_class, source, description, label, units FROM dictionary "
                        "WHERE %s ORDER by data_class, name;", work);
            else
                strcpy(sql, "SELECT name, cpf_table, data_class, source, description, label, units FROM dictionary "
                        "ORDER by data_class, name;");

            UDA_LOG(UDA_LOG_DEBUG, "Meta sql: %s\n", sql);

// Execute the SQL

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta:: Database Query Failed!\n");
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta:: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);

            UDA_LOG(UDA_LOG_DEBUG, "Meta sql: Rows = %d\n", nrows);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }



// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "CPFLISTNAMES_R");        // Default is Row Oriented
                usertype.size = sizeof(CPFLISTNAMES_R);
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "CPFLISTNAMES_C");
                usertype.size = sizeof(CPFLISTNAMES_C);
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            defineField(&field, "name", "table column name", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "table", "database table name", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "class", "data classification", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "source", "data source", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "description", "description of the data", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "label", "data label", &offset, stringTypeId);
            addCompoundField(&usertype, field);
            defineField(&field, "units", "data units", &offset, stringTypeId);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

            if (castTypeId == CASTROW) {
                initUserDefinedType(&usertype);
                strcpy(usertype.name, "CPFLISTNAMES_RR");
                usertype.size = sizeof(CPFLISTNAMES_RR);
                strcpy(usertype.source, "listData");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = UDA_TYPE_COMPOUND;

                offset = 0;
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);

                initCompoundField(&field);
                strcpy(field.name, "list");
                field.atomictype = UDA_TYPE_UNKNOWN;
                strcpy(field.type, "CPFLISTNAMES_R");
                strcpy(field.desc, "[CPFLISTNAMES_R *list] Metadata records");
                field.pointer = 1;
                field.count = 1;
                field.rank = 0;
                field.shape = NULL;
                field.size = field.count * sizeof(CPFLISTNAMES_R*);
                field.offset = newoffset(offset, field.type);
                field.offpad = padding(offset, field.type);
                field.alignment = getalignmentof(field.type);
                offset = field.offset + field.size;
                addCompoundField(&usertype, field);

                addUserDefinedType(userdefinedtypelist, usertype);
            }

// Create Data

            UDA_LOG(UDA_LOG_DEBUG, "listData:\n");

            if (castTypeId == CASTROW) {                // Row oriented

                CPFLISTNAMES_RR* dataR;
                dataR = (CPFLISTNAMES_RR*)malloc(sizeof(CPFLISTNAMES_RR));
                addMalloc(logmalloclist, (void*)dataR, 1, sizeof(CPFLISTNAMES_RR), "CPFLISTNAMES_RR");
                structData = (void*)dataR;

                CPFLISTNAMES_R* data;
                data = (CPFLISTNAMES_R*)malloc(nrows * sizeof(CPFLISTNAMES_R));
                addMalloc(logmalloclist, (void*)data, nrows, sizeof(CPFLISTNAMES_R), "CPFLISTNAMES_R");

                dataR->list = data;
                dataR->count = nrows;

                for (i = 0; i < nrows; i++) {

                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data[i].name = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].name, PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data[i].name, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data[i].table = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].table, PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data[i].table, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data[i].class = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].class, PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data[i].class, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                    data[i].source = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].source, PQgetvalue(DBQuery, i, 3));
                    addMalloc(logmalloclist, (void*)data[i].source, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data[i].description = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].description, PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data[i].description, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 5)) + 1;
                    data[i].label = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].label, PQgetvalue(DBQuery, i, 5));
                    addMalloc(logmalloclist, (void*)data[i].label, 1, stringLength * sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 6)) + 1;
                    data[i].units = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data[i].units, PQgetvalue(DBQuery, i, 6));
                    addMalloc(logmalloclist, (void*)data[i].units, 1, stringLength * sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "name       : %s\n", data[i].name);
                    UDA_LOG(UDA_LOG_DEBUG, "table      : %s\n", data[i].table);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data[i].class);
                    UDA_LOG(UDA_LOG_DEBUG, "source     : %s\n", data[i].source);
                    UDA_LOG(UDA_LOG_DEBUG, "description: %s\n", data[i].description);
                    UDA_LOG(UDA_LOG_DEBUG, "label      : %s\n", data[i].label);
                    UDA_LOG(UDA_LOG_DEBUG, "units      : %s\n", data[i].units);
                }
            } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                CPFLISTNAMES_C* data;
                data = (CPFLISTNAMES_C*)malloc(sizeof(CPFLISTNAMES_C));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(CPFLISTNAMES_C), "CPFLISTNAMES_C");
                structData = (void*)data;

                data->name = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->name, nrows, sizeof(char*), "STRING *");
                data->table = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->table, nrows, sizeof(char*), "STRING *");
                data->class = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->class, nrows, sizeof(char*), "STRING *");
                data->source = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->source, nrows, sizeof(char*), "STRING *");
                data->description = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->description, nrows, sizeof(char*), "STRING *");
                data->label = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->label, nrows, sizeof(char*), "STRING *");
                data->units = (char**)malloc(nrows * sizeof(char*));
                addMalloc(logmalloclist, (void*)data->units, nrows, sizeof(char*), "STRING *");

                data->count = nrows;

                for (i = 0; i < nrows; i++) {

                    stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
                    data->name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->name[i], PQgetvalue(DBQuery, i, 0));
                    addMalloc(logmalloclist, (void*)data->name[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
                    data->table[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->table[i], PQgetvalue(DBQuery, i, 1));
                    addMalloc(logmalloclist, (void*)data->table[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
                    data->class[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->class[i], PQgetvalue(DBQuery, i, 2));
                    addMalloc(logmalloclist, (void*)data->class[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
                    data->source[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->source[i], PQgetvalue(DBQuery, i, 3));
                    addMalloc(logmalloclist, (void*)data->source[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
                    data->description[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->description[i], PQgetvalue(DBQuery, i, 4));
                    addMalloc(logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 5)) + 1;
                    data->label[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->label[i], PQgetvalue(DBQuery, i, 5));
                    addMalloc(logmalloclist, (void*)data->label[i], stringLength, sizeof(char), "char");

                    stringLength = strlen(PQgetvalue(DBQuery, i, 6)) + 1;
                    data->units[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->units[i], PQgetvalue(DBQuery, i, 6));
                    addMalloc(logmalloclist, (void*)data->units[i], stringLength, sizeof(char), "char");

                    UDA_LOG(UDA_LOG_DEBUG, "name       : %s\n", data->name[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "table      : %s\n", data->table[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "class      : %s\n", data->class[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "source     : %s\n", data->source[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "description: %s\n", data->description[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "label      : %s\n", data->label[i]);
                    UDA_LOG(UDA_LOG_DEBUG, "units      : %s\n", data->units[i]);
                }
            }

            PQclear(DBQuery);
            free(desc_match_escaped);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "listData");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFLISTNAMES_RR", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFLISTNAMES_C", 0);
            }

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Return from function listData\n");

            break;

        } else if (context == CONTEXT_CPF &&
                   (STR_IEQUALS(request_block->function, "getdata") || STR_IEQUALS(request_block->function, "get"))) {

            //--------------------------------------------------------------------------------------------------------------------------
            // Get CPF Data
            // e.g. meta::getData(context=cpf [,cast={row|column}][,names=names][,table=table][,where=where][,limit=limit])

            UDA_LOG(UDA_LOG_DEBUG, "Meta: getData function called with CPF context\n");

            if (nameDependent < 0) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err,
                             "No CPF data item names have been specified!");
                break;
            }

            if (tableDependent < 0) {
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No CPF data table has been specified!");
                break;
            }

// Test the names list uses comma as the delimiter: allowed characters are a-z,A-Z,0-9,_

            err = 0;
            char* tst = request_block->nameValueList.nameValue[nameDependent].value;
            while (*tst != '\0') {
                if ((*tst >= '0' && *tst <= '9') || (*tst >= 'A' && *tst <= 'Z') || (*tst >= 'a' && *tst <= 'z') ||
                    *tst == '_' || *tst == ',') {
                    tst++;
                    continue;
                }
                err = 999;    // Error - SQL Name list is not compliant!
                break;
            }
            if (err != 0) {
                addIdamError(CODEERRORTYPE, "readMeta", err, "Syntax error in CPF Names list!");
                break;
            }

// Test the Table name is OK

            err = 0;
            tst = request_block->nameValueList.nameValue[tableDependent].value;
            while (*tst != '\0') {
                if ((*tst >= '0' && *tst <= '9') || (*tst >= 'A' && *tst <= 'Z') || (*tst >= 'a' && *tst <= 'z') ||
                    *tst == '_') {
                    tst++;
                    continue;
                }
                err = 999;    // Error - SQL Table Name is not compliant!
                break;
            }
            if (err != 0) {
                addIdamError(CODEERRORTYPE, "readMeta", err, "Syntax error in CPF table name!");
                break;
            }

            sprintf(sql, "SELECT %s FROM %s ", request_block->nameValueList.nameValue[nameDependent].value,
                    request_block->nameValueList.nameValue[tableDependent].value);

            if (whereDependent >= 0) {
                strcat(sql, "WHERE ");
                strcat(sql, request_block->nameValueList.nameValue[whereDependent].value);
            }
            if (limitDependent >= 0) {
                strcat(sql, " LIMIT ");
                strcat(sql, request_block->nameValueList.nameValue[limitDependent].value);
            }

            UDA_LOG(UDA_LOG_DEBUG, "Meta sql: %s\n", sql);

// Execute the SQL

            UDA_LOG(UDA_LOG_DEBUG, "Query (cpf) : %s\n", sql);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                err = 999;
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta:: Database Query Failed!\n");
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta:: Database Query Failed!\n");
                addIdamError(CODEERRORTYPE, "readMeta", err, "Database Query Failed!");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta: %s\n", PQresultErrorMessage(DBQuery));
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, PQresultErrorMessage(DBQuery));
                break;
            }

            nrows = PQntuples(DBQuery);        // Number of Rows
            ncols = PQnfields(DBQuery);        // Number of Columns

            UDA_LOG(UDA_LOG_DEBUG, "Meta sql: Rows = %d\n", nrows);

            if (nrows == 0) {
                UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: No Meta Data available!\n");
                UDA_LOG(UDA_LOG_DEBUG, "ERROR Meta: No Meta Data available!\n");
                err = 999;
                addIdamError(CODEERRORTYPE, "readMeta", err, "No Meta Data available!");
                break;
            }

// Create the Returned Structure Definition

            initUserDefinedType(&usertype);            // New structure definition

            if (castTypeId == CASTROW) {
                strcpy(usertype.name, "CPFDATA_R");            // Default is Row Oriented
                usertype.size = ncols * sizeof(char*);
                UDA_LOG(UDA_LOG_DEBUG, "Structure is Row aligned\n");
            } else if (castTypeId == CASTCOLUMN) {
                strcpy(usertype.name, "CPFDATA_C");
                usertype.size = sizeof(unsigned int) + ncols * sizeof(char*);
                UDA_LOG(UDA_LOG_DEBUG, "Structure is Column aligned\n");
            }

            strcpy(usertype.source, "readMeta");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = UDA_TYPE_COMPOUND;

            offset = 0;

            if (castTypeId == CASTCOLUMN) {
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);
            }

            for (i = 0; i < ncols; i++) {
                defineField(&field, PQfname(DBQuery, i), "field name", &offset, stringTypeId);
                addCompoundField(&usertype, field);
            }

            addUserDefinedType(userdefinedtypelist, usertype);

            if (castTypeId == CASTROW) {
                initUserDefinedType(&usertype);
                strcpy(usertype.name, "CPFDATA_RR");
                usertype.size = sizeof(CPFDATA_RR);
                strcpy(usertype.source, "getData");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = UDA_TYPE_COMPOUND;

                offset = 0;
                defineField(&field, "count", "Array element count", &offset, SCALARUINT);
                addCompoundField(&usertype, field);

                initCompoundField(&field);
                strcpy(field.name, "list");
                field.atomictype = UDA_TYPE_UNKNOWN;
                strcpy(field.type, "CPFDATA_R");
                strcpy(field.desc, "[CPFDATA_R *list] Metadata records");
                field.pointer = 1;
                field.count = 1;
                field.rank = 0;
                field.shape = NULL;
                field.size = field.count * sizeof(void*);
                field.offset = newoffset(offset, field.type);
                field.offpad = padding(offset, field.type);
                field.alignment = getalignmentof(field.type);
                offset = field.offset + field.size;
                addCompoundField(&usertype, field);

                addUserDefinedType(userdefinedtypelist, usertype);
            }

// Create Data

            UDA_LOG(UDA_LOG_DEBUG, "getData:\n");

            if (castTypeId == CASTROW) {                // Row oriented

                CPFDATA_RR* dataR;
                dataR = (CPFDATA_RR*)malloc(sizeof(CPFDATA_RR));
                addMalloc(logmalloclist, (void*)dataR, 1, sizeof(CPFDATA_RR), "CPFDATA_RR");
                structData = (void*)dataR;

                char** data;
                data = (char**)malloc(nrows * ncols * sizeof(char*));
                addMalloc(logmalloclist, (void*)data, nrows, ncols * sizeof(char*), "CPFDATA_R");

                dataR->list = (void*)data;
                dataR->count = nrows;

                offset = 0;
                for (i = 0; i < nrows; i++) {
                    for (j = 0; j < ncols; j++) {
                        stringLength = strlen(PQgetvalue(DBQuery, i, j)) + 1;
                        s = (char*)malloc(stringLength * sizeof(char));
                        strcpy(s, PQgetvalue(DBQuery, i, j));
                        addMalloc(logmalloclist, (void*)s, 1, stringLength * sizeof(char), "char");
                        data[offset++] = s;

                        UDA_LOG(UDA_LOG_DEBUG, "name       : %s\n", PQfname(DBQuery, j));
                        UDA_LOG(UDA_LOG_DEBUG, "value      : %s\n", data[offset - 1]);
                    }
                }
            } else if (castTypeId == CASTCOLUMN) {                // Column oriented

                void* data;
                data = (void*)malloc(sizeof(unsigned int) + ncols * sizeof(char*));
                addMalloc(logmalloclist, (void*)data, 1, sizeof(unsigned int) + ncols * sizeof(char*), "CPFDATA_C");
                structData = (void*)data;

                char*** sData = (char***)malloc(ncols * sizeof(char**));    // sData[ncols][nrows][]
                for (j = 0; j < ncols; j++) {
                    sData[j] = (char**)malloc(nrows * sizeof(char*));
                    addMalloc(logmalloclist, (void*)sData[j], nrows, sizeof(char*), "STRING *");
                }

                unsigned int* r = (unsigned int*)data;
                char** p = (char**)(data + sizeof(unsigned int));

                *r = nrows;

                offset = 0;
                for (j = 0; j < ncols; j++) {
                    UDA_LOG(UDA_LOG_DEBUG, "name       : %s\n", PQfname(DBQuery, j));
                    for (i = 0; i < nrows; i++) {
                        stringLength = strlen(PQgetvalue(DBQuery, i, j)) + 1;
                        sData[j][i] = (char*)malloc(stringLength * sizeof(char));
                        strcpy(sData[j][i], PQgetvalue(DBQuery, i, j));
                        addMalloc(logmalloclist, (void*)sData[j][i], stringLength, sizeof(char), "char");
                        UDA_LOG(UDA_LOG_DEBUG, "value      : %s\n", sData[j][i]);
                    }
                    p[offset++] = (char*)sData[j];
                }
            }

            PQclear(DBQuery);

// Pass Data

            data_block->data_type = UDA_TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*)structData;

            strcpy(data_block->data_desc, "getData");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            if (castTypeId == CASTROW) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFDATA_RR", 0);
            } else if (castTypeId == CASTCOLUMN) {
                data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "CPFDATA_C", 0);
            }

            UDA_LOG(UDA_LOG_DEBUG, "readMeta: Return from function getData\n");

            break;

        } else {

            //----------------------------------------------------------------------------------------
            // Not a Known Function!

            UDA_LOG(UDA_LOG_ERROR, "ERROR Meta: Function %s Not Known.!\n", request_block->function);
            err = 999;
            addIdamError(CODEERRORTYPE, "readMeta", err, "Unknown Function requested");
            addIdamError(CODEERRORTYPE, "readMeta", err, request_block->function);
            break;
        }

    } while (0);

    return err;
}
