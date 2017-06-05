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

#include <server/pluginStructs.h>
#include <plugins/udaPlugin.h>
#include <structures/genStructs.h>
#include <server/udaServer.h>
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

extern int query(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0, offset;

    static short init = 0;

//----------------------------------------------------------------------------------------
// Database Objects

    static mongoc_client_t* client;
    static mongoc_database_t* database;
    static mongoc_collection_t* collection;
    mongoc_cursor_t* cursor;

    bson_t* query;
    const bson_t* doc;

    static short sqlPrivate = 0;                // If the Database connection was opened here, it remains local and open but is not passed back.
    static unsigned short DBType = PLUGINSQLNOTKNOWN;    // The database type
    static void* DBConnect = NULL;            // The database connection object

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;
    DATA_SOURCE* data_source;
    SIGNAL_DESC* signal_desc;
    ENVIRONMENT* environment;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;
    data_source = idam_plugin_interface->data_source;
    signal_desc = idam_plugin_interface->signal_desc;
    environment = idam_plugin_interface->environment;

    housekeeping = idam_plugin_interface->housekeeping;

// Database connection passed in from the server (external)

    if (!sqlPrivate) {                        // Use External database connection
        DBType = idam_plugin_interface->sqlConnectionType;
        if (DBType == PLUGINSQLMONGODB) {
            DBConnect = idam_plugin_interface->sqlConnection;
            client = (mongoc_client_t*)DBConnect;
        }
    }

// Additional interface components (must be defined at the bottom of the standard data structure)
// Versioning must be consistent with the macro THISPLUGIN_MAX_INTERFACE_VERSION and the plugin registration with the server

    //if(idam_plugin_interface->interfaceVersion >= 2){
    // NEW COMPONENTS
    //}

//----------------------------------------------------------------------------------------
// Heap Housekeeping 

// Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
// Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
// Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
// Plugin must destroy lists at end of housekeeping

// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        if (sqlPrivate && DBConnect != NULL && DBType == PLUGINSQLMONGODB) {
            mongoc_collection_destroy(collection);    // Release handles and clean up libmongoc
            mongoc_database_destroy(database);
            mongoc_client_destroy(client);
            mongoc_cleanup();
            sqlPrivate = 0;
            DBConnect = NULL;
            client = NULL;
            DBType = PLUGINSQLNOTKNOWN;
        }

        init = 0;

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!STR_IEQUALS(request_block->function, "help") && (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise"))) {

        char* env = NULL;

// Is there an Open Database Connection? If not then open a private (within scope of this plugin only) connection

        if (DBConnect == NULL && (DBType == PLUGINSQLMONGODB || DBType == PLUGINSQLNOTKNOWN)) {

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

            if (environment->sql_user[0] == '\0' && (env = getenv("UDA_SQLUSER")) != NULL)
                strcpy(environment->sql_user, env);
            if (environment->sql_host[0] == '\0' && (env = getenv("UDA_SQLHOST")) != NULL)
                strcpy(environment->sql_host, env);
            if (environment->sql_dbname[0] == '\0' && (env = getenv("UDA_SQLDBNAME")) != NULL)
                strcpy(environment->sql_dbname, env);
            if (environment->sql_port == 0 && (env = getenv("UDA_SQLPORT")) != NULL) environment->sql_port = atoi(env);
            char* password = getenv("UDA_SQLPASSWORD");

            if (environment->sql_user[0] != '\0' && environment->sql_host[0] != '\0' &&
                environment->sql_dbname[0] != '\0' && environment->sql_port > 0 && password != NULL)
                sprintf(uri, "mongodb://%s:%s@%s:%d/%s", environment->sql_user, password, environment->sql_host,
                        environment->sql_port, environment->sql_dbname);
            else {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                             "Insufficient Connection and Authentication details!");
                IDAM_LOG(LOG_ERROR, "MongoDBPlugin: Insufficient Connection and Authentication details!");
                return err;
            }

            mongoc_init();            // Initialize libmongoc's internals

            client = mongoc_client_new(
                    uri);    // Create a new client instance (Not thread safe) authenticating with the IDAM database

            DBConnect = (void*)client;        // No prior connection to IDAM Database
            if (DBConnect != NULL) {
                DBType = PLUGINSQLMONGODB;
                sqlPrivate = 1;            // the connection belongs to this plugin
                IDAM_LOG(LOG_DEBUG, "mongodbplugin: Private regular database connection made.\n");
            }
        }

        if (!client) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                         "No connection to Database server made!");
            IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No connection to Database server made!");
            return err;
        }

        database = mongoc_client_get_database(client, environment->sql_dbname);

        if (!database) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                         "No connection to Database cluster made!");
            IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No connection to Database cluster made!");
            return err;
        }

        if ((env = getenv("UDA_SQLTABLE")) != NULL) {
            collection = mongoc_client_get_collection(client, environment->sql_dbname, env);
        } else {
            collection = mongoc_client_get_collection(client, environment->sql_dbname,
                                                      "maps");
        }    // Get a handle on the database and collection

        if (!collection) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                         "No handle to Database collection made!");
            IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No handle to Database collection made!");
            return err;
        }

