#include "paramsdb.h"

#include <stdlib.h>
#include <strings.h>
#include <libpq-fe.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <structures/struct.h>
#include <structures/accessors.h>

static char* db_host = "idam3.mast.ccfe.ac.uk";
static char* db_port = "60000";
static char* db_name = "paramsdb";
static char* db_user = "paramsdb";
static char* db_pass = "paramsdb";

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_getActiveLimit(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn);
static int do_getForceCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn);
static int do_getFilterCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn);
static int do_getBoardCalibrations(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn);
static int do_getCoilParameters(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn);

static void
loggingNoticeProcessor(void* arg, const char* message)
{
    IDAM_LOGF(UDA_LOG_WARN, "%s", message);
}

int paramsdb(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static int init = 0;

    idamSetLogLevel(UDA_LOG_DEBUG);

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

    static PGconn* conn = NULL;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!

        PQfinish(conn);
        conn = NULL;

        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        conn = PQsetdbLogin(db_host, db_port, NULL, NULL, db_name, db_user, db_pass);

        PQsetNoticeProcessor(conn, loggingNoticeProcessor, NULL);

        if (PQstatus(conn) != CONNECTION_OK) {
            RAISE_PLUGIN_ERROR_F("DB connection failed", "DB connection failed: %s", PQerrorMessage(conn));
        }

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
    } else if (STR_IEQUALS(request_block->function, "getActiveLimit")) {
        return do_getActiveLimit(idam_plugin_interface, conn);
    } else if (STR_IEQUALS(request_block->function, "getForceCoefficients")) {
        return do_getForceCoefficients(idam_plugin_interface, conn);
    } else if (STR_IEQUALS(request_block->function, "getFilterCoefficients")) {
        return do_getFilterCoefficients(idam_plugin_interface, conn);
    } else if (STR_IEQUALS(request_block->function, "getBoardCalibrations")) {
        return do_getBoardCalibrations(idam_plugin_interface, conn);
    } else if (STR_IEQUALS(request_block->function, "getCoilParameters")) {
        return do_getCoilParameters(idam_plugin_interface, conn);
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
    data_block->data = (char*)data;
    strcpy(data_block->data_desc, "Maximum Interface Version");
    strcpy(data_block->data_label, "version");
    strcpy(data_block->data_units, "");

    return 0;
}

// Struct to store results in
typedef struct ActiveLimitsStruct {
    char* system;
    char* subtype;
    char** coils;
    char** upper_lowers;
    double* values;
} ACTIVELIMITS_STRUCT;

// Struct to store results in
typedef struct ForceCoefficientsStruct {
    char* coil;
    char* subtype;
    char* upper_lower;
    char** coils;
    double* values;
} FORCECOEFFICIENTS_STRUCT;

// Struct to store results in
typedef struct FilterCoefficientsStruct {
    int filter;
    int* coefficients;
    double* values;
} FILTERCOEFFICIENTS_STRUCT;

// Struct to store results in
typedef struct BoardCalibrationsStruct {
    int board;
    int* channels;
    double* gains;
    double* offsets;
} BOARDCALIBRATION_STRUCTS;

// Struct to store results in
typedef struct CoilParameters {
    char* coil;
    char* upper_lower;
    char* parameter;
    char** names;
    double* values;
} COILPARAMETERS_STRUCT;

typedef struct ParamsDB {
    ACTIVELIMITS_STRUCT* activelimits;
    FORCECOEFFICIENTS_STRUCT* forcecoefficients;
    FILTERCOEFFICIENTS_STRUCT* filtercoefficients;
    BOARDCALIBRATION_STRUCTS* boardcalibration;
    COILPARAMETERS_STRUCT* coilparameters;
} PARAMSDB_STRUCT;

//static ACTIVELIMITS_STRUCT* allocActiveLimits(const char* system, const char* subtype, const char* coil, int nrows)
//{
//    ACTIVELIMITS_STRUCT* data = (ACTIVELIMITS_STRUCT*)malloc(sizeof(ACTIVELIMITS_STRUCT));
//    addMalloc(data, 1, sizeof(ACTIVELIMITS_STRUCT), "ACTIVELIMITS_STRUCT");
//
//    data->system = strdup(system);
//    addMalloc(data->system, (int)strlen(system), sizeof(char), "STRING");
//
//    if (subtype != NULL) {
//        data->subtype = strdup(subtype);
//        addMalloc(data->subtype, (int)strlen(subtype), sizeof(char), "STRING");
//    } else {
//        data->subtype = NULL;
//    }
//
////    data->coils = (char**)malloc(nrows * sizeof(char*));
////    addMalloc(data->coils, nrows, sizeof(char*), "STRING *");
////
////    data->upper_lowers = (char**)malloc(nrows * sizeof(char*));
////    addMalloc(data->upper_lowers, nrows, sizeof(char*), "STRING *");
////
////    data->values = (double*)malloc(nrows * sizeof(double));
////    addMalloc(data->values, nrows, sizeof(double), "double");
//
//    return data;
//}

