#include "paramsdb.h"

#include <stdlib.h>
#include <strings.h>
#include <libpq-fe.h>

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <stddef.h>

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

static PGresult* activeLimitsQuery(PGconn* conn, const char* system, const char* subtype, bool is_subtype, const char* coil, bool is_coil)
{
    PGresult* res = NULL;

#define ACITIVELIMIT_SQL "SELECT s.subtype, c.name, al.upper_lower_id, al.value" \
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
        IDAM_LOGF(LOG_DEBUG, "params: ('%s', '%s', '%s')\n", system, subtype, coil);
        res = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    } else if (is_subtype) {
        const char* params[2];
        params[0] = system;
        params[1] = subtype;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1 AND s.subtype = $2";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        IDAM_LOGF(LOG_DEBUG, "params: ('%s', '%s')\n", system, subtype);
        res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    } else if (is_coil) {
        const char* params[2];
        params[0] = system;
        params[1] = coil;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1 AND c.name = $2";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        IDAM_LOGF(LOG_DEBUG, "params: ('%s', '%s')\n", system, coil);
        res = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);
    } else {
        const char* params[1];
        params[0] = system;

        char* sql = ACITIVELIMIT_SQL " WHERE s.name = $1";
        IDAM_LOGF(UDA_LOG_DEBUG, "sql: %s\n", sql);
        IDAM_LOGF(LOG_DEBUG, "params: ('%s')\n", system);
        res = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);
    }

    return res;
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

    IDAM_LOGF(LOG_DEBUG, "num rows: %d\n", nrows);

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    typedef struct CoilStruct {
        char* name;
        char* upper_lower;
        double value;
    } COIL_STRUCT;

    USERDEFINEDTYPE coil_type;
    initUserDefinedType(&coil_type);

    {
        strcpy(coil_type.name, "COIL_STRUCT");
        strcpy(coil_type.source, "paramsdb");
        coil_type.ref_id = 0;
        coil_type.imagecount = 0;
        coil_type.image = NULL;
        coil_type.size = sizeof(COIL_STRUCT);
        coil_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&coil_type, field);

        defineField(&field, "upper_lower", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&coil_type, field);

        defineField(&field, "value", "string array structure element", &offset, SCALARDOUBLE);
        addCompoundField(&coil_type, field);

        addUserDefinedType(userdefinedtypelist, coil_type);
    }

    typedef struct SubtypeStruct {
        char* name;
        int num_coils;
        COIL_STRUCT* coils;
    } SUBTYPE_STRUCT;

    USERDEFINEDTYPE subtype_type;
    initUserDefinedType(&subtype_type);

    {
        strcpy(subtype_type.name, "SUBTYPE_STRUCT");
        strcpy(subtype_type.source, "paramsdb");
        subtype_type.ref_id = 0;
        subtype_type.imagecount = 0;
        subtype_type.image = NULL;
        subtype_type.size = sizeof(SUBTYPE_STRUCT);
        subtype_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&subtype_type, field);

        defineField(&field, "num_coils", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&subtype_type, field);

        defineCompoundField(&field, "COIL_STRUCT", "coils", "COIL_STRUCT array element",
                            offsetof(SUBTYPE_STRUCT, coils), sizeof(COIL_STRUCT*));
        addCompoundField(&subtype_type, field);

        addUserDefinedType(userdefinedtypelist, subtype_type);
    }

    typedef struct SystemStruct {
        char* name;
        int num_subtypes;
        SUBTYPE_STRUCT* subtypes;
    } SYSTEM_STRUCT;

    USERDEFINEDTYPE system_type;
    initUserDefinedType(&system_type);

    {
        strcpy(system_type.name, "SYSTEM_STRUCT");
        strcpy(system_type.source, "paramsdb");
        system_type.ref_id = 0;
        system_type.imagecount = 0;
        system_type.image = NULL;
        system_type.size = sizeof(SYSTEM_STRUCT);
        system_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&system_type, field);

        defineField(&field, "num_subtypes", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&system_type, field);

        defineCompoundField(&field, "SUBTYPE_STRUCT", "subtypes", "SUBTYPE_STRUCT array element",
                            offsetof(SYSTEM_STRUCT, subtypes), sizeof(SUBTYPE_STRUCT*));
        addCompoundField(&system_type, field);

        addUserDefinedType(userdefinedtypelist, system_type);
    }

    // Create Data

    SYSTEM_STRUCT* system_struct = (SYSTEM_STRUCT*)malloc(sizeof(SYSTEM_STRUCT));
    addMalloc(system_struct, 1, sizeof(SYSTEM_STRUCT), "SYSTEM_STRUCT");

    system_struct->name = strdup(system);
    addMalloc(system_struct->name, 1, (strlen(system) + 1) * sizeof(char), "char");

    system_struct->num_subtypes = 0;
    system_struct->subtypes = NULL;

    int i;
    for (i = 0; i < nrows; i++) {

        const char* db_subtype = PQgetvalue(res, i, 0);
        const char* db_coil = PQgetvalue(res, i, 1);
        const char* db_upper_lower = PQgetvalue(res, i, 2);
        double db_value = atof(PQgetvalue(res, i, 3));

        IDAM_LOGF(LOG_DEBUG, "query row: '%s' '%s' '%s' %f\n", db_subtype, db_coil, db_upper_lower, db_value);

        int subtype_idx = 0;
        for (; subtype_idx < system_struct->num_subtypes; ++subtype_idx) {
            if (STR_EQUALS(system_struct->subtypes[subtype_idx].name, db_subtype)) {
                break;
            }
        }

        // subtype not found
        if (subtype_idx == system_struct->num_subtypes) {
            ++system_struct->num_subtypes;
            system_struct->subtypes = realloc(system_struct->subtypes, system_struct->num_subtypes * sizeof(SUBTYPE_STRUCT));

            SUBTYPE_STRUCT* subtype_struct = &system_struct->subtypes[subtype_idx];

            subtype_struct->name = strdup(db_subtype);
            addMalloc(subtype_struct->name, 1, (strlen(db_subtype) + 1) * sizeof(char), "char");

            subtype_struct->num_coils = 0;
            subtype_struct->coils = NULL;
        }

        SUBTYPE_STRUCT* subtype_struct = &system_struct->subtypes[subtype_idx];

        ++subtype_struct->num_coils;
        subtype_struct->coils = realloc(subtype_struct->coils, subtype_struct->num_coils * sizeof(COIL_STRUCT));

        int coil_idx = subtype_struct->num_coils - 1;
        COIL_STRUCT* coil_struct = &subtype_struct->coils[coil_idx];

        coil_struct->name = strdup(db_coil);
        addMalloc(coil_struct->name, 1, (strlen(db_coil) + 1) * sizeof(char), "char");

        coil_struct->upper_lower = strdup(db_upper_lower);
        addMalloc(coil_struct->upper_lower, 1, (strlen(db_upper_lower) + 1) * sizeof(char), "char");

        coil_struct->value = db_value;
    }

    addMalloc(system_struct->subtypes, system_struct->num_subtypes, sizeof(SUBTYPE_STRUCT), "SUBTYPE_STRUCT");
    for (i = 0; i < system_struct->num_subtypes; ++i) {
        SUBTYPE_STRUCT* subtype_struct = &system_struct->subtypes[i];
        addMalloc(subtype_struct->coils, subtype_struct->num_coils, sizeof(COIL_STRUCT), "COIL_STRUCT");
    }

    PQclear(res);

    // Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)system_struct;

    strcpy(data_block->data_desc, "Active Limits");
    strcpy(data_block->data_label, "Active Limits");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("SYSTEM_STRUCT", 0);

    return 0;
}