// Reduce logging overhead

        //mongoc_log_trace_disable();		// needs mongoc to have been built with --enable-tracing
        //mongoc_log_set_handler (NULL, NULL);	// causes a seg fault!

        init = 1;
    }

//----------------------------------------------------------------------------------------
// Return the private (local) connection Object
// Not intended for client use ...

    if (STR_IEQUALS(request_block->function, "connection")) {

        if (sqlPrivate && DBConnect != NULL) {
            idam_plugin_interface->sqlConnectionType = PLUGINSQLMONGODB;
            idam_plugin_interface->sqlConnection = (void*)client;
        } else {
            idam_plugin_interface->sqlConnectionType = PLUGINSQLNOTKNOWN;
            idam_plugin_interface->sqlConnection = NULL;
        }

        return 0;
    }

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------
/* 
 The query method is controlled by passed name value pairs. This method and plugin is called explicitly for use cases where this is just another regular plugin resource.

 The default method is called when the plugin has the 'special' role of resolving generic signal names using the IDAM database containing name mappings etc. 
 Neither plugin name nor method name is included in the user request for this specific use case. Therefore, the default method must be identified via the plugin registration 
 process and added to the request.  

 query( [signal|objectName]=objectName, [shot|exp_number|pulse|pulno]=exp_number [,source|objectSource]=objectSource [,device=device] [,type=type] [,sourceClass=sourceClass] [,objectClass=objectClass] [,/allMeta])
 get()

*/

    do {

        if (STR_IEQUALS(request_block->function, "query") ||
            STR_IEQUALS(request_block->function, THISPLUGIN_DEFAULT_METHOD)) {

            bool isObjectName;
            bool isObjectSource = false;
            bool isExpNumber = false;
            bool isDevice;
            bool isType = false;
            bool isSourceClass = false;
            bool isObjectClass = false;
            bool allMeta;

            const char* objectName = NULL;
            const char* objectSource = NULL;
            int expNumber = 0;
            const char* device = NULL;
            const char* type = NULL;
            const char* sourceClass = NULL;
            const char* objectClass = NULL;

            if (STR_IEQUALS(request_block->function, "query")) {
                // Name Value pairs => a regular returned DATA_BLOCK

                isObjectName = findStringValue(&request_block->nameValueList, &objectName, "signal|objectName");
                isObjectSource = findStringValue(&request_block->nameValueList, &objectName, "source|objectSource");
                isExpNumber = findIntValue(&request_block->nameValueList, &expNumber, "shot|exp_number|pulse|pulno");
                isDevice = FIND_STRING_VALUE(request_block->nameValueList, device);
                isType = FIND_STRING_VALUE(request_block->nameValueList, type);
                isSourceClass = FIND_STRING_VALUE(request_block->nameValueList, sourceClass);
                isObjectClass = FIND_STRING_VALUE(request_block->nameValueList, objectClass);
                allMeta = findValue(&request_block->nameValueList, "allMeta");

            } else {
                // Default Method: Names and shot or source passed via the standard legacy API arguments => returned SIGNAL_DESC and DATA_SOURCE

                isObjectName = 1;
                objectName = request_block->signal;

                if (request_block->exp_number > 0) {
                    isExpNumber = 1;
                    expNumber = request_block->exp_number;
                }
                if (request_block->tpass[0] != '\0') {
                    isObjectSource = 1;
                    objectSource = request_block->tpass;
                }

                isDevice = 0;
                allMeta = 0;
            }

// Mandatory arguments

            if (!isObjectName) {
                IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No Data Object Name specified\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "No Data Object Name specified");
                break;
            }

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
                    IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No Experiment Number or data source specified\n");
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err,
                                 "No Experiment Number or data source specified");
                    break;
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