static PGresult* activeLimitsQuery(PGconn* conn, const char* system, const char* subtype, bool is_subtype, const char* coil, bool is_coil)
{
    PGresult* res = NULL;

#define ACITIVELIMIT_SQL "SELECT c.name, al.upper_lower_id, al.value" \
            " FROM ActiveLimits al" \
            " JOIN Coil AS c ON al.coil_id = c.id" \
            " JOIN System AS s ON al.system_id = s.id" \
            " JOIN Limits AS l ON al.limit_id = l.id"

    if (is_subtype && is_coil) {
        const char* params[3];
        params[0] = system;
        params[1] = subtype;
        params[2] = coil;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1 AND s.subtype = $2 AND c.name = $3";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    } else if (is_subtype) {
        const char* params[2];
        params[0] = system;
        params[1] = subtype;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1 AND s.subtype = $2";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    } else if (is_coil) {
        const char* params[2];
        params[0] = system;
        params[1] = coil;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1 AND c.name = $2";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    } else {
        const char* params[1];
        params[0] = system;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    }

    return res;
}

static FORCECOEFFICIENTS_STRUCT*
allocForceCoefficients(const char* coil, const char* subtype, const char* upper_lower, int nrows)
{
    FORCECOEFFICIENTS_STRUCT* data = (FORCECOEFFICIENTS_STRUCT*)malloc(sizeof(FORCECOEFFICIENTS_STRUCT));
    addMalloc(data, 1, sizeof(FORCECOEFFICIENTS_STRUCT), "FORCECOEFFICIENTS_STRUCT");

    data->coil = strdup(coil);
    addMalloc(data->coils, (int)strlen(coil), sizeof(char), "STRING");

    data->subtype = strdup(subtype);
    addMalloc(data->subtype, (int)strlen(subtype), sizeof(char), "STRING");

    data->upper_lower = strdup(upper_lower);
    addMalloc(data->upper_lower, (int)strlen(upper_lower), sizeof(char), "STRING");

    data->coils = (char**)malloc(nrows * sizeof(char*));
    addMalloc(data->coils, nrows, sizeof(char*), "STRING *");

    data->values = (double*)malloc(nrows * sizeof(double));
    addMalloc(data->values, nrows, sizeof(double), "DOUBLE *");
    return data;
}

static FILTERCOEFFICIENTS_STRUCT* allocFilterCoefficients(int filter, int nrows)
{
    FILTERCOEFFICIENTS_STRUCT* data = (FILTERCOEFFICIENTS_STRUCT*)malloc(sizeof(FILTERCOEFFICIENTS_STRUCT));
    addMalloc(data, 1, sizeof(FILTERCOEFFICIENTS_STRUCT), "FILTERCOEFFICIENTS_STRUCT");

    data->filter = filter;

    data->coefficients = (int*)malloc((nrows) * sizeof(int));
    addMalloc(data->coefficients, nrows, sizeof(int), "INT *");

    data->values = (double*)malloc((nrows) * sizeof(double));
    addMalloc(data->values, nrows, sizeof(double), "DOUBLE *");
    return data;
}

static BOARDCALIBRATION_STRUCTS* allocBoardCalibration(int board, int nrows)
{
    BOARDCALIBRATION_STRUCTS* data = (BOARDCALIBRATION_STRUCTS*)malloc(sizeof(BOARDCALIBRATION_STRUCTS));
    addMalloc(data, 1, sizeof(BOARDCALIBRATION_STRUCTS), "BOARDCALIBRATION_STRUCTS");

    data->board = board;

    data->channels = (int*)malloc((nrows) * sizeof(int));
    addMalloc(data->channels, nrows, sizeof(int), "INT *");

    data->gains = (double*)malloc((nrows) * sizeof(double));
    addMalloc(data->gains, nrows, sizeof(double), "DOUBLE *");

    data->offsets = (double*)malloc((nrows) * sizeof(double));
    addMalloc(data->offsets, nrows, sizeof(double), "DOUBLE *");

    return data;
}