//
//call: getForceCoefficients([coil=?, [upper_lower=?]])
//returns:
//  [ (P1, value), (Pc, value), ... ]
int do_getForceCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    const char* coil = NULL;
    const char* upper_lower = NULL;

    bool is_coil = FIND_STRING_VALUE(request_block->nameValueList, coil);
    bool is_upper_lower = FIND_STRING_VALUE(request_block->nameValueList, upper_lower);

    PGresult* res = NULL;

    if (is_coil && is_upper_lower) {
        const char* params[2];
        params[0] = coil;
        params[1] = upper_lower;

        res = PQexecParams(conn,
                           "SELECT c.name, cm.upper_lower_id, c2.name, cm.force_coefficient"
                                   " FROM CoilMatrix AS cm"
                                   " JOIN Coil AS c ON cm.coil_id = c.id"
                                   " JOIN Coil AS c2 on cm.coil_driven_id = c2.id"
                                   " WHERE c.name = $1"
                                   "   AND upper_lower_id = $2",
                           2, NULL, params, NULL, NULL, 0);
    } else if (is_coil) {
        const char* params[1];
        params[0] = coil;

        res = PQexecParams(conn,
                           "SELECT c.name, cm.upper_lower_id, c2.name, cm.force_coefficient"
                                   " FROM CoilMatrix AS cm"
                                   " JOIN Coil AS c ON cm.coil_id = c.id"
                                   " JOIN Coil AS c2 on cm.coil_driven_id = c2.id"
                                   " WHERE c.name = $1",
                           1, NULL, params, NULL, NULL, 0);
    } else if (is_upper_lower) {
        const char* params[1];
        params[0] = upper_lower;

        res = PQexecParams(conn,
                           "SELECT c.name, cm.upper_lower_id, c2.name, cm.force_coefficient"
                                   " FROM CoilMatrix AS cm"
                                   " JOIN Coil AS c ON cm.coil_id = c.id"
                                   " JOIN Coil AS c2 on cm.coil_driven_id = c2.id"
                                   " WHERE upper_lower_id = $1",
                           1, NULL, params, NULL, NULL, 0);
    } else {
        res = PQexec(conn,
                   "SELECT c.name, cm.upper_lower_id, c2.name, cm.force_coefficient"
                           " FROM CoilMatrix AS cm"
                           " JOIN Coil AS c ON cm.coil_id = c.id"
                           " JOIN Coil AS c2 on cm.coil_driven_id = c2.id");
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    typedef struct ForceCoefficientsStruct {
        char* driven_coil;
        double value;
    } FORCECOEFFICIENTS_STRUCT;

    USERDEFINEDTYPE force_coeff_type;
    initUserDefinedType(&force_coeff_type);

    {
        strcpy(force_coeff_type.name, "FORCECOEFFICIENTS_STRUCT");
        strcpy(force_coeff_type.source, "paramsdb");
        force_coeff_type.ref_id = 0;
        force_coeff_type.imagecount = 0;
        force_coeff_type.image = NULL;
        force_coeff_type.size = sizeof(FORCECOEFFICIENTS_STRUCT);
        force_coeff_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "driven_coil", "string scalar structure element", &offset, SCALARSTRING);
        addCompoundField(&force_coeff_type, field);

        defineField(&field, "value", "double scalar structure element", &offset, SCALARDOUBLE);
        addCompoundField(&force_coeff_type, field);

        addUserDefinedType(userdefinedtypelist, force_coeff_type);
    }

    typedef struct UpperLowerStruct {
        char* name;
        int num_force_coeffs;
        FORCECOEFFICIENTS_STRUCT* force_coeffs;
    } UPPER_LOWER_STRUCT;

    USERDEFINEDTYPE upper_lower_type;
    initUserDefinedType(&upper_lower_type);

    {
        strcpy(upper_lower_type.name, "UPPER_LOWER_STRUCT");
        strcpy(upper_lower_type.source, "paramsdb");
        upper_lower_type.ref_id = 0;
        upper_lower_type.imagecount = 0;
        upper_lower_type.image = NULL;
        upper_lower_type.size = sizeof(UPPER_LOWER_STRUCT);
        upper_lower_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&upper_lower_type, field);

        defineField(&field, "num_force_coeffs", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&upper_lower_type, field);

        defineCompoundField(&field, "FORCECOEFFICIENTS_STRUCT", "force_coeffs", "FORCECOEFFICIENTS_STRUCT array element",
                            offsetof(UPPER_LOWER_STRUCT, force_coeffs), sizeof(FORCECOEFFICIENTS_STRUCT*));
        addCompoundField(&upper_lower_type, field);

        addUserDefinedType(userdefinedtypelist, upper_lower_type);
    }

    typedef struct CoilStruct {
        char* name;
        int num_upper_lowers;
        UPPER_LOWER_STRUCT* upper_lowers;
    } COIL_STRUCT;

    USERDEFINEDTYPE coil_type;
    initUserDefinedType(&coil_type);

    {
        strcpy(coil_type.name, "COIL_STRUCT");
        strcpy(coil_type.source, "paramsdb");
        coil_type.ref_id = 0;
        coil_type.imagecount = 0;
        coil_type.image = NULL;
        coil_type.size = sizeof(COIL_STRUCT);
        coil_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "string array structure element", &offset, SCALARSTRING);
        addCompoundField(&coil_type, field);

        defineField(&field, "num_upper_lowers", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&coil_type, field);

        defineCompoundField(&field, "UPPER_LOWER_STRUCT", "upper_lowers", "UPPER_LOWER_STRUCT array element",
                            offsetof(COIL_STRUCT, upper_lowers), sizeof(UPPER_LOWER_STRUCT*));
        addCompoundField(&coil_type, field);

        addUserDefinedType(userdefinedtypelist, coil_type);
    }