case insensitive search    {name:{'$regex' : '^string$', '$options' : 'i'}}      Does not use the index so very slow!

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
            query = BCON_NEW ("$or", "[",
                              "{", "signal_alias", BCON_UTF8(name), "}",
                              "{", "generic_name", BCON_UTF8(name), "}",
                              "]");
            free(name);

            if (isExpNumber) {
                BCON_APPEND (query, "$and", "[",
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

            if (isDevice) {
                char* lower = strlwr(strdup(device));
                char* upper = strupr(strdup(device));
                BCON_APPEND (query, "$and", "[",
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
                BCON_APPEND (query, "$and", "[",
                             "{", "$or", "[",
                             "{", "objectSource", "{", "$eq", BCON_UTF8(lower), "}", "}",
                             "{", "objectSource", "{", "$eq", BCON_UTF8(upper), "}", "}",
                             "]",
                             "}",
                             "]");
                free(lower);
                free(upper);
            }
            if (isType) {
                char* lower = strlwr(strdup(type));
                char* upper = strupr(strdup(type));
                BCON_APPEND (query, "$and", "[",
                             "{", "$or", "[",
                             "{", "type", "{", "$eq", BCON_UTF8(lower), "}", "}",
                             "{", "type", "{", "$eq", BCON_UTF8(upper), "}", "}",
                             "]",
                             "}",
                             "]");
                free(lower);
                free(upper);
            }
            if (isSourceClass) {
                char* lower = strlwr(strdup(sourceClass));
                char* upper = strupr(strdup(sourceClass));
                BCON_APPEND (query, "$and", "[",
                             "{", "$or", "[",
                             "{", "source_alias", "{", "$eq", BCON_UTF8(lower), "}", "}",
                             "{", "source_alias", "{", "$eq", BCON_UTF8(upper), "}", "}",
                             "]",
                             "}",
                             "]");
                free(lower);
                free(upper);
            }

            if (isObjectClass) {
                char* lower = strlwr(strdup(objectClass));
                char* upper = strupr(strdup(objectClass));
                BCON_APPEND (query, "$and", "[",
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

#if MONGOC_CHECK_VERSION(1, 6, 0)
            bson_t* opts = BCON_NEW("limit", BCON_INT64(limit));

            if (!(cursor = mongoc_collection_find_with_opts(collection, query, opts, NULL))) {
                IDAM_LOG(LOG_ERROR, "MongoDBPlugin: Data Object not found!\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "Data Object not found!");
                break;
            }
#else
            if(!(cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, limit, 0, query, NULL, NULL))){
                IDAM_LOG(LOG_ERROR, "MongoDBPlugin: Data Object not found!\n");
                err =  999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "Data Object not found!");
                break;
            }
#endif

/*
// Count is very slow - > 40ms ! so do not use
         bson_error_t error;
         int64_t docCount = mongoc_collection_count(collection, MONGOC_QUERY_NONE, query, 0, 0, NULL, &error);

         if(debugon){
            int cost;      
            gettimeofday(&tv_end, NULL); 
            cost = (int)(tv_end.tv_sec-tv_start.tv_sec)*1000000 + (int)(tv_end.tv_usec - tv_start.tv_usec);
            fprintf(dbgout,"+++ mongodbPlugin +++\n");
            fprintf(dbgout,"Count Time: %d (micro sec)\n",cost);
            tv_start = tv_end;
         }         

         if(docCount != 1){
            if(docCount == 0){
               if(verbose) fprintf (errout, "MongoDBPlugin: No data object found!\n");
               if(debugon) fprintf (dbgout, "MongoDBPlugin: No data object found!\n");
               err =  999;
               addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "No data object found!");
               break;
            } 
            if(verbose) fprintf (errout, "MongoDBPlugin: Too many data objects found!\n");
            if(debugon) fprintf (dbgout, "MongoDBPlugin: Too many data objects found!\n");
            err =  999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "Too many data objects found!");
            break;
         }
*/

// Test for a returned BSON document

// Iterate over the document for the fields of interest (not very fast!  Typically > 40ms! and same as mongoc_collection_count)
// Returned Structures should be initialised prior to entry


            int docAvail = mongoc_cursor_more(cursor);

            int docCount = 0;

            while (1 && docAvail && mongoc_cursor_next(cursor, &doc)) {        // Get the document (slow!)
                bson_iter_t iter;
                bson_iter_t value;

                if (docCount++ > 0) {
                    IDAM_LOG(LOG_ERROR, "MongoDBPlugin: Too many data objects found!\n");
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "Too many data objects found!");
                    break;
                }

                request_block->exp_number = expNumber;

                char* dbDevice = NULL;
                if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source_device", &value) &&
                    BSON_ITER_HOLDS_UTF8 (&value)) {
                    uint32_t length = 0;
                    const char* str = bson_iter_utf8(&value, &length);
                    dbDevice = (char*)str;
                    sprintf(data_source->path, "%s%s%d", str, request_block->api_delim, expNumber);
                    strcpy(request_block->source, data_source->path);
                }
                if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "signal_name", &value) &&
                    BSON_ITER_HOLDS_UTF8 (&value)) {
                    uint32_t length = 0;
                    const char* str = bson_iter_utf8(&value, &length);
                    strcpy(signal_desc->signal_name, str);
                }
                if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "type", &value) &&
                    BSON_ITER_HOLDS_UTF8 (&value)) {
                    uint32_t length = 0;
                    const char* str = bson_iter_utf8(&value, &length);
                    signal_desc->type = str[0];
                }
                if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source", &value) &&
                    BSON_ITER_HOLDS_UTF8 (&value)) {
                    uint32_t length = 0;
                    const char* str = bson_iter_utf8(&value, &length);
                    if (!isExpNumber) {
                        if (dbDevice != NULL)
                            sprintf(data_source->path, "%s%s%s", dbDevice, request_block->api_delim, str);
                        else
                            sprintf(data_source->path, "%s", str);
                    } else
                        sprintf(data_source->path, "%s%s%d/%s", dbDevice, request_block->api_delim, expNumber, str);
                }

                if (allMeta) {
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "generic_name", &value) &&
                        BSON_ITER_HOLDS_UTF8 (&value)) {
                        uint32_t length = 0;
                        const char* str = bson_iter_utf8(&value, &length);
                        strcpy(signal_desc->generic_name, str);
                    }
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "source_alias", &value) &&
                        BSON_ITER_HOLDS_UTF8 (&value)) {
                        uint32_t length = 0;
                        const char* str = bson_iter_utf8(&value, &length);
                        strcpy(signal_desc->source_alias, str);
                    }
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "signal_class", &value) &&
                        BSON_ITER_HOLDS_UTF8 (&value)) {
                        uint32_t length = 0;
                        const char* str = bson_iter_utf8(&value, &length);
                        strcpy(signal_desc->signal_class, str);
                    }
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "description", &value) &&
                        BSON_ITER_HOLDS_UTF8 (&value)) {
                        uint32_t length = 0;
                        const char* str = bson_iter_utf8(&value, &length);
                        strcpy(signal_desc->description, str);
                    }
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "range_start", &value) &&
                        BSON_ITER_HOLDS_INT32 (&value)) {
                        signal_desc->range_start = bson_iter_int32(&value);
                    }
                    if (bson_iter_init(&iter, doc) && bson_iter_find_descendant(&iter, "range_end", &value) &&
                        BSON_ITER_HOLDS_INT32 (&value)) {
                        signal_desc->range_stop = bson_iter_int32(&value);
                    }
                }

                // strcpy(signal_desc->creation,     ???);
            }   // while

            bson_destroy(query);
            mongoc_cursor_destroy(cursor);

            if (docCount == 0) {
                IDAM_LOG(LOG_ERROR, "MongoDBPlugin: No data object found!\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "No data object found!");
                break;
            }

            if (err > 0) break;

// Return Result: Data Block with the Meta Data structures

            if (STR_IEQUALS(request_block->function, "query")) {

// Create the Returned Structure Definition

                initUserDefinedType(&usertype);            // New structure definition

                strcpy(usertype.name, "MONGO_R");
                usertype.size = sizeof(MONGO_R);

                strcpy(usertype.source, "mongoDBPlugin");
                usertype.ref_id = 0;
                usertype.imagecount = 0;                // No Structure Image data
                usertype.image = NULL;
                usertype.idamclass = TYPE_COMPOUND;

                offset = 0;

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

                addUserDefinedType(userdefinedtypelist, usertype);

// Create the data to be returned

                MONGO_R* data = (MONGO_R*)malloc(sizeof(MONGO_R));
                addMalloc((void*)data, 1, sizeof(MONGO_R), "MONGO_R");

                int lstr = strlen(objectName) + 1;
                data->objectName = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->objectName, 1, lstr * sizeof(char), "char");
                strcpy(data->objectName, objectName);

                lstr = strlen(device) + 1;
                data->device = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->device, 1, lstr * sizeof(char), "char");
                strcpy(data->device, device);

                lstr = strlen(objectSource) + 1;
                data->objectSource = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->objectSource, 1, lstr * sizeof(char), "char");
                strcpy(data->objectSource, objectSource);

                lstr = strlen(data_source->path) + 1;
                data->source = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->source, 1, lstr * sizeof(char), "char");
                strcpy(data->source, data_source->path);

                lstr = strlen(signal_desc->signal_name) + 1;
                data->signal_name = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->signal_name, 1, lstr * sizeof(char), "char");
                strcpy(data->signal_name, signal_desc->signal_name);

                lstr = 2;
                data->type = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->type, 1, lstr * sizeof(char), "char");
                data->type[0] = signal_desc->type;
                data->type[1] = '\0';

                lstr = strlen(signal_desc->signal_alias) + 1;
                data->signal_alias = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->signal_alias, 1, lstr * sizeof(char), "char");
                strcpy(data->signal_alias, signal_desc->signal_alias);

                lstr = strlen(signal_desc->generic_name) + 1;
                data->generic_name = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->generic_name, 1, lstr * sizeof(char), "char");
                strcpy(data->generic_name, signal_desc->generic_name);

                lstr = strlen(signal_desc->source_alias) + 1;
                data->source_alias = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->source_alias, 1, lstr * sizeof(char), "char");
                strcpy(data->source_alias, signal_desc->source_alias);

                lstr = strlen(signal_desc->signal_class) + 1;
                data->signal_class = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->signal_class, 1, lstr * sizeof(char), "char");
                strcpy(data->signal_class, signal_desc->signal_class);

                lstr = strlen(signal_desc->description) + 1;
                data->description = (char*)malloc(lstr * sizeof(char));
                addMalloc((void*)data->description, 1, lstr * sizeof(char), "char");
                strcpy(data->description, signal_desc->description);

                data->expNumber = expNumber;
                data->range_start = signal_desc->range_start;
                data->range_stop = signal_desc->range_stop;

