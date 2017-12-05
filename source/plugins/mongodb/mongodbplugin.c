/*---------------------------------------------------------------
* v1 IDAM Plugin: Query the IDAM MongoDB Metadata Catalog
*                 Designed for use case where each signal class is recorded but not each signal instance
*                 Only the first document identified that satisfies the selection criteria is returned.
*                 Documents within the database must be unique 
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
*
* Public Functions:
*
*       query	return the database metadata record for a data object
*
* Private Functions:
*
*       get	return the name mapping between an alias or generic name for a data object and its true name or
*               method of data access.
*      
*       connection   return the private database connection object for other plugins to reuse 
* 
* Standard functionality:
*
*	help	a description of what this plugin does together with a list of functions available
*
*	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
*		This has the same functionality as setting the housekeeping directive in the plugin interface
*		data structure to TRUE (1)
*
*	init	Initialise the plugin: read all required data and process. Retain staticly for
*		future reference.
*---------------------------------------------------------------------------------------------------------------*/
#include "mongodbplugin.h"

#include <bson.h>
#include <mongoc.h>

#include <plugins/udaPlugin.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/initStructs.h>

typedef struct MongoR {
    char* objectName;
    char* device;
    char* objectSource;
    char* source;
    char* signal_name;
    char* type;
    char* signal_alias;
    char* generic_name;
    char* source_alias;
    char* signal_class;
    char* description;
    int expNumber;
    int range_start;
    int range_stop;
} MONGO_R;

#define MAXURILENGTH 1024

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_query(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, void* conn);