//    COIL_STRUCT* coil_struct = (COIL_STRUCT*)malloc(sizeof(COIL_STRUCT));
//    addMalloc((void*)coil_struct, 1, sizeof(COIL_STRUCT), "COIL_STRUCT");
//
//    coil_struct->name = strdup(coil);
//    addMalloc(coil_struct->name, 1, (strlen(coil) + 1) * sizeof(char), "char");
//
//    coil_struct->num_upper_lowers = 0;
//    coil_struct->upper_lowers = NULL;

    int num_coils = 0;
    COIL_STRUCT* coils = NULL;

    IDAM_LOGF(LOG_DEBUG, "num rows: %d\n", nrows);

    int i;
    for (i = 0; i < nrows; i++) {
        const char* db_coil = PQgetvalue(res, i, 0);
        const char* db_upper_lower = PQgetvalue(res, i, 1);
        const char* db_driven_coil = PQgetvalue(res, i, 2);
        double db_value = atof(PQgetvalue(res, i, 3));

        IDAM_LOGF(LOG_DEBUG, "query row: '%s' '%s' '%s' %f\n", db_coil, db_upper_lower, db_driven_coil, db_value);

        int coil_idx = 0;
        for (; coil_idx < num_coils; ++coil_idx) {
            if (STR_EQUALS(coils[coil_idx].name, db_coil)) {
                break;
            }
        }

        // coil not found
        if (coil_idx == num_coils) {
            ++num_coils;
            coils = realloc(coils, num_coils * sizeof(UPPER_LOWER_STRUCT));

            COIL_STRUCT* coil_struct = &coils[coil_idx];

            coil_struct->name = strdup(db_coil);
            addMalloc(coil_struct->name, 1, (strlen(db_coil) + 1) * sizeof(char), "char");

            coil_struct->num_upper_lowers = 0;
            coil_struct->upper_lowers = NULL;
        }

        COIL_STRUCT* coil_struct = &coils[coil_idx];

        int upper_lower_idx = 0;
        for (; upper_lower_idx < coil_struct->num_upper_lowers; ++upper_lower_idx) {
            if (STR_EQUALS(coil_struct->upper_lowers[upper_lower_idx].name, db_upper_lower)) {
                break;
            }
        }

        // upper_lower not found
        if (upper_lower_idx == coil_struct->num_upper_lowers) {
            ++coil_struct->num_upper_lowers;
            coil_struct->upper_lowers = realloc(coil_struct->upper_lowers, coil_struct->num_upper_lowers * sizeof(UPPER_LOWER_STRUCT));

            UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[upper_lower_idx];

            upper_lower_struct->name = strdup(db_upper_lower);
            addMalloc(upper_lower_struct->name, 1, (strlen(db_upper_lower) + 1) * sizeof(char), "char");

            upper_lower_struct->num_force_coeffs = 0;
            upper_lower_struct->force_coeffs = NULL;
        }

        UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[upper_lower_idx];

        ++upper_lower_struct->num_force_coeffs;
        upper_lower_struct->force_coeffs = realloc(upper_lower_struct->force_coeffs,
                                                   upper_lower_struct->num_force_coeffs * sizeof(FORCECOEFFICIENTS_STRUCT));

        int force_coeff_idx = upper_lower_struct->num_force_coeffs - 1;
        FORCECOEFFICIENTS_STRUCT* force_coeff_struct = &upper_lower_struct->force_coeffs[force_coeff_idx];

        force_coeff_struct->driven_coil = strdup(db_driven_coil);
        addMalloc(force_coeff_struct->driven_coil, 1, (strlen(db_driven_coil) + 1) * sizeof(char), "char");

        force_coeff_struct->value = db_value;
    }

    int shape[1] = {};
    shape[0] = num_coils;
    addMalloc2(coils, num_coils, sizeof(COIL_STRUCT), "COIL_STRUCT", 1, shape);

    for (i = 0; i < num_coils; ++i) {
        COIL_STRUCT* coil_struct = &coils[i];
        addMalloc(coil_struct->upper_lowers, coil_struct->num_upper_lowers, sizeof(UPPER_LOWER_STRUCT), "UPPER_LOWER_STRUCT");
        for (i = 0; i < coil_struct->num_upper_lowers; ++i) {
            UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[i];
            addMalloc(upper_lower_struct->force_coeffs, upper_lower_struct->num_force_coeffs, sizeof(FORCECOEFFICIENTS_STRUCT), "FORCECOEFFICIENTS_STRUCT");
        }

    }

    PQclear(res);

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)coils;

    strcpy(data_block->data_desc, "Force Coefficients");
    strcpy(data_block->data_label, "Force Coefficients");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("COIL_STRUCT", 0);

    return 0;
}

