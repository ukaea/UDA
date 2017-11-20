/*
 *  xpadtree.c
 *
 *  Created on: 24 Feb 2016
 *      Author: lkogan
 *  
 *  Reads signal tag and tree information for use in eg. XPAD.
 *                    
 *  Methods that are implemented
 */

#include <strings.h>
#include <stdlib.h>

#include <clientserver/errorLog.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <logging/logging.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>

#include "xpadtree.h"

static PGconn* open_connection(const char* host, char* port, const char* dbname, const char* user)
{

//-------------------------------------------------------------
// Debug Trace Queries

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection: host %s\n", host);
    UDA_LOG(UDA_LOG_DEBUG, "                port %s\n", port);
    UDA_LOG(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    UDA_LOG(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    PGconn* DBConnect = NULL;

    if ((DBConnect = PQsetdbLogin(host, port, NULL, NULL, dbname, user, NULL)) == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "SQL Server Connect Error\n");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        UDA_LOG(UDA_LOG_DEBUG, "Bad SQL Server Connect Status\n");
        PQfinish(DBConnect);
        return NULL;
    }

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return DBConnect;
}

int idamXpadTree(IDAM_PLUGIN_INTERFACE* idam_plugin_interface) {

    static int init = 0;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK *request_block = idam_plugin_interface->request_block;

    static PGconn *DBConnectIdam = NULL;
    static PGconn *DBConnectXpad = NULL;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!

        PQfinish(DBConnectIdam);
        DBConnectIdam = NULL;

        PQfinish(DBConnectXpad);
        DBConnectXpad = NULL;

        // Free Heap & reset counters
        init = 0;
        return 0;
    }


    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        char *db_idam_host = getenv("XPADTREE_IDAM_DB_HOST");
        char *db_idam_port_str = getenv("XPADTREE_IDAM_DB_PORT");
        char *db_idam_name = getenv("XPADTREE_IDAM_DB_NAME");
        char *db_idam_user = getenv("XPADTREE_IDAM_DB_USER");

        if (db_idam_host == NULL || db_idam_port_str == NULL || db_idam_name == NULL || db_idam_user == NULL) {
            RAISE_PLUGIN_ERROR("UDA_SQL host, port, name and user env variables were not set.\n");
        }

        DBConnectIdam = open_connection(db_idam_host, db_idam_port_str, db_idam_name, db_idam_user);

        if (DBConnectIdam == NULL) {
            RAISE_PLUGIN_ERROR("Could not open uda database connection\n");
        }

        char *db_xpadtree_host = getenv("XPADTREE_DB_HOST");
        char *db_xpadtree_port_str = getenv("XPADTREE_DB_PORT");
        char *db_xpadtree_name = getenv("XPADTREE_DB_NAME");
        char *db_xpadtree_user = getenv("XPADTREE_DB_USER");

        if (db_xpadtree_host == NULL || db_xpadtree_port_str == NULL || db_xpadtree_name == NULL ||
            db_xpadtree_user == NULL) {
            RAISE_PLUGIN_ERROR("XPADTREE_DB host, port, name and user env variables were not set.\n");
        }

        DBConnectXpad = open_connection(db_xpadtree_host, db_xpadtree_port_str, db_xpadtree_name, db_xpadtree_user);

        if (DBConnectXpad == NULL) {
            RAISE_PLUGIN_ERROR("Could not open uda database connection\n");
        }
        init = 1;

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }

    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------
    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "getsignals")) {
        return do_xpad_signals(idam_plugin_interface, DBConnectIdam);
    } else if (STR_IEQUALS(request_block->function, "getsignaltags")) {
        return do_xpad_signal_tags(idam_plugin_interface, DBConnectXpad);
    } else if (STR_IEQUALS(request_block->function, "gettree")) {
        return do_xpad_signal_tree(idam_plugin_interface, DBConnectXpad);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface) {

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* help = "xpadtree::gettree() - Read in tree.\n";

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    initDimBlock(&data_block->dims[0]);

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "help = description of this plugin");

    data_block->data = (char*)malloc(sizeof(char) * (strlen(help) + 1));
    strcpy(data_block->data, help);

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int)strlen(help) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