static COILPARAMETERS_STRUCT*
allocCoilParameters(const char* coil, const char* upper_lower, const char* parameter, int nrows)
{
    COILPARAMETERS_STRUCT* data = (COILPARAMETERS_STRUCT*)malloc(sizeof(COILPARAMETERS_STRUCT));
    addMalloc(data, 1, sizeof(COILPARAMETERS_STRUCT), "COILPARAMETERS_STRUCT");

    data->coil = strdup(coil);
    addMalloc(data->coil, (int)strlen(coil), sizeof(char), "STRING");

    data->upper_lower = strdup(upper_lower);
    addMalloc(data->upper_lower, (int)strlen(upper_lower), sizeof(char), "STRING");

    data->parameter = strdup(parameter);
    addMalloc(data->parameter, (int)strlen(parameter), sizeof(char), "STRING");

    data->names = (char**)malloc(nrows * sizeof(char*));
    addMalloc(data->names, nrows, sizeof(char*), "STRING *");

    data->values = (double*)malloc(nrows * sizeof(double));
    addMalloc(data->values, nrows, sizeof(double), "DOUBLE *");
    return data;
}

//call: getAll(system=?)
//returns:
//  {
//    ActiveLimits:
//      SubType1: [ (Coil1, UpperLower, Value), ... ]
//      ...
//    ForceCoefficients:
//      SubType1:
//        Upper: [ (Coil, Value), ... ]
//        ...
//      ...
//    FilterCoefficients:
//
//    BoardCalibrations:
//    CoilParamters:
//  }
//  [ (TFP1, Only, value), (TF, Only, value), ... ]
int do_getAll(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    const char* system = NULL;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, system);

    PARAMSDB_STRUCT* data = (PARAMSDB_STRUCT*)malloc(sizeof(ACTIVELIMITS_STRUCT));
    addMalloc(data, 1, sizeof(PARAMSDB_STRUCT), "PARAMSDB_STRUCT");

    PGresult* res = activeLimitsQuery(conn, system, NULL, FALSE, NULL, FALSE);

    data->activelimits = (ACTIVELIMITS_STRUCT*)malloc(sizeof(ACTIVELIMITS_STRUCT));
    addMalloc(data, 1, sizeof(ACTIVELIMITS_STRUCT), "ACTIVELIMITS_STRUCT *");

    data->forcecoefficients = (FORCECOEFFICIENTS_STRUCT*)malloc(sizeof(FORCECOEFFICIENTS_STRUCT));
    addMalloc(data, 1, sizeof(FORCECOEFFICIENTS_STRUCT), "FORCECOEFFICIENTS_STRUCT *");

    data->filtercoefficients = (FILTERCOEFFICIENTS_STRUCT*)malloc(sizeof(FILTERCOEFFICIENTS_STRUCT));
    addMalloc(data, 1, sizeof(FILTERCOEFFICIENTS_STRUCT), "FILTERCOEFFICIENTS_STRUCT *");

    data->boardcalibration = (BOARDCALIBRATION_STRUCTS*)malloc(sizeof(BOARDCALIBRATION_STRUCTS));
    addMalloc(data, 1, sizeof(BOARDCALIBRATION_STRUCTS), "BOARDCALIBRATION_STRUCTS *");

    data->coilparameters = (COILPARAMETERS_STRUCT*)malloc(sizeof(COILPARAMETERS_STRUCT));
    addMalloc(data, 1, sizeof(COILPARAMETERS_STRUCT), "COILPARAMETERS_STRUCT *");


//    int i;
//    for (i = 0; i < nrows; i++) {
//        const char* buf = PQgetvalue(res, i, 0);
//        data->coils[i] = strdup(buf);
//        addMalloc(data->coils[i], (int)strlen(data->coils[i]), sizeof(char), "char");
//
//        buf = PQgetvalue(res, i, 1);
//        data->upper_lowers[i] = strdup(buf);
//        addMalloc(data->upper_lowers[i], (int)strlen(data->upper_lowers[i]), sizeof(char), "char");
//
//        buf = PQgetvalue(res, i, 2);
//        data->values[i] = atof(buf);
//    }

    PQclear(res);

    USERDEFINEDTYPE parentTree;

    initUserDefinedType(&parentTree);
    parentTree.idamclass = TYPE_COMPOUND;
    strcpy(parentTree.name, "ACTIVELIMITS_STRUCT");
    strcpy(parentTree.source, "paramsdb");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(ACTIVELIMITS_STRUCT);

    COMPOUNDFIELD field;
    int offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "coils");
    defineField(&field, "coils", "coils", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "upper_lowers");
    defineField(&field, "upper_lowers", "upper_lowers", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "values");
    defineField(&field, "values", "values", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    addUserDefinedType(userdefinedtypelist, parentTree);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Active Limits");
    strcpy(data_block->data_label, "Active Limits");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("ACTIVELIMITS_STRUCT", 0);

    return 0;
}