//call: getFilterCoefficients([filter=?])
//returns:
//  [ (0, value), (1, value), ..., (64, value) ]
int do_getFilterCoefficients(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    int filter = 0;

    bool is_filter = FIND_INT_VALUE(request_block->nameValueList, filter);

    typedef struct CoefficientsStruct {
        int coefficient;
        double value;
    } COEFFICIENTS_STRUCT;

    USERDEFINEDTYPE coeff_type;
    initUserDefinedType(&coeff_type);

    {
        strcpy(coeff_type.name, "COEFFICIENTS_STRUCT");
        strcpy(coeff_type.source, "paramsdb");
        coeff_type.ref_id = 0;
        coeff_type.imagecount = 0;
        coeff_type.image = NULL;
        coeff_type.size = sizeof(COEFFICIENTS_STRUCT);
        coeff_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "coefficient", "integer scalar structure element", &offset, SCALARINT);
        addCompoundField(&coeff_type, field);

        defineField(&field, "value", "double scalar structure element", &offset, SCALARDOUBLE);
        addCompoundField(&coeff_type, field);

        addUserDefinedType(userdefinedtypelist, coeff_type);
    }

    typedef struct FilterStruct {
        int filter;
        int num_coefficients;
        COEFFICIENTS_STRUCT* coefficients;
    } FILTER_STRUCT;

    USERDEFINEDTYPE filter_type;
    initUserDefinedType(&filter_type);

    {
        strcpy(filter_type.name, "FILTER_STRUCT");
        strcpy(filter_type.source, "paramsdb");
        filter_type.ref_id = 0;
        filter_type.imagecount = 0;
        filter_type.image = NULL;
        filter_type.size = sizeof(FILTER_STRUCT);
        filter_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "filter", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&filter_type, field);

        defineField(&field, "num_coefficients", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&filter_type, field);

        defineCompoundField(&field, "COEFFICIENTS_STRUCT", "coefficients", "COEFFICIENTS_STRUCT array element",
                            offsetof(FILTER_STRUCT, coefficients), sizeof(COEFFICIENTS_STRUCT*));
        addCompoundField(&filter_type, field);

        addUserDefinedType(userdefinedtypelist, filter_type);
    }

    PGresult* res = NULL;

    if (is_filter) {
        const char* params[1];
        params[0] = FormatString("%d", filter);

        res = PQexecParams(conn,
                           "SELECT filter, coefficient, value FROM FilterCoefficient WHERE filter = $1",
                           1, NULL, params, NULL, NULL, 0);

        free((void*)params[0]);
    } else {
        res = PQexec(conn, "SELECT filter, coefficient, value FROM FilterCoefficient");
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    int num_filters = 0;
    FILTER_STRUCT* filters = NULL;

    int i;
    for (i = 0; i < nrows; i++) {
        int db_filter = atoi(PQgetvalue(res, i, 0));
        int db_coefficient = atoi(PQgetvalue(res, i, 1));
        double db_value = atof(PQgetvalue(res, i, 2));

        IDAM_LOGF(LOG_DEBUG, "query row: %d %d %f\n", db_filter, db_coefficient, db_value);

        int filter_idx = 0;
        for (; filter_idx < num_filters; ++filter_idx) {
            if (filters[filter_idx].filter == db_filter) {
                break;
            }
        }

        // filter not found
        if (filter_idx == num_filters) {
            ++num_filters;
            filters = realloc(filters, num_filters * sizeof(FILTER_STRUCT));

            FILTER_STRUCT* filters_struct = &filters[filter_idx];

            filters_struct->filter = db_filter;
            filters_struct->num_coefficients = 0;
            filters_struct->coefficients = NULL;
        }

        FILTER_STRUCT* filters_struct = &filters[filter_idx];

        int coeffiecient_idx = 0;
        for (; coeffiecient_idx < filters_struct->num_coefficients; ++coeffiecient_idx) {
            if (filters_struct->coefficients[coeffiecient_idx].coefficient == db_coefficient) {
                break;
            }
        }

        // coefficient not found
        if (coeffiecient_idx == filters_struct->num_coefficients) {
            ++filters_struct->num_coefficients;
            filters_struct->coefficients = realloc(filters_struct->coefficients,
                                                   filters_struct->num_coefficients * sizeof(COEFFICIENTS_STRUCT));

            COEFFICIENTS_STRUCT* coefficients_struct = &filters_struct->coefficients[coeffiecient_idx];

            coefficients_struct->coefficient = db_coefficient;
            coefficients_struct->value = db_value;
        }
    }

    PQclear(res);

    int shape[1] = {};
    shape[0] = num_filters;
    addMalloc2(filters, num_filters, sizeof(FILTER_STRUCT), "FILTER_STRUCT", 1, shape);

    for (i = 0; i < num_filters; ++i) {
        FILTER_STRUCT* filter_struct = &filters[i];
        addMalloc(filter_struct->coefficients, filter_struct->num_coefficients, sizeof(COEFFICIENTS_STRUCT),
                  "COEFFICIENTS_STRUCT");
    }

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)filters;

    strcpy(data_block->data_desc, "Filter Coefficients");
    strcpy(data_block->data_label, "Filter Coefficients");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("FILTER_STRUCT", 0);

    return 0;
}