int do_xpad_signals(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect) {

    // Return the following information about all signals available in IDAM:
    //  - signal alias
    //  - signal description
    //  - signal category
    //  - signal id
    //
    // This information will come directly from IDAM.
    // At the moment, it is in idam3.
    //
    // Arguments:
    //  - signal_type : ie. 'R' for raw data, 'A' for analysed, 'M' for modelling. If not given, return all.

    //////////////////////////////
    // Read in signal type
    //////////////////////////////
    char* signalType = NULL;
    int foundType = 0;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int n_args = request_block->nameValueList.pairCount;
    int i_arg;
    int err = 0;

    for (i_arg = 0; i_arg < n_args; i_arg++) {
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "signaltype")) {
            signalType = request_block->nameValueList.nameValue[i_arg].value;
            UDA_LOG(UDA_LOG_DEBUG, "Retrieving signals of type %s\n", signalType);
            foundType = 1;
        }
    }

    //////////////////////////////
    // Open the connection
    // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
    //////////////////////////////
    UDA_LOG(UDA_LOG_DEBUG, "trying to get connection\n");
    PGresult* DBQuery = NULL;

    if (DBConnect == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "Connection to idam1 database failed. %s\n");
        RAISE_PLUGIN_ERROR("Error connecting to uda db!\n")
    }

    char query[MAXSQL];

    if (foundType == 1) {
        char* signaltype_for_query = (char*)malloc((2 * strlen(signalType) + 1) * sizeof(char));
        PQescapeStringConn(DBConnect, signaltype_for_query, signalType, strlen(signalType), &err);
        sprintf(query,
                "SELECT signal_desc_id, signal_alias, description, category FROM signal_desc WHERE type='%s';",
                signaltype_for_query);
        free(signaltype_for_query);
    } else {
        sprintf(query, "SELECT signal_desc_id, signal_alias, description, category FROM signal_desc;");
    }

    UDA_LOG(UDA_LOG_DEBUG, "escape query %s\n", query);

    if (err != 0) {
        RAISE_PLUGIN_ERROR("Could not escape string\n");
    }

    // Get result
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "SELECT failed. %s\n", query);
        RAISE_PLUGIN_ERROR("SELECT failed. \n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "Database query failed.\n");
        RAISE_PLUGIN_ERROR("Database Query Failed!\n");
    }

    //Get number of rows that were returned, and allocate memory to arrays
    int nRows = PQntuples(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "query returned %d\n", nRows);

    if (nRows == 0) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "No signals with type: %s were found\n", signalType);
        RAISE_PLUGIN_ERROR("No signals were found\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "setup datastruct\n");

    // Struct to store results in
    struct DATASTRUCT {
        char** signal_alias;
        char** signal_description;
        char** signal_category;
        int* signal_id;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    DATASTRUCT* data;
    data = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data->signal_alias = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_alias, nRows, sizeof(char*), "STRING *");
    data->signal_description = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_description, nRows, sizeof(char*), "STRING *");
    data->signal_category = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_category, nRows, sizeof(char*), "STRING *");
    data->signal_id = (int*)malloc((nRows) * sizeof(int));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_id, nRows, sizeof(int), "int");

    //Get data from result
    int s_name_fnum = PQfnumber(DBQuery, "signal_alias");
    int s_desc_fnum = PQfnumber(DBQuery, "description");
    int s_cat_fnum = PQfnumber(DBQuery, "category");
    int s_id_fnum = PQfnumber(DBQuery, "signal_desc_id");

    UDA_LOG(UDA_LOG_DEBUG, "retrieve results\n");
    int i;
    int stringLength;
    for (i = 0; i < nRows; i++) {
        // Retrieve signal alias
        if (!PQgetisnull(DBQuery, i, s_name_fnum)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_name_fnum)) + 1;
            data->signal_alias[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->signal_alias[i], PQgetvalue(DBQuery, i, s_name_fnum));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_alias[i], stringLength, sizeof(char), "char");
        } else {
            data->signal_alias[i] = (char*)malloc(8 * sizeof(char));
            strcpy(data->signal_alias[i], "unknown");
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_alias[i], 8, sizeof(char), "char");
        }
        // Retrieve description, if NULL then set to an empty string
        if (!PQgetisnull(DBQuery, i, s_desc_fnum)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_desc_fnum)) + 1;
            data->signal_description[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->signal_description[i], PQgetvalue(DBQuery, i, s_desc_fnum));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_description[i], stringLength, sizeof(char), "char");
        } else {
            data->signal_description[i] = (char*)malloc(sizeof(char));
            strcpy(data->signal_description[i], "");
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_description[i], 1, sizeof(char), "char");
        }
        // Retrieve category (S or P, for secondary or primary), if NULL then set to S
        if (!PQgetisnull(DBQuery, i, s_cat_fnum)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_cat_fnum)) + 1;
            data->signal_category[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->signal_category[i], PQgetvalue(DBQuery, i, s_cat_fnum));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_category[i], stringLength, sizeof(char), "char");
        } else {
            data->signal_category[i] = (char*)malloc(2 * sizeof(char));
            strcpy(data->signal_category[i], "S");
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_category[i], 2, sizeof(char), "char");
        }
        // Retrieve signal id
        if (!PQgetisnull(DBQuery, i, s_id_fnum)) {
            data->signal_id[i] = atoi(PQgetvalue(DBQuery, i, s_id_fnum));
        } else {
            data->signal_id[i] = -1;
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Clear query\n");

    // Close db connection
    PQclear(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "define structure for datablock\n");

    // Output data block
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATASTRUCT");
    strcpy(parentTree.source, "idam3");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATASTRUCT);

    //Compound fields for datastruct
    initCompoundField(&field);
    strcpy(field.name, "signal_alias");
    defineField(&field, "signal_alias", "signal_alias", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "signal_description");
    defineField(&field, "signal_description", "signal_description", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "signal_category");
    defineField(&field, "signal_category", "signal_category", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "signal_id");
    defineField(&field, "signal_id", "signal_id", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    // Put data struct into data block for return
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "signals");
    strcpy(data_block->data_label, "signals");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "everything done\n");

    return 0;

}