//call: getActiveLimit(system=?, [subtype=?], [coil=?])
//returns:
//  [ (TFP1, Only, value), (TF, Only, value), ... ]
int do_getActiveLimit(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* system = NULL;
    const char* subtype = NULL;
    const char* coil = NULL;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, system);
    bool is_subtype = FIND_STRING_VALUE(request_block->nameValueList, subtype);
    bool is_coil = FIND_STRING_VALUE(request_block->nameValueList, coil);

    PGresult* res = activeLimitsQuery(conn, system, subtype, is_subtype, coil, is_coil);

    int nrows = PQntuples(res);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    if (is_subtype && is_coil && nrows > 1) {
        RAISE_PLUGIN_ERROR("DB query returned multiple rows");
    }

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);

    strcpy(usertype.name, "ACTIVELIMITS_STRUCT");
    strcpy(usertype.source, "paramsdb");
    usertype.ref_id = 0;
    usertype.imagecount = 0;
    usertype.image = NULL;
    usertype.size = sizeof(ACTIVELIMITS_STRUCT);
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    defineField(&field, "system", "string structure element", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "subtype", "string structure element", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "coils", "string array structure element", &offset, ARRAYSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "upper_lowers", "string array structure element", &offset, ARRAYSTRING);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "values", "string array structure element", &offset, ARRAYDOUBLE);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    ACTIVELIMITS_STRUCT* data = (ACTIVELIMITS_STRUCT*)malloc(sizeof(ACTIVELIMITS_STRUCT));
    addMalloc((void*)data, 1, sizeof(ACTIVELIMITS_STRUCT), "ACTIVELIMITS_STRUCT");

    data->system = strdup(system);
    addMalloc(data->system, 1, (strlen(system) + 1) * sizeof(char), "STRING");

    data->subtype = strdup(subtype);
    addMalloc(data->subtype, 1, (strlen(subtype) + 1) * sizeof(char), "STRING");

    int* shape = malloc(sizeof(int));
    shape[0] = nrows;

    data->coils = malloc(nrows * sizeof(char*));
    addMalloc2(data->coils, nrows, sizeof(char*), "STRING *", 1, shape);

    data->upper_lowers = malloc(nrows * sizeof(char*));
    addMalloc2(data->upper_lowers, nrows, sizeof(char*), "STRING *", 1, shape);

    data->values = malloc(nrows * sizeof(double));
    addMalloc(data->values, nrows, sizeof(double), "double");

    for (int i = 0; i < nrows; i++) {
        const char* buf = PQgetvalue(res, i, 0);
        data->coils[i] = strdup(buf);
        addMalloc(data->coils[i], (int)strlen(data->coils[i]) + 1, sizeof(char), "char");

        buf = PQgetvalue(res, i, 1);
        data->upper_lowers[i] = strdup(buf);
        addMalloc(data->upper_lowers[i], (int)strlen(data->upper_lowers[i]) + 1, sizeof(char), "char");

        buf = PQgetvalue(res, i, 2);
        data->values[i] = atof(buf);
    }

    PQclear(res);

    // Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Active Limits");
    strcpy(data_block->data_label, "Active Limits");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("ACTIVELIMITS_STRUCT", 0);

    return 0;
}