extern int query(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static short init = 0;

    //----------------------------------------------------------------------------------------
    // Database Objects

    static mongoc_client_t* client;
    static mongoc_database_t* database;
    static mongoc_collection_t* collection;

    static short sqlPrivate = 0;                            // If the Database connection was opened here, it remains local and open but is not passed back.
    static unsigned short db_type = PLUGINSQLNOTKNOWN;      // The database type
    static void* db_conn = NULL;                            // The database connection object

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    housekeeping = idam_plugin_interface->housekeeping;

    // Database connection passed in from the server (external)

    if (!sqlPrivate) {                        // Use External database connection
        db_type = idam_plugin_interface->sqlConnectionType;
        if (db_type == PLUGINSQLMONGODB) {
            db_conn = idam_plugin_interface->sqlConnection;
            client = (mongoc_client_t*)db_conn;
        }
    }

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Free Heap & reset counters

        if (sqlPrivate && db_conn != NULL && db_type == PLUGINSQLMONGODB) {
            mongoc_collection_destroy(collection);    // Release handles and clean up libmongoc
            mongoc_database_destroy(database);
            mongoc_client_destroy(client);
            mongoc_cleanup();
            sqlPrivate = 0;
            db_conn = NULL;
            client = NULL;
            db_type = PLUGINSQLNOTKNOWN;
        }

        init = 0;

        return 0;
    }

    if (!STR_IEQUALS(request_block->function, "help") && (!init || STR_IEQUALS(request_block->function, "init")
                                                          || STR_IEQUALS(request_block->function, "initialise"))) {

        ENVIRONMENT* environment = idam_plugin_interface->environment;

        // Is there an Open Database Connection? If not then open a private (within scope of this plugin only) connection

        if (db_conn == NULL && (db_type == PLUGINSQLMONGODB || db_type == PLUGINSQLNOTKNOWN)) {

            // Create the connection
            /*
            mongodb://                                   <1>
               [username:password@]                      <2>
               host1                                     <3>
               [:port1]                                  <4>
               [,host2[:port2],...[,hostN[:portN]]]      <5>
               [/[database]                              <6> Authentication
               [?options]]

            User Roles:
            read		read all non system collections		<----- use this built-in role for all database queries
            readwrite
            */

            static char uri[MAXURILENGTH + 1];

            char* env;
            if (environment->sql_user[0] == '\0' && (env = getenv("UDA_SQLUSER")) != NULL) {
                strcpy(environment->sql_user, env);
            }
            if (environment->sql_host[0] == '\0' && (env = getenv("UDA_SQLHOST")) != NULL) {
                strcpy(environment->sql_host, env);
            }
            if (environment->sql_dbname[0] == '\0' && (env = getenv("UDA_SQLDBNAME")) != NULL) {
                strcpy(environment->sql_dbname, env);
            }
            if (environment->sql_port == 0 && (env = getenv("UDA_SQLPORT")) != NULL) environment->sql_port = atoi(env);
            char* password = getenv("UDA_SQLPASSWORD");

            if (environment->sql_user[0] != '\0' && environment->sql_host[0] != '\0' &&
                environment->sql_dbname[0] != '\0' && environment->sql_port > 0 && password != NULL) {
                sprintf(uri, "mongodb://%s:%s@%s:%d/%s", environment->sql_user, password, environment->sql_host,
                        environment->sql_port, environment->sql_dbname);
            } else {
                RAISE_PLUGIN_ERROR("Insufficient Connection and Authentication details!");
            }

            mongoc_init();            // Initialize libmongoc's internals

            client = mongoc_client_new(
                    uri);    // Create a new client instance (Not thread safe) authenticating with the IDAM database

            db_conn = (void*)client;        // No prior connection to IDAM Database
            if (db_conn != NULL) {
                db_type = PLUGINSQLMONGODB;
                sqlPrivate = 1;            // the connection belongs to this plugin
                UDA_LOG(UDA_LOG_DEBUG, "mongodbplugin: Private regular database connection made.\n");
            }
        }

        if (!client) {
            RAISE_PLUGIN_ERROR("No connection to Database server made!");
        }

        database = mongoc_client_get_database(client, environment->sql_dbname);

        if (!database) {
            RAISE_PLUGIN_ERROR("No connection to Database cluster made!");
        }

        char* env;
        if ((env = getenv("UDA_SQLTABLE")) != NULL) {
            collection = mongoc_client_get_collection(client, environment->sql_dbname, env);
        } else {
            collection = mongoc_client_get_collection(client, environment->sql_dbname,
                                                      "maps");
        }    // Get a handle on the database and collection

        if (!collection) {
            RAISE_PLUGIN_ERROR("No handle to Database collection made!");
        }

        init = 1;
    }

    //----------------------------------------------------------------------------------------
    // Return the private (local) connection Object
    // Not intended for client use ...

    if (STR_IEQUALS(request_block->function, "connection")) {

        if (sqlPrivate && db_conn != NULL) {
            idam_plugin_interface->sqlConnectionType = PLUGINSQLMONGODB;
            idam_plugin_interface->sqlConnection = (void*)client;
        } else {
            idam_plugin_interface->sqlConnectionType = PLUGINSQLNOTKNOWN;
            idam_plugin_interface->sqlConnection = NULL;
        }

        return 0;
    }

    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        return do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        return do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        return do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        return do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "query")
               || STR_IEQUALS(request_block->function, THISPLUGIN_DEFAULT_METHOD)) {
        return do_query(idam_plugin_interface, db_conn);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return 0;
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nMongoDBPlugin: Function Names, Syntax, and Descriptions\n\n"
            "Query the mongoDB IDAM database for specific instances of a data object by alias or generic name and shot number\n\n"
            "\tquery( [signal|objectName]=objectName, [shot|exp_number|pulse|pulno]=exp_number [,source|objectSource=objectSource] [,device=device] [,/allMeta])\n"
            "\t       [objectClass=objectClass] [,sourceClass=sourceClass] [,type=type])\n\n"
            "\tobjectName: The alias or generic name of the data object.\n"
            "\texp_number: The experiment shot number. This may be passed via the client API's second argument.\n"
            "\tobjectSource: the abstract name of the source. This may be passed via the client API's second argument, either alone or with exp_number [exp_number/objectSource]\n"
            "\tdevice: the name of the experiment device, e.g. ITER\n"
            "\tallMeta: a keyword to request the full database record be returned\n"
            "\tobjectClass: the name of the data's measurement class, e.g. magnetics\n"
            "\tsourceClass: the name of the data's source class, e.g. imas\n"
            "\ttype: the data type classsification, e.g. P for Plugin\n\n"

            "\tobjectName is a mandatory argument. One or both of exp_number and ObjectSource is also mandatory unless passed via the client API's second argument.\n\n"

            "\tExample\tidamGetAPI(\"mongodbplugin::query(signal=ip, shot=12345, device=ITER, /allmeta)\", \"\");\n"
            "\t\tidamGetAPI(\"mongodbplugin::query(signal=ip, device=ITER, /allmeta)\", \"12345\");\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

static int do_query(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, void* conn)
{

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    // Mandatory arguments

    const char* objectName;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, objectName);

    int expNumber;
    bool isExpNumber = FIND_INT_VALUE(request_block->nameValueList, expNumber);

    const char* objectSource;
    bool isObjectSource = FIND_STRING_VALUE(request_block->nameValueList, objectSource);

    if (!isExpNumber && !isObjectSource) {

        if (request_block->exp_number >
            0) {        // The expNumber has been specified via the client API's second argument
            isExpNumber = 1;
            expNumber = request_block->exp_number;
        }

        if (request_block->tpass[0] != '\0') {
            isObjectSource = 1;
            objectSource = request_block->tpass;    // The object's source has been specified via the client API's second argument
        }

        if (!isExpNumber && !isObjectSource) {
            RAISE_PLUGIN_ERROR("No Experiment Number or data source specified");
        }

    }

    // Query for a specific named data object valid for the shot number range

    // All classification and abstraction data should be recorded in single case
    // Upper case is chosen as the convention. This is mandatory for the object name
    // Lower case matches are also made so do not use Mixed Case for data in the database!

    // NOTE: Ensure all query elements are separated by commas when using BCON_APPEND !

    /*
    bson query;
    bson_init(&query);
    bson_append_start_object(&query, "signal_alias");
    bson_append_utf8(&query, "$eq", strupr(objectName));
    bson_append_finish_object(&query);
    bson_finish(&query);

    case insensitive search    {name:{'$regex' : '^string$', '$options': 'i'}}      Does not use the index so very slow!

    sprintf(temp, "^%s$", objectClass);
    BCON_APPEND (query, "$and", "[",
                                           "{", "signal_class",
                                                "{", "$regex", BCON_UTF8 (temp), "$options", "i", "}",
                                           "}",
                                      "]");

    use specific case for querying

    Best Query time (micro secs) [load]
    1> signal_alias only			83	[2.2 on 4 core host]
    2> signal_alias or generic name only	99	[2.2 on 4 core host]
    3> signal_alias and shot number only	161	[2.2 on 4 core host]
    4> and device				170	[2.2 on 4 core host]
    5> and objectClass			192	[1.1 on 4 core host]

    */

    char* name = strupr(strdup(objectName));
    bson_t* query = BCON_NEW("$or", "[",
                     "{", "signal_alias", BCON_UTF8(name), "}",
                     "{", "generic_name", BCON_UTF8(name), "}",
                     "]");
    free(name);

    if (isExpNumber) {
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "range_start", "{", "$eq", BCON_INT32(0), "}", "}",
                    "{", "range_start", "{", "$lte", BCON_INT32(expNumber), "}", "}",
                    "]",
                    "}",
                    "{", "$or", "[",
                    "{", "range_end", "{", "$eq", BCON_INT32(0), "}", "}",
                    "{", "range_end", "{", "$gte", BCON_INT32(expNumber), "}", "}",
                    "]",
                    "}",
                    "]");
    }

    const char* device;
    bool isDevice = FIND_STRING_VALUE(request_block->nameValueList, device);

    if (isDevice) {
        char* lower = strlwr(strdup(device));
        char* upper = strupr(strdup(device));
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "source_device", "{", "$eq", BCON_UTF8(lower), "}", "}",
                    "{", "source_device", "{", "$eq", BCON_UTF8(upper), "}", "}",
                    "]",
                    "}",
                    "]");
        free(lower);
        free(upper);
    }

    if (isObjectSource) {
        char* lower = strlwr(strdup(objectSource));
        char* upper = strupr(strdup(objectSource));
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "objectSource", "{", "$eq", BCON_UTF8(lower), "}", "}",
                    "{", "objectSource", "{", "$eq", BCON_UTF8(upper), "}", "}",
                    "]",
                    "}",
                    "]");
        free(lower);
        free(upper);
    }

    const char* type;
    bool isType = FIND_STRING_VALUE(request_block->nameValueList, type);

    if (isType) {
        char* lower = strlwr(strdup(type));
        char* upper = strupr(strdup(type));
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "type", "{", "$eq", BCON_UTF8(lower), "}", "}",
                    "{", "type", "{", "$eq", BCON_UTF8(upper), "}", "}",
                    "]",
                    "}",
                    "]");
        free(lower);
        free(upper);
    }

    const char* sourceClass;
    bool isSourceClass = FIND_STRING_VALUE(request_block->nameValueList, sourceClass);

    if (isSourceClass) {
        char* lower = strlwr(strdup(sourceClass));
        char* upper = strupr(strdup(sourceClass));
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "source_alias", "{", "$eq", BCON_UTF8(lower), "}", "}",
                    "{", "source_alias", "{", "$eq", BCON_UTF8(upper), "}", "}",
                    "]",
                    "}",
                    "]");
        free(lower);
        free(upper);
    }

    const char* objectClass;
    bool isObjectClass = FIND_STRING_VALUE(request_block->nameValueList, objectClass);

    if (isObjectClass) {
        char* lower = strlwr(strdup(objectClass));
        char* upper = strupr(strdup(objectClass));
        BCON_APPEND(query, "$and", "[",
                    "{", "$or", "[",
                    "{", "signal_class", BCON_UTF8(lower), "}",
                    "{", "signal_class", BCON_UTF8(upper), "}",
                    "]",
                    "}",
                    "]");
        free(lower);
        free(upper);
    }

    // Find the document

    uint32_t limit = 2;

    mongoc_cursor_t* cursor = NULL;
    mongoc_collection_t* collection = NULL;