//call: getBoardCalibrations([board=?])
//returns:
//  [ (0, gain, offset), (1, gain, offset), ..., (31, gain, offset) ]
int do_getBoardCalibrations(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* conn)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    int board = 0;
    bool is_board = FIND_INT_VALUE(request_block->nameValueList, board);

    typedef struct ChannelStruct {
        int channel;
        double gain;
        double cal_offset;
    } CHANNEL_STRUCT;

    USERDEFINEDTYPE channel_type;
    initUserDefinedType(&channel_type);

    {
        strcpy(channel_type.name, "CHANNEL_STRUCT");
        strcpy(channel_type.source, "paramsdb");
        channel_type.ref_id = 0;
        channel_type.imagecount = 0;
        channel_type.image = NULL;
        channel_type.size = sizeof(CHANNEL_STRUCT);
        channel_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "channel", "integer scalar structure element", &offset, SCALARINT);
        addCompoundField(&channel_type, field);

        defineField(&field, "gain", "double scalar structure element", &offset, SCALARDOUBLE);
        addCompoundField(&channel_type, field);

        defineField(&field, "cal_offset", "double scalar structure element", &offset, SCALARDOUBLE);
        addCompoundField(&channel_type, field);

        addUserDefinedType(userdefinedtypelist, channel_type);
    }

    typedef struct BoardStruct {
        int board;
        int num_channels;
        CHANNEL_STRUCT* channels;
    } BOARD_STRUCT;

    USERDEFINEDTYPE board_type;
    initUserDefinedType(&board_type);

    {
        strcpy(board_type.name, "BOARD_STRUCT");
        strcpy(board_type.source, "paramsdb");
        board_type.ref_id = 0;
        board_type.imagecount = 0;
        board_type.image = NULL;
        board_type.size = sizeof(BOARD_STRUCT);
        board_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "board", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&board_type, field);

        defineField(&field, "num_channels", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&board_type, field);

        defineCompoundField(&field, "CHANNEL_STRUCT", "channels", "CHANNEL_STRUCT array element",
                            offsetof(BOARD_STRUCT, channels), sizeof(CHANNEL_STRUCT*));
        addCompoundField(&board_type, field);

        addUserDefinedType(userdefinedtypelist, board_type);
    }

    PGresult* res = NULL;

    if (is_board) {
        const char* params[1];
        params[0] = FormatString("%d", board);

        res = PQexecParams(conn,
                           "SELECT board, channel, gain, cal_offset FROM BoardCalibration WHERE board = $1",
                           1, NULL, params, NULL, NULL, 0);

        free((void*)params[0]);
    } else {
        res = PQexec(conn, "SELECT board, channel, gain, cal_offset FROM BoardCalibration");
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        RAISE_PLUGIN_ERROR_F("DB query failed", "DB query failed: %s", PQresultErrorMessage(res));
    }

    if (PQntuples(res) == 0) {
        RAISE_PLUGIN_ERROR("DB query returned no rows");
    }

    int nrows = PQntuples(res);

    int num_boards = 0;
    BOARD_STRUCT* boards = NULL;

    int i;
    for (i = 0; i < nrows; i++) {
        int db_board = atoi(PQgetvalue(res, i, 0));
        int db_channel = atoi(PQgetvalue(res, i, 1));
        double db_gain = atof(PQgetvalue(res, i, 2));
        double db_cal_offset = atof(PQgetvalue(res, i, 3));

        IDAM_LOGF(LOG_DEBUG, "query row: %d %d %f %f\n", db_board, db_channel, db_gain, db_cal_offset);

        int board_idx = 0;
        for (; board_idx < num_boards; ++board_idx) {
            if (boards[board_idx].board == db_board) {
                break;
            }
        }

        // filter not found
        if (board_idx == num_boards) {
            ++num_boards;
            boards = realloc(boards, num_boards * sizeof(BOARD_STRUCT));

            BOARD_STRUCT* board_struct = &boards[board_idx];

            board_struct->board = db_board;
            board_struct->num_channels = 0;
            board_struct->channels = NULL;
        }

        BOARD_STRUCT* board_struct = &boards[board_idx];

        int channel_idx = 0;
        for (; channel_idx < board_struct->num_channels; ++channel_idx) {
            if (board_struct->channels[channel_idx].channel == db_channel) {
                break;
            }
        }

        // coefficient not found
        if (channel_idx == board_struct->num_channels) {
            ++board_struct->num_channels;
            board_struct->channels = realloc(board_struct->channels,
                                             board_struct->num_channels * sizeof(CHANNEL_STRUCT));

            CHANNEL_STRUCT* channel_struct = &board_struct->channels[channel_idx];

            channel_struct->channel = db_channel;
            channel_struct->gain = db_gain;
            channel_struct->cal_offset = db_cal_offset;
        }
    }

    PQclear(res);

    int shape[1] = {};
    shape[0] = num_boards;
    addMalloc2(boards, num_boards, sizeof(BOARD_STRUCT), "BOARD_STRUCT", 1, shape);

    for (i = 0; i < num_boards; ++i) {
        BOARD_STRUCT* board_struct = &boards[i];
        addMalloc(board_struct->channels, board_struct->num_channels, sizeof(CHANNEL_STRUCT), "CHANNEL_STRUCT");
    }

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)boards;

    strcpy(data_block->data_desc, "Board Calibrations");
    strcpy(data_block->data_label, "Board Calibrations");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("BOARD_STRUCT", 0);

    return 0;
}