//
//call: getForceCoefficients(coil=?, subtype=?, upper_lower=?)
//returns:
//  [ (P1, value), (Pc, value), ... ]
int do_getForceCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    const char* coil = NULL;
    const char* subtype = NULL;
    const char* upper_lower = NULL;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, coil);
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, subtype);
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, upper_lower);

    PGresult* res = NULL;

    const char* params[3];
    params[0] = coil;
    params[1] = subtype;
    params[2] = upper_lower;

    res = PQexecParams(conn,
                       "SELECT c2.name, cm.force_coefficient"
                               " FROM CoilMatrix AS cm"
                               " JOIN Coil AS c ON cm.coil_id = c.id"
                               " JOIN Coil AS c2 on cm.coil_driven_id = c2.id"
                               " WHERE c.name = $1"
                               "   AND cm.subtype = $2"
                               "   AND upper_lower_id = $3",
                       3, NULL, params, NULL, NULL, 0
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    FORCECOEFFICIENTS_STRUCT* data = allocForceCoefficients(coil, subtype, upper_lower, nrows);

    int i;
    for (i = 0; i < nrows; i++) {
        const char* buf = PQgetvalue(res, i, 0);
        data->coils[i] = strdup(buf);
        addMalloc(data->coils[i], (int)strlen(data->coils[i]), sizeof(char), "char");

        buf = PQgetvalue(res, i, 2);
        data->values[i] = atof(buf);
    }

    PQclear(res);

    USERDEFINEDTYPE parentTree;

    initUserDefinedType(&parentTree);
    parentTree.idamclass = TYPE_COMPOUND;
    strcpy(parentTree.name, "FORCECOEFFICIENTS_STRUCT");
    strcpy(parentTree.source, "paramsdb");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(FORCECOEFFICIENTS_STRUCT);

    COMPOUNDFIELD field;
    int offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "coil");
    defineField(&field, "coil", "coil", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "subtype");
    defineField(&field, "subtype", "subtype", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "upperlower");
    defineField(&field, "upperlower", "upperlower", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "coils");
    defineField(&field, "coils", "coils", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "values");
    defineField(&field, "values", "values", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    addUserDefinedType(userdefinedtypelist, parentTree);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Force Coefficients");
    strcpy(data_block->data_label, "Force Coefficients");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("FORCECOEFFICIENTS_STRUCT", 0);

    return 0;
}

//call: getFilterCoefficients(filter=?)
//returns:
//  [ (0, value), (1, value), ..., (64, value) ]
int do_getFilterCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    int filter = 0;

    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, filter);

    PGresult* res = NULL;

    const char* params[1];
    params[0] = FormatString("%d", filter);

    res = PQexecParams(conn,
                       "SELECT coefficient, value"
                               " FROM FilterCoefficient"
                               " WHERE filter = $1",
                       1, NULL, params, NULL, NULL, 0
    );

    free((void*)params[0]);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    FILTERCOEFFICIENTS_STRUCT* data = allocFilterCoefficients(filter, nrows);

    int i;
    for (i = 0; i < nrows; i++) {
        const char* buf = PQgetvalue(res, i, 0);
        data->coefficients[i] = atoi(buf);

        buf = PQgetvalue(res, i, 1);
        data->values[i] = atof(buf);
    }

    PQclear(res);

    USERDEFINEDTYPE parentTree;

    initUserDefinedType(&parentTree);
    parentTree.idamclass = TYPE_COMPOUND;
    strcpy(parentTree.name, "FILTERCOEFFICIENTS_STRUCT");
    strcpy(parentTree.source, "paramsdb");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(FILTERCOEFFICIENTS_STRUCT);

    COMPOUNDFIELD field;
    int offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "filter");
    defineField(&field, "filter", "filter", &offset, SCALARINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "coefficients");
    defineField(&field, "coefficients", "coefficients", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "values");
    defineField(&field, "values", "values", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    addUserDefinedType(userdefinedtypelist, parentTree);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Filter Coefficients");
    strcpy(data_block->data_label, "Filter Coefficients");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("FILTERCOEFFICIENTS_STRUCT", 0);

    return 0;
}

