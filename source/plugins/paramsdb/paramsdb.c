#include "paramsdb.h"

#include <stdlib.h>
#include <strings.h>
#include <libpq-fe.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/idamTypes.h>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_getActive(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int paramsdb(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static int init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

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

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!
        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise"))
            return 0;
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

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
    } else if (STR_IEQUALS(request_block->function, "getActive")) {
        return do_getActive(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*) malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\nParamsDB: Add Functions Names, Syntax, and Descriptions\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc, "ParamsDB: help = description of this plugin");

    data_block->data = (char*) p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
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
    data_block->data_type = TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*) malloc(sizeof(int));
    data[0] = THISPLUGIN_VERSION;
    data_block->data = (char*) data;
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
    data_block->data_type = TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(__DATE__) + 1;
    char* data = (char*) malloc(data_block->data_n * sizeof(char));
    strcpy(data, __DATE__);
    data_block->data = data;
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
    data_block->data_type = TYPE_STRING;
    data_block->rank = 0;
    data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
    char* data = (char*) malloc(data_block->data_n * sizeof(char));
    strcpy(data, THISPLUGIN_DEFAULT_METHOD);
    data_block->data = data;
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
    data_block->data_type = TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    int* data = (int*) malloc(sizeof(int));
    data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
    data_block->data = (char*) data;
    strcpy(data_block->data_desc, "Maximum Interface Version");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

static void
loggingNoticeProcessor(void* arg, const char* message)
{
    IDAM_LOGF(LOG_WARN, "%s", message);
}

//----------------------------------------------------------------------------------------
// Add functionality here ....
int do_getActive(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    char* coil = NULL;
    char* system = NULL;
    char* limit = NULL;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, coil);
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, system);
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, limit);

    PGconn* conn = PQconnectdb("host=idam3.mast.ccfe.ac.uk port=60000 dbname=paramsdb user=paramsdb password=paramsdb");

    PQsetNoticeProcessor(conn, loggingNoticeProcessor, NULL);

    FILE* log = fopen("/Users/jhollocombe/Projects/uda/etc/paramsdb.log", "w");
    idamSetLogLevel(LOG_DEBUG);
    idamSetLogFile(LOG_DEBUG, log);
    idamSetLogFile(LOG_INFO, log);
    idamSetLogFile(LOG_WARN, log);
    idamSetLogFile(LOG_ERROR, log);
    idamSetLogFile(LOG_ACCESS, log);

    if (PQstatus(conn) != CONNECTION_OK) {
        RAISE_PLUGIN_ERROR_F("DB connection failed", "DB connection failed: %s", PQerrorMessage(conn));
    }

    const char* params[3];
    params[0] = coil;
    params[1] = system;
    params[2] = limit;

    PGresult* res = PQexecParams(conn,
        "SELECT value FROM ActiveLimits"
            " WHERE coil_id = (SELECT id FROM Coil WHERE name = $1)"
            "   AND system_id = (SELECT id FROM System WHERE name = $2)"
            "   AND limit_id = (SELECT id FROM Limits WHERE type = $3)",
        3, NULL, params, NULL, NULL, 1
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    if (PQntuples(res) > 1) {
        RAISE_PLUGIN_ERROR("DB query returned multiple rows");
    }

    if (PQfsize(res, 0) != sizeof(double)) {
        RAISE_PLUGIN_ERROR("DB query returned column with unexpected data size");
    }

    const char* buf = PQgetvalue(res, 0, 0);

    PQclear(res);
    PQfinish(conn);

    data_block->rank = 0;
    data_block->dims = NULL;

    data_block->data_type = TYPE_DOUBLE;
    strcpy(data_block->data_desc, "Active coil limit");

    data_block->data = malloc(sizeof(double));
    memcpy(data_block->data, buf, sizeof(double));
    data_block->data_n = 1;

    strcpy(data_block->data_label, limit);
    strcpy(data_block->data_units, "");

    return 0;
}