//call: getCoilParameters([coil=?], [upper_lower=?], [parameter=?])
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

    bool is_coil = FIND_STRING_VALUE(request_block->nameValueList, coil);
    bool is_upper_lower = FIND_STRING_VALUE(request_block->nameValueList, upper_lower);
    bool is_parameter = FIND_STRING_VALUE(request_block->nameValueList, parameter);

    typedef struct ParameterStruct {
        const char* parameter;
        double value;
    } PARAMETER_STRUCT;

    USERDEFINEDTYPE parameter_type;
    initUserDefinedType(&parameter_type);

    {
        strcpy(parameter_type.name, "PARAMETER_STRUCT");
        strcpy(parameter_type.source, "paramsdb");
        parameter_type.ref_id = 0;
        parameter_type.imagecount = 0;
        parameter_type.image = NULL;
        parameter_type.size = sizeof(PARAMETER_STRUCT);
        parameter_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "parameter", "string scalar structure element", &offset, SCALARSTRING);
        addCompoundField(&parameter_type, field);

        defineField(&field, "value", "double scalar structure element", &offset, SCALARDOUBLE);
        addCompoundField(&parameter_type, field);

        addUserDefinedType(userdefinedtypelist, parameter_type);
    }

    typedef struct UpperLowerStruct {
        const char* upper_lower;
        int num_parameters;
        PARAMETER_STRUCT* parameters;
    } UPPER_LOWER_STRUCT;

    USERDEFINEDTYPE upper_lower_type;
    initUserDefinedType(&upper_lower_type);

    {
        strcpy(upper_lower_type.name, "UPPER_LOWER_STRUCT");
        strcpy(upper_lower_type.source, "paramsdb");
        upper_lower_type.ref_id = 0;
        upper_lower_type.imagecount = 0;
        upper_lower_type.image = NULL;
        upper_lower_type.size = sizeof(UPPER_LOWER_STRUCT);
        upper_lower_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "upper_lower", "scalar string structure element", &offset, SCALARSTRING);
        addCompoundField(&upper_lower_type, field);

        defineField(&field, "num_parameters", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&upper_lower_type, field);

        defineCompoundField(&field, "PARAMETER_STRUCT", "parameters", "PARAMETER_STRUCT array element",
                            offsetof(UPPER_LOWER_STRUCT, parameters), sizeof(PARAMETER_STRUCT*));
        addCompoundField(&upper_lower_type, field);

        addUserDefinedType(userdefinedtypelist, upper_lower_type);
    }

    typedef struct CoilStruct {
        const char* name;
        int num_upper_lowers;
        UPPER_LOWER_STRUCT* upper_lowers;
    } COIL_STRUCT;

    USERDEFINEDTYPE coil_type;
    initUserDefinedType(&coil_type);

    {
        strcpy(coil_type.name, "COIL_STRUCT");
        strcpy(coil_type.source, "paramsdb");
        coil_type.ref_id = 0;
        coil_type.imagecount = 0;
        coil_type.image = NULL;
        coil_type.size = sizeof(COIL_STRUCT);
        coil_type.idamclass = TYPE_COMPOUND;

        COMPOUNDFIELD field;
        int offset = 0;

        defineField(&field, "name", "scalar int structure element", &offset, SCALARSTRING);
        addCompoundField(&coil_type, field);

        defineField(&field, "num_upper_lowers", "scalar int structure element", &offset, SCALARINT);
        addCompoundField(&coil_type, field);

        defineCompoundField(&field, "UPPER_LOWER_STRUCT", "upper_lowers", "UPPER_LOWER_STRUCT array element",
                            offsetof(COIL_STRUCT, upper_lowers), sizeof(UPPER_LOWER_STRUCT*));
        addCompoundField(&coil_type, field);

        addUserDefinedType(userdefinedtypelist, coil_type);
    }

    PGresult* res = NULL;