//call: getBoardCalibrations(board=?)
//returns:
//  [ (0, gain, offset), (1, gain, offset), ..., (31, gain, offset) ]
int do_getBoardCalibrations(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    int board = 0;

    FIND_REQUIRED_INT_VALUE(request_block->nameValueList, board);

    PGresult* res = NULL;

    const char* params[1];
    params[0] = FormatString("%d", board);

    res = PQexecParams(conn,
                       "SELECT channel, gain, cal_offset"
                               " FROM BoardCalibration"
                               " WHERE board = $1",
                       1, NULL, params, NULL, NULL, 0
    );

    free((void*)params[0]);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    BOARDCALIBRATION_STRUCTS* data = allocBoardCalibration(board, nrows);

    data->board = board;

    int i;
    for (i = 0; i < nrows; i++) {
        const char* buf = PQgetvalue(res, i, 0);
        data->channels[i] = atoi(buf);

        buf = PQgetvalue(res, i, 1);
        data->gains[i] = atof(buf);

        buf = PQgetvalue(res, i, 2);
        data->offsets[i] = atof(buf);
    }

    PQclear(res);
    PQfinish(conn);

    USERDEFINEDTYPE parentTree;

    initUserDefinedType(&parentTree);
    parentTree.idamclass = TYPE_COMPOUND;
    strcpy(parentTree.name, "BOARDCALIBRATION_STRUCTS");
    strcpy(parentTree.source, "paramsdb");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(BOARDCALIBRATION_STRUCTS);

    COMPOUNDFIELD field;
    int offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "board");
    defineField(&field, "board", "board", &offset, SCALARINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "channels");
    defineField(&field, "channels", "channels", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "gains");
    defineField(&field, "gains", "gains", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "offsets");
    defineField(&field, "offsets", "offsets", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    addUserDefinedType(userdefinedtypelist, parentTree);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Board Calibrations");
    strcpy(data_block->data_label, "Board Calibrations");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("BOARDCALIBRATION_STRUCTS", 0);

    return 0;
}

//call: getCoilParameters(coil=?, upper_lower=?, parameter=?)
//returns:
//  value
int do_getCoilParameters(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    const char* coil = NULL;
    const char* upper_lower = NULL;
    const char* parameter = NULL;

    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, coil);
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, upper_lower);
    bool is_parameter = FIND_STRING_VALUE(request_block->nameValueList, parameter);

    PGresult* res = NULL;

    if (is_parameter) {
        const char* params[3];
        params[0] = coil;
        params[1] = upper_lower;
        params[2] = parameter;

        res = PQexecParams(conn,
                           "SELECT p.name, cp.value"
                                   " FROM CoilParameters AS cp"
                                   " JOIN Coil AS c ON cp.coil_id = c.id"
                                   " JOIN Parameter AS p ON cp.parameter_id = p.id"
                                   " WHERE c.name = $1"
                                   "   AND cp.upper_lower_id = $2"
                                   "   AND p.name = $3",
                           3, NULL, params, NULL, NULL, 0
        );
    } else {
        const char* params[2];
        params[0] = coil;
        params[1] = upper_lower;

        res = PQexecParams(conn,
                           "SELECT p.name, cp.value"
                                   " FROM CoilParameters AS cp"
                                   " JOIN Coil AS c ON cp.coil_id = c.id"
                                   " JOIN Parameter AS p ON cp.parameter_id = p.id"
                                   " WHERE c.name = $1"
                                   "   AND cp.upper_lower_id = $2",
                           2, NULL, params, NULL, NULL, 0
        );
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    if (is_parameter && PQntuples(res) > 1) {
        RAISE_PLUGIN_ERROR("DB query returned multiple rows");
    }

    COILPARAMETERS_STRUCT* data = allocCoilParameters(coil, upper_lower, parameter, nrows);

    int i;
    for (i = 0; i < nrows; i++) {
        const char* buf = PQgetvalue(res, i, 0);
        data->names[i] = strdup(buf);
        addMalloc(data->names[i], (int)strlen(data->names[i]), sizeof(char), "char");

        buf = PQgetvalue(res, i, 2);
        data->values[i] = atof(buf);
    }

    PQclear(res);
    PQfinish(conn);

    USERDEFINEDTYPE parentTree;

    initUserDefinedType(&parentTree);
    parentTree.idamclass = TYPE_COMPOUND;
    strcpy(parentTree.name, "COILPARAMETERS_STRUCT");
    strcpy(parentTree.source, "paramsdb");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(COILPARAMETERS_STRUCT);

    COMPOUNDFIELD field;
    int offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "coil");
    defineField(&field, "coil", "coil", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "upper_lower");
    defineField(&field, "upper_lower", "upper_lower", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "parameter");
    defineField(&field, "parameter", "parameter", &offset, SCALARSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "names");
    defineField(&field, "names", "names", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "values");
    defineField(&field, "values", "values", &offset, ARRAYDOUBLE);
    addCompoundField(&parentTree, field);

    addUserDefinedType(userdefinedtypelist, parentTree);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Coil Parameters");
    strcpy(data_block->data_label, "Coil Parameters");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("COILPARAMETERS_STRUCT", 0);

    return 0;
}