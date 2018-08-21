/*---------------------------------------------------------------
* v1 IDAM Plugin Template: Standardised plugin design template, just add ... 
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
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
#include "keyvaluePlugin.h"

#include <leveldb/c.h>
#include <stdlib.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <plugins/udaPlugin.h>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_write(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_writeoptions_t* woptions);

static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_readoptions_t* roptions);

static int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_writeoptions_t* woptions);

int keyValue(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static short init = 0;

    static leveldb_readoptions_t* roptions = NULL;
    static leveldb_writeoptions_t* woptions = NULL;
    static leveldb_options_t* options = NULL;
    static leveldb_t* db = NULL;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_EQUALS(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Free Heap & reset counters

        leveldb_close(db);
        db = NULL;

        leveldb_writeoptions_destroy(woptions);
        woptions = NULL;

        leveldb_readoptions_destroy(roptions);
        roptions = NULL;

        leveldb_options_destroy(options);
        options = NULL;

        init = 0;

        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        options = leveldb_options_create();
        leveldb_options_set_create_if_missing(options, 1);

        char* err = NULL;
        db = leveldb_open(options, "idam_ks", &err);
        if (err != NULL) {
            RAISE_PLUGIN_ERROR(err);
        }

        woptions = leveldb_writeoptions_create();
        roptions = leveldb_readoptions_create();

        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    int err;

    if (STR_IEQUALS(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "write")) {
        err = do_write(idam_plugin_interface, db, woptions);
    } else if (STR_IEQUALS(request_block->function, "read")) {
        err = do_read(idam_plugin_interface, db, roptions);
    } else if (STR_IEQUALS(request_block->function, "delete")) {
        err = do_delete(idam_plugin_interface, db, woptions);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return err;
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "templatePlugin: help = description of this plugin");

    data_block->data = (char*)p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_VERSION;
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin version number");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Build Date
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(__DATE__) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, __DATE__);
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin build date");
    strcpy(data_block->data_label, "date");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Default Method
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
    char* data = (char*)malloc(data_block->data_n * sizeof(char));
    strcpy(data, THISPLUGIN_DEFAULT_METHOD);
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Plugin default method");
    strcpy(data_block->data_label, "method");
    strcpy(data_block->data_units, "");

    return 0;
}

// Plugin Maximum Interface Version
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);
    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*)malloc(sizeof(int));
    data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Maximum Interface Version");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

int do_write(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_writeoptions_t* woptions)
{
    const char* key = NULL;
    const char* value = NULL;

    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, key);
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, value);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == NULL) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = NULL;
    leveldb_put(db, woptions, key, strlen(key), value, strlen(value), &err);

    if (err != NULL) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    return 0;
}

int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_readoptions_t* roptions)
{
    const char* key = NULL;

    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, key);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == NULL) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = NULL;
    size_t value_len;

    char* value = leveldb_get(db, roptions, key, strlen(key), &value_len, &err);

    if (err != NULL) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    idam_plugin_interface->data_block->data = value;
    idam_plugin_interface->data_block->data_n = value_len;

    return 0;
}

int do_delete(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, leveldb_t* db, leveldb_writeoptions_t* woptions)
{
    const char* key = NULL;

    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, key);

    char* env = getenv("UDA_PLUGIN_KEYVALUE_STORE");
    if (env == NULL) {
        RAISE_PLUGIN_ERROR("Environmental variable IDAM_PLUGIN_KEYVALUE_STORE not found");
    }

    if (STR_EQUALS(env, "LEVELDB")) {
        UDA_LOG(UDA_LOG_DEBUG, "Writing key %s to LevelDB keystore", key);
    } else {
        RAISE_PLUGIN_ERROR("Unknown keyvalue store requested");
    }

    char* err = NULL;

    leveldb_delete(db, woptions, key, strlen(key), &err);

    if (err != NULL) {
        RAISE_PLUGIN_ERROR_EX(err, { leveldb_free(err); });
    }

    return 0;
}