#define COIL_PARAMETERS_SQL "SELECT c.name, cp.upper_lower_id, p.name, cp.value" \
    " FROM CoilParameters AS cp JOIN Coil AS c ON cp.coil_id = c.id JOIN Parameter AS p ON cp.parameter_id = p.id"

    if (is_coil && is_upper_lower && is_parameter) {
        const char* params[3];
        params[0] = coil;
        params[1] = upper_lower;
        params[2] = parameter;

        res = PQexecParams(conn,
                           COIL_PARAMETERS_SQL
                                   " WHERE c.name = $1"
                                   "   AND cp.upper_lower_id = $2"
                                   "   AND p.name = $3",
                           3, NULL, params, NULL, NULL, 0
        );
    } else if (is_coil && is_upper_lower) {
        const char* params[2];
        params[0] = coil;
        params[1] = upper_lower;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE c.name = $1 AND cp.upper_lower_id = $2",
                           2, NULL, params, NULL, NULL, 0);
    } else if (is_coil && is_parameter) {
        const char* params[2];
        params[0] = coil;
        params[1] = parameter;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE c.name = $1 AND p.name = $2",
                           2, NULL, params, NULL, NULL, 0);
    } else if (is_upper_lower && is_parameter) {
        const char* params[2];
        params[0] = upper_lower;
        params[1] = parameter;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE cp.upper_lower_id = $1 AND p.name = $2",
                           2, NULL, params, NULL, NULL, 0);
    } else if (is_coil) {
        const char* params[1];
        params[0] = coil;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE c.name = $1", 1, NULL, params, NULL, NULL, 0);
    } else if (is_upper_lower) {
        const char* params[1];
        params[0] = upper_lower;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE cp.upper_lower_id = $1", 1, NULL, params, NULL, NULL, 0);
    } else if (is_parameter) {
        const char* params[1];
        params[0] = parameter;

        res = PQexecParams(conn, COIL_PARAMETERS_SQL " WHERE p.name = $1", 1, NULL, params, NULL, NULL, 0);
    } else {
        res = PQexec(conn, COIL_PARAMETERS_SQL);
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

    int num_coils = 0;
    COIL_STRUCT* coils = NULL;

    int i;
    for (i = 0; i < nrows; i++) {
        const char* db_coil = PQgetvalue(res, i, 0);
        const char* db_upper_lower = PQgetvalue(res, i, 1);
        const char* db_parameter = PQgetvalue(res, i, 2);
        double db_value = atof(PQgetvalue(res, i, 3));

        IDAM_LOGF(LOG_DEBUG, "query row: '%s' '%s' '%s' %f\n", db_coil, db_upper_lower, db_parameter, db_value);

        int coil_idx = 0;
        for (; coil_idx < num_coils; ++coil_idx) {
            if (STR_EQUALS(coils[coil_idx].name, db_coil)) {
                break;
            }
        }

        // coil not found
        if (coil_idx == num_coils) {
            ++num_coils;
            coils = realloc(coils, num_coils * sizeof(COIL_STRUCT));

            COIL_STRUCT* coil_struct = &coils[coil_idx];

            coil_struct->name = strdup(db_coil);
            addMalloc((void*)coil_struct->name, 1, (strlen(coil_struct->name) + 1) * sizeof(char), "char");

            coil_struct->num_upper_lowers = 0;
            coil_struct->upper_lowers = NULL;
        }

        COIL_STRUCT* coil_struct = &coils[coil_idx];

        int upper_lower_idx = 0;
        for (; upper_lower_idx < coil_struct->num_upper_lowers; ++upper_lower_idx) {
            if (STR_EQUALS(coil_struct->upper_lowers[upper_lower_idx].upper_lower, db_upper_lower)) {
                break;
            }
        }

        // upper_lower not found
        if (upper_lower_idx == coil_struct->num_upper_lowers) {
            ++coil_struct->num_upper_lowers;
            coil_struct->upper_lowers = realloc(coil_struct->upper_lowers,
                                                coil_struct->num_upper_lowers * sizeof(UPPER_LOWER_STRUCT));

            UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[upper_lower_idx];

            upper_lower_struct->upper_lower = strdup(db_upper_lower);
            addMalloc((void*)upper_lower_struct->upper_lower, 1, (strlen(db_upper_lower) + 1) * sizeof(char), "char");

            upper_lower_struct->num_parameters = 0;
            upper_lower_struct->parameters = NULL;
        }

        UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[upper_lower_idx];

        int parameter_idx = 0;
        for (; parameter_idx < upper_lower_struct->num_parameters; ++parameter_idx) {
            if (STR_EQUALS(upper_lower_struct->parameters[parameter_idx].parameter, db_parameter)) {
                break;
            }
        }

        // parameter not found
        if (parameter_idx == upper_lower_struct->num_parameters) {
            ++upper_lower_struct->num_parameters;
            upper_lower_struct->parameters = realloc(upper_lower_struct->parameters,
                                                     upper_lower_struct->num_parameters * sizeof(UPPER_LOWER_STRUCT));

            PARAMETER_STRUCT* parameter_struct = &upper_lower_struct->parameters[parameter_idx];

            parameter_struct->parameter = strdup(db_parameter);
            addMalloc((void*)parameter_struct->parameter, 1, (strlen(db_parameter) + 1) * sizeof(char), "char");

            parameter_struct->value = db_value;
        }
    }

    PQclear(res);

    int shape[1] = {};
    shape[0] = num_coils;
    addMalloc2(coils, num_coils, sizeof(COIL_STRUCT), "COIL_STRUCT", 1, shape);

    for (i = 0; i < num_coils; ++i) {
        COIL_STRUCT* coil_struct = &coils[i];
        addMalloc(coil_struct->upper_lowers, coil_struct->num_upper_lowers, sizeof(UPPER_LOWER_STRUCT),
                  "UPPER_LOWER_STRUCT");

        int j;
        for (j = 0; j < coil_struct->num_upper_lowers; ++j) {
            UPPER_LOWER_STRUCT* upper_lower_struct = &coil_struct->upper_lowers[j];
            addMalloc(upper_lower_struct->parameters, upper_lower_struct->num_parameters, sizeof(PARAMETER_STRUCT),
                      "PARAMETER_STRUCT");
        }
    }

    initDataBlock(data_block);

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)coils;

    strcpy(data_block->data_desc, "Coil Parameters");
    strcpy(data_block->data_label, "Coil Parameters");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType("COIL_STRUCT", 0);

    return 0;
}