int do_xpad_signal_tags(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect) {
    // Return the mapping information between the tree tags and the signals
    //  - tag id
    //  - signal alias
    //  - signal id
    //
    // Arguments:
    //  - tree_name : tree name from which to obtain mapping

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int err = 0;
    char* treename = NULL;
    int foundTreename = 0;

    int n_args = request_block->nameValueList.pairCount;
    int i_arg;

    for (i_arg = 0; i_arg < n_args; i_arg++) {
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "treename")) {
            treename = request_block->nameValueList.nameValue[i_arg].value;
            UDA_LOG(UDA_LOG_DEBUG, "xpadtree : retrieving tags for tree %s\n", treename);
            foundTreename = 1;
        }
    }

    if (foundTreename != 1) {
        RAISE_PLUGIN_ERROR("Treename must be provided.");
    }

    //////////////////////////////
    // Open the connection
    // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
    //////////////////////////////
    UDA_LOG(UDA_LOG_DEBUG, "trying to get connection\n");
    PGresult* DBQuery = NULL;

    if (DBConnect == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "Connection to xpad database failed.");
        RAISE_PLUGIN_ERROR("Error connecting to xpad db!\n");
    }

    // Get signal-tag mapping
    char* treename_for_query = (char*)malloc((2 * strlen(treename) + 1) * sizeof(char));
    PQescapeStringConn(DBConnect, treename_for_query, treename, strlen(treename), &err);

    char query[MAXSQL];
    sprintf(query,
            "SELECT t.tag_id, ts.signal_desc_id FROM tag t, signal_tags ts WHERE tree_id=(SELECT tree_id FROM tag_tree WHERE tree_name='%s') AND t.tag_id=ts.tag_id;",
            treename_for_query);
    free(treename_for_query);

    UDA_LOG(UDA_LOG_DEBUG, "QUERY IS %s\n", query);

    if (err != 0) {
        RAISE_PLUGIN_ERROR("Could not escape string\n");
    }

    // Get result
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "SELECT failed. %s\n", query);
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("SELECT failed. \n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "Database query failed.\n");
        RAISE_PLUGIN_ERROR("Database Query Failed!\n");
    }

    //Get number of rows that were returned, and allocate memory to arrays
    int nRows = PQntuples(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "query returned %d\n", nRows);

    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "No tags for tree: %s were found\n", treename);
        RAISE_PLUGIN_ERROR("No signals were found\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "setup datastruct\n");

    // Struct to store results in
    struct DATASTRUCT {
        int* tag_id;
        int* signal_id;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    DATASTRUCT* data;
    data = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data->tag_id = (int*)malloc((nRows) * sizeof(int));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->tag_id, nRows, sizeof(int), "int");
    data->signal_id = (int*)malloc((nRows) * sizeof(int));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_id, nRows, sizeof(int), "int");

    // Get data from result
    int s_tag_id_fnum = PQfnumber(DBQuery, "tag_id");
    int s_signal_id_fnum = PQfnumber(DBQuery, "signal_desc_id");

    UDA_LOG(UDA_LOG_DEBUG, "retrieve results\n");
    int i;
    for (i = 0; i < nRows; i++) {
        if (!PQgetisnull(DBQuery, i, s_tag_id_fnum)) {
            data->tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_tag_id_fnum));
        } else {
            data->tag_id[i] = -1;
        }
        // Retrieve signal id
        if (!PQgetisnull(DBQuery, i, s_signal_id_fnum)) {
            data->signal_id[i] = atoi(PQgetvalue(DBQuery, i, s_signal_id_fnum));
        } else {
            data->signal_id[i] = -1;
        }
    }

    // Close db connection
    PQclear(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "define structure for datablock\n");

    // Output data block
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATASTRUCT");
    strcpy(parentTree.source, "idam3");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATASTRUCT);


    //Compound fields for datastruct
    initCompoundField(&field);
    strcpy(field.name, "tag_id");
    defineField(&field, "tag_id", "tag_id", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "signal_id");
    defineField(&field, "signal_id", "signal_id", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    // Put data struct into data block for return
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "signaltag_map");
    strcpy(data_block->data_label, "signaltag_map");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "everything done\n");

    return 0;
}