// Return the Data	 

                data_block->data_type = TYPE_COMPOUND;
                data_block->rank = 0;
                data_block->data_n = 1;
                data_block->data = (char*)data;

                strcpy(data_block->data_desc, "mongoDB query result");
                strcpy(data_block->data_label, "");
                strcpy(data_block->data_units, "");

                data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
                data_block->opaque_count = 1;
                data_block->opaque_block = (void*)findUserDefinedType("MONGO_R", 0);

            }

            break;
        } else

//----------------------------------------------------------------------------------------    
// Help: A Description of library functionality

        if (STR_IEQUALS(request_block->function, "help")) {

            char* help = "\nMongoDBPlugin: Function Names, Syntax, and Descriptions\n\n"
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

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

            int i;
            for (i = 0; i < data_block->rank; i++) {
                initDimBlock(&data_block->dims[i]);
            }

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "MongoDBPlugin: help = description of this plugin");

            data_block->data = strdup(help);

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = strlen(help) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

//----------------------------------------------------------------------------------------    
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion 

        if (STR_IEQUALS(request_block->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Build Date

        if (STR_IEQUALS(request_block->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Default Method

        if (STR_IEQUALS(request_block->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char* data = (char*)malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Maximum Interface Version

        if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*)malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*)data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else {

//======================================================================================
// Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "MongoDBPlugin", err, "Unknown plugin function requested!");
            break;
        }

    } while (0);

    return err;
}