#if MONGOC_CHECK_VERSION(1, 6, 0)
    bson_t* opts = BCON_NEW("limit", BCON_INT64(limit));

    if (!(cursor = mongoc_collection_find_with_opts(collection, query, opts, NULL))) {
        RAISE_PLUGIN_ERROR("Data Object not found!");
    }
#else
    if (!(cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, limit, 0, query, NULL, NULL))) {
        RAISE_PLUGIN_ERROR("Data Object not found!");
    }
#endif

    // Test for a returned BSON document

    // Iterate over the document for the fields of interest (not very fast!  Typically > 40ms! and same as mongoc_collection_count)
    // Returned Structures should be initialised prior to entry

    bool docAvail = mongoc_cursor_more(cursor);

    int docCount = 0;

    const bson_t* doc = NULL;

    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;

    initDataSource(data_source);
    initSignalDesc(signal_desc);

    bool allMeta = findValue(&request_block->nameValueList, "allMeta");

    while (docAvail && mongoc_cursor_next(cursor, &doc)) {        // Get the document (slow!)
        bson_iter_t iter;
        bson_iter_t value;

        if (docCount++ > 0) {
            RAISE_PLUGIN_ERROR("Too many data objects found!");
        }

        request_block->exp_number = expNumber;

        char* dbDevice = NULL;
        if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source_device", &value) &&
            BSON_ITER_HOLDS_UTF8(&value)) {
            uint32_t length = 0;
            const char* str = bson_iter_utf8(&value, &length);
            dbDevice = (char*)str;
            sprintf(data_source->path, "%s%s%d", str, request_block->api_delim, expNumber);
            strcpy(request_block->source, data_source->path);
        }
        if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "signal_name", &value) &&
            BSON_ITER_HOLDS_UTF8(&value)) {
            uint32_t length = 0;
            const char* str = bson_iter_utf8(&value, &length);
            strcpy(signal_desc->signal_name, str);
        }
        if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "type", &value) &&
            BSON_ITER_HOLDS_UTF8(&value)) {
            uint32_t length = 0;
            const char* str = bson_iter_utf8(&value, &length);
            signal_desc->type = str[0];
        }
        if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source", &value) &&
            BSON_ITER_HOLDS_UTF8(&value)) {
            uint32_t length = 0;
            const char* str = bson_iter_utf8(&value, &length);
            if (!isExpNumber) {
                if (dbDevice != NULL) {
                    sprintf(data_source->path, "%s%s%s", dbDevice, request_block->api_delim, str);
                } else {
                    sprintf(data_source->path, "%s", str);
                }
            } else {
                sprintf(data_source->path, "%s%s%d/%s", dbDevice, request_block->api_delim, expNumber, str);
            }
        }

        if (allMeta) {
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "generic_name", &value) &&
                BSON_ITER_HOLDS_UTF8(&value)) {
                uint32_t length = 0;
                const char* str = bson_iter_utf8(&value, &length);
                strcpy(signal_desc->generic_name, str);
            }
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source_alias", &value) &&
                BSON_ITER_HOLDS_UTF8(&value)) {
                uint32_t length = 0;
                const char* str = bson_iter_utf8(&value, &length);
                strcpy(signal_desc->source_alias, str);
            }
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "signal_class", &value) &&
                BSON_ITER_HOLDS_UTF8(&value)) {
                uint32_t length = 0;
                const char* str = bson_iter_utf8(&value, &length);
                strcpy(signal_desc->signal_class, str);
            }
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "description", &value) &&
                BSON_ITER_HOLDS_UTF8(&value)) {
                uint32_t length = 0;
                const char* str = bson_iter_utf8(&value, &length);
                strcpy(signal_desc->description, str);
            }
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "range_start", &value) &&
                BSON_ITER_HOLDS_INT32(&value)) {
                signal_desc->range_start = bson_iter_int32(&value);
            }
            if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "range_end", &value) &&
                BSON_ITER_HOLDS_INT32(&value)) {
                signal_desc->range_stop = bson_iter_int32(&value);
            }
        }
    }

    bson_destroy(query);
    mongoc_cursor_destroy(cursor);

    if (docCount == 0) {
        RAISE_PLUGIN_ERROR("No data object found!");
    }

    // Return Result: Data Block with the Meta Data structures

    if (STR_IEQUALS(request_block->function, "query")) {

        // Create the Returned Structure Definition

        USERDEFINEDTYPE usertype;
        initUserDefinedType(&usertype);            // New structure definition

        strcpy(usertype.name, "MONGO_R");
        usertype.size = sizeof(MONGO_R);

        strcpy(usertype.source, "mongoDBPlugin");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.idamclass = UDA_TYPE_COMPOUND;

        int offset = 0;

        COMPOUNDFIELD field;
        defineField(&field, "objectName", "The target data object's (abstracted) (signal) name", &offset,
                    SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "expNumber", "The target experiment number", &offset, SCALARINT);
        addCompoundField(&usertype, field);
        defineField(&field, "objectSource", "The target data object's (abstracted) source name", &offset,
                    SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "device", "The device name where the data source is located", &offset,
                    SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "source", "The true data source", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "signal_name", "the name of the target data object", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "type", "the type of data object", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "signal_alias", "the alias name of the target data object", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "generic_name", "the generic name of the target data object", &offset,
                    SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "source_alias", "the source class", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "signal_class", "the data class", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "description", "a description of the data object", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);
        defineField(&field, "range_start", "validity range starting value", &offset, SCALARINT);
        addCompoundField(&usertype, field);
        defineField(&field, "range_stop", "validity range ending value", &offset, SCALARINT);
        addCompoundField(&usertype, field);

        USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
        addUserDefinedType(userdefinedtypelist, usertype);

        // Create the data to be returned

        LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;

        MONGO_R* data = (MONGO_R*)malloc(sizeof(MONGO_R));
        addMalloc(logmalloclist, (void*)data, 1, sizeof(MONGO_R), "MONGO_R");

        size_t lstr = strlen(objectName) + 1;
        data->objectName = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->objectName, 1, lstr * sizeof(char), "char");
        strcpy(data->objectName, objectName);

        lstr = strlen(device) + 1;
        data->device = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->device, 1, lstr * sizeof(char), "char");
        strcpy(data->device, device);

        lstr = strlen(objectSource) + 1;
        data->objectSource = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->objectSource, 1, lstr * sizeof(char), "char");
        strcpy(data->objectSource, objectSource);

        lstr = strlen(data_source->path) + 1;
        data->source = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->source, 1, lstr * sizeof(char), "char");
        strcpy(data->source, data_source->path);

        lstr = strlen(signal_desc->signal_name) + 1;
        data->signal_name = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->signal_name, 1, lstr * sizeof(char), "char");
        strcpy(data->signal_name, signal_desc->signal_name);

        lstr = 2;
        data->type = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->type, 1, lstr * sizeof(char), "char");
        data->type[0] = signal_desc->type;
        data->type[1] = '\0';

        lstr = strlen(signal_desc->signal_alias) + 1;
        data->signal_alias = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->signal_alias, 1, lstr * sizeof(char), "char");
        strcpy(data->signal_alias, signal_desc->signal_alias);

        lstr = strlen(signal_desc->generic_name) + 1;
        data->generic_name = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->generic_name, 1, lstr * sizeof(char), "char");
        strcpy(data->generic_name, signal_desc->generic_name);

        lstr = strlen(signal_desc->source_alias) + 1;
        data->source_alias = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->source_alias, 1, lstr * sizeof(char), "char");
        strcpy(data->source_alias, signal_desc->source_alias);

        lstr = strlen(signal_desc->signal_class) + 1;
        data->signal_class = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->signal_class, 1, lstr * sizeof(char), "char");
        strcpy(data->signal_class, signal_desc->signal_class);

        lstr = strlen(signal_desc->description) + 1;
        data->description = (char*)malloc(lstr * sizeof(char));
        addMalloc(logmalloclist, (void*)data->description, 1, lstr * sizeof(char), "char");
        strcpy(data->description, signal_desc->description);

        data->expNumber = expNumber;
        data->range_start = signal_desc->range_start;
        data->range_stop = signal_desc->range_stop;

        // Return the Data

        DATA_BLOCK* data_block = idam_plugin_interface->data_block;
        initDataBlock(data_block);

        data_block->data_type = UDA_TYPE_COMPOUND;
        data_block->rank = 0;
        data_block->data_n = 1;
        data_block->data = (char*)data;

        strcpy(data_block->data_desc, "mongoDB query result");
        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");

        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "MONGO_R", 0);

    }

    return 0;
}