int do_xpad_signal_tree(IDAM_PLUGIN_INTERFACE *idam_plugin_interface, PGconn* DBConnect) {
    // Return the tree structure for a tree of a given name
    //  - tag id
    //  - parent tag id
    //  - tag name
    //
    // Also return the maximum tag id for all trees
    //
    // Arguments:
    //  - tree_name : tree name for which to obtain structure
    char* treename = NULL;
    int foundTreename = 0;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int err = 0;
    int n_args = request_block->nameValueList.pairCount;
    int i_arg;

    UDA_LOG(UDA_LOG_DEBUG, "TEST SIGTREE");

    for (i_arg = 0; i_arg < n_args; i_arg++) {
        if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "treename")) {
            treename = request_block->nameValueList.nameValue[i_arg].value;
            UDA_LOG(UDA_LOG_DEBUG, "xpadtree : retrieving tags for tree %s\n", treename);
            foundTreename = 1;
        }
    }

    if (foundTreename != 1) {
        RAISE_PLUGIN_ERROR("Treename must be provided.\n");
    }

    //////////////////////////////
    // Open the connection
    // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
    //////////////////////////////
    UDA_LOG(UDA_LOG_DEBUG, "trying to get connection\n");
    PGresult* DBQuery = NULL;

    if (DBConnect == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "Connection to xpad database failed.");
        RAISE_PLUGIN_ERROR("Error connecting to xpad db!\n");
    }

    // Retrieve maximum tag id
    char* query_max = "SELECT max(tag_id) FROM tag;";

    // Get result
    if ((DBQuery = PQexec(DBConnect, query_max)) == NULL) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "SELECT failed. %s\n", query_max);
        RAISE_PLUGIN_ERROR("SELECT failed. \n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "Database query failed.\n");
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    //Get number of rows that were returned, and allocate memory to arrays
    int nRows_max = PQntuples(DBQuery);

    if (nRows_max == 0) {
        PQclear(DBQuery);
        UDA_LOG(UDA_LOG_DEBUG, "Database query failed to get max id.\n");
        RAISE_PLUGIN_ERROR("Database query failed to get max id.\n");
    }

    int max_tag_id = 0;
    int s_max_id_fnum = PQfnumber(DBQuery, "max");
    int i;

    for (i = 0; i < PQntuples(DBQuery); i++) {
        if (!PQgetisnull(DBQuery, i, s_max_id_fnum)) {
            max_tag_id = atoi(PQgetvalue(DBQuery, i, s_max_id_fnum));
        }
    }

    PQclear(DBQuery);

    // Main query to get tags
    char* treename_for_query = (char*)malloc((2 * strlen(treename) + 1) * sizeof(char));
    PQescapeStringConn(DBConnect, treename_for_query, treename, strlen(treename), &err);

    char query[MAXSQL];
    sprintf(query,
            "SELECT tag_id, tag_name, parent_tag_id FROM tag WHERE tree_id=(SELECT tree_id FROM tag_tree WHERE tree_name='%s')",
            treename_for_query);
    free(treename_for_query);

    if (err != 0) {
        RAISE_PLUGIN_ERROR("Could not escape string\n");
    }

    // Get result
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("SELECT failed. \n");
        UDA_LOG(UDA_LOG_DEBUG, "SELECT failed. %s\n", query);
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    //Get number of rows that were returned, and allocate memory to arrays
    int nRows = PQntuples(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "query returned %d\n", nRows);

    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "No tags for tree: %s were found\n", treename);
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("No signals were found\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "setup datastruct\n");

    // Struct to store results in
    struct DATASTRUCT {
        char** tag_name;
        int* tag_id;
        int* parent_tag_id;
        int tag_id_max;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    DATASTRUCT* data;
    data = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data->tag_name = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->tag_name, nRows, sizeof(char*), "STRING *");
    data->tag_id = (int*)malloc((nRows) * sizeof(int));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->tag_id, nRows, sizeof(int), "int");
    data->parent_tag_id = (int*)malloc((nRows) * sizeof(int));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->parent_tag_id, nRows, sizeof(int), "int");

    data->tag_id_max = max_tag_id;

    // Get data from result
    int s_name_fnum = PQfnumber(DBQuery, "tag_name");
    int s_id_fnum = PQfnumber(DBQuery, "tag_id");
    int s_parent_fnum = PQfnumber(DBQuery, "parent_tag_id");

    int stringLength;

    for (i = 0; i < nRows; i++) {
        if (!PQgetisnull(DBQuery, i, s_name_fnum)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_name_fnum)) + 1;
            data->tag_name[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->tag_name[i], PQgetvalue(DBQuery, i, s_name_fnum));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->tag_name[i], stringLength, sizeof(char), "char");
            UDA_LOG(UDA_LOG_DEBUG, "%d tag %s\n", i, data->tag_name[i]);
        }
        if (!PQgetisnull(DBQuery, i, s_id_fnum)) {
            data->tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_id_fnum));
        } else {
            data->tag_id[i] = -1;
        }
        if (!PQgetisnull(DBQuery, i, s_parent_fnum)) {
            data->parent_tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_parent_fnum));
        } else {
            data->parent_tag_id[i] = -1;
        }
    }

    // Close db connection
    PQclear(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "define structure for datablock\n");

    // Output data block
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATASTRUCT");
    strcpy(parentTree.source, "idam3");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATASTRUCT);

    //Compound fields for datastruct
    initCompoundField(&field);
    strcpy(field.name, "tag_name");
    defineField(&field, "tag_name", "tag_name", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "tag_id");
    defineField(&field, "tag_id", "tag_id", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "parent_tag_id");
    defineField(&field, "parent_tag_id", "parent_tag_id", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    initCompoundField(&field);
    strcpy(field.name, "tag_id_max");
    defineField(&field, "tag_id_max", "tag_id_max", &offset, SCALARINT);
    addCompoundField(&parentTree, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    // Put data struct into data block for return
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "tags");
    strcpy(data_block->data_label, "tags");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "everything done\n");

    return 0;
}

