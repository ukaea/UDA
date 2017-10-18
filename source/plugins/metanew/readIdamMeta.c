#include "readIdamMeta.h"

#include <strings.h>

#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>

PGconn* open_connection()
{
    char* db_host = getenv("UDA_SQLHOST");
    char* db_port_str = getenv("UDA_SQLPORT");
    int db_port = -1;
    if (db_port_str != NULL) db_port = atoi(db_port_str);
    char* db_name = getenv("UDA_SQLDBNAME");
    char* db_user = getenv("UDA_SQLUSER");

    if (db_host == NULL || db_port_str == NULL || db_name == NULL || db_user == NULL) {
        return NULL;
    }

    PGconn* DBConnect = openDatabase(db_host, db_port, db_name, db_user);

    return DBConnect;
}

char* get_escaped_string(PGconn* DBConnect, const char* instring)
{
    // String length for PQescapeStringConn needs to be double the length
    int new_length = 2 * strlen(instring) + 1;

    char* new_string = NULL;
    new_string = (char*)malloc(new_length * sizeof(char));

    // Escape it
    int err = 0;
    PQescapeStringConn(DBConnect, new_string, instring, strlen(instring), &err);

    return new_string;
}

int get_lastshot(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    // Open database connection
    PGconn* DBConnect = open_connection();
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to database failed.\n");
    }

    // Query
    char query[MAXSQL];
    sprintf(query, "SELECT max(exp_number) FROM ExpDateTime;");

    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "No rows found\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    if (PQgetisnull(DBQuery, 0, 0)) {
        UDA_LOG(UDA_LOG_DEBUG, "exp_number not found in row returned by query\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("exp_number not found in row returned by query\n");
    }

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    // Create the Returned Structure Definition
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "DATALASTSHOT");
    strcpy(usertype.source, "readMetaNew");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(DATALASTSHOT);        // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;
    defineField(&field, "lastshot", "last shot number", &offset, SCALARUINT);
    addCompoundField(&usertype, field);        // Single Structure element
    addUserDefinedType(idam_plugin_interface->userdefinedtypelist, usertype);

    // Create Data
    DATALASTSHOT* data;
    data = (DATALASTSHOT*)malloc(sizeof(DATALASTSHOT));    // Structured Data Must be a heap variable
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATALASTSHOT), "DATALASTSHOT");

    data->lastshot = atoi(PQgetvalue(DBQuery, 0, 0));

    UDA_LOG(UDA_LOG_DEBUG, "getLastShot: %d\n", data->lastshot);

    PQclear(DBQuery);

    // Pass Data
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "getLastShot");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(idam_plugin_interface->userdefinedtypelist, "DATALASTSHOT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "readMetaNew: Function getLastShot called\n");

    PQfinish(DBConnect);

    return 0;
}

int get_lastpass(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    // User must give shot number & source
    int shot = 0;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shot);

    const char* source = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, source);

    // Open database connection
    PGconn* DBConnect = open_connection();
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to database failed.\n");
    }

    // Query
    char query[MAXSQL];
    sprintf(query, "SELECT pass, type FROM data_source WHERE source_alias='%s' AND exp_number='%d' "
            "ORDER BY pass DESC LIMIT 1", source, shot);

    UDA_LOG(UDA_LOG_DEBUG, "Query %s\n", query);

    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "no rows found\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    if (PQgetisnull(DBQuery, 0, 0) || PQgetisnull(DBQuery, 0, 1)) {
        UDA_LOG(UDA_LOG_DEBUG, "exp_number not found in row returned by query\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("exp_number not found in row returned by query\n");
    }

    if (!STR_IEQUALS(PQgetvalue(DBQuery, 0, 1), "A")) {
        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Only analysed data sources have a pass number!\n");
    }

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    // Create the Returned Structure Definition
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "DATALASTPASS");
    strcpy(usertype.source, "readMetaNew");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(DATALASTPASS);        // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;
    defineField(&field, "lastpass", "last pass", &offset, SCALARUINT);
    addCompoundField(&usertype, field);        // Single Structure element
    addUserDefinedType(idam_plugin_interface->userdefinedtypelist, usertype);

    // Create Data
    DATALASTPASS* data;
    data = (DATALASTPASS*)malloc(sizeof(DATALASTPASS));    // Structured Data Must be a heap variable
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATALASTPASS), "DATALASTPASS");

    UDA_LOG(UDA_LOG_DEBUG, "lastpass from query %s\n", PQgetvalue(DBQuery, 0, 0));

    data->lastpass = atoi(PQgetvalue(DBQuery, 0, 0));

    UDA_LOG(UDA_LOG_DEBUG, "get_lastpass: %d\n", data->lastpass);

    PQclear(DBQuery);

    // Pass Data
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "get_lastpass");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(idam_plugin_interface->userdefinedtypelist, "DATALASTPASS", 0);

    UDA_LOG(UDA_LOG_DEBUG, "readMetaNew: Function get_lastpass called\n");

    PQfinish(DBConnect);

    return 0;
}

int get_shotdatetime(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    // User must give shot number
    int shot = 0;
    FIND_REQUIRED_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shot);

    // Open database connection
    PGconn* DBConnect = open_connection();
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to database failed.\n");
    }

    // Query
    char query[MAXSQL];
    sprintf(query, "SELECT exp_date, exp_time FROM ExpDateTime WHERE exp_number=%d", shot);

    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "no rows returned by query\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    int s_expdate = PQfnumber(DBQuery, "exp_date");
    int s_exptime = PQfnumber(DBQuery, "exp_time");

    if (PQgetisnull(DBQuery, 0, s_expdate) || PQgetisnull(DBQuery, 0, s_exptime)) {
        UDA_LOG(UDA_LOG_DEBUG, "Date and time returned from query are null\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("exp date and time not found in row returned by query\n");
    }

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    // Create the Returned Structure Definition
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "DATASHOTDATETIME");
    strcpy(usertype.source, "readMeta");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(DATASHOTDATETIME);    // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    defineField(&field, "shot", "shot number", &offset, SCALARUINT);
    addCompoundField(&usertype, field);
    defineField(&field, "date", "shot date", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);
    defineField(&field, "time", "shot time", &offset, SCALARSTRING);
    addCompoundField(&usertype, field);

    addUserDefinedType(idam_plugin_interface->userdefinedtypelist, usertype);

    // Create Data

    DATASHOTDATETIME* data;
    data = (DATASHOTDATETIME*)malloc(sizeof(DATASHOTDATETIME));    // Structured Data Must be a heap variable
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASHOTDATETIME), "DATASHOTDATETIME");

    data->shot = shot;

    int stringLength = strlen(PQgetvalue(DBQuery, 0, s_expdate)) + 1;
    data->date = (char*)malloc(stringLength * sizeof(char));
    strcpy(data->date, PQgetvalue(DBQuery, 0, s_expdate));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->date, 1, stringLength * sizeof(char), "char");

    stringLength = strlen(PQgetvalue(DBQuery, 0, s_exptime)) + 1;
    data->time = (char*)malloc(stringLength * sizeof(char));
    strcpy(data->time, PQgetvalue(DBQuery, 0, s_exptime));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->time, 1, stringLength * sizeof(char), "char");

    UDA_LOG(UDA_LOG_DEBUG, "getShotDateTime:\n");
    UDA_LOG(UDA_LOG_DEBUG, "Shot: %d\n", data->shot);
    UDA_LOG(UDA_LOG_DEBUG, "Date: %s\n", data->date);
    UDA_LOG(UDA_LOG_DEBUG, "Time: %s\n", data->time);

    PQclear(DBQuery);

    // Pass Data
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "getShotDateTime");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(idam_plugin_interface->userdefinedtypelist, "DATASHOTDATETIME", 0);

    UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function getShotDateTime called\n");

    PQfinish(DBConnect);

    return 0;
}

int get_listsignals(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    // Open database connection
    PGconn* DBConnect = open_connection();
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to database failed.\n");
    }

    // User inputs
    int shot = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, shot);

    int pass = -1;
    FIND_INT_VALUE(idam_plugin_interface->request_block->nameValueList, pass);

    const char* source = NULL;
    char* source_escaped = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, source);

    if (source != NULL) {
        source_escaped = get_escaped_string(DBConnect, source);
    }

    const char* type = NULL;
    char* type_escaped = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, type);

    if (type != NULL) {
        type_escaped = get_escaped_string(DBConnect, type);
    }

    const char* signal_match = NULL;
    char* signal_match_escaped = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, signal_match);

    if (signal_match != NULL) {
        signal_match_escaped = get_escaped_string(DBConnect, signal_match);
    }

    const char* description = NULL;
    char* description_escaped = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, description);

    if (description != NULL) {
        description_escaped = get_escaped_string(DBConnect, description);
    }

    const char* cast = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, cast);
    int column = 1;
    if (cast != NULL) {
        if (STR_IEQUALS(cast, "row")) {
            column = 0;
        }
    }

    // Optional bits for query
    char extra[MAXSQL];
    int has_extra = ((pass >= 0) || (source_escaped != NULL)
                     || (type_escaped != NULL) || (signal_match_escaped != NULL)
                     || (description_escaped != NULL));

    if (pass >= 0 && shot > 0) {
        sprintf(extra, " AND ds.pass = %d", pass);
    } else {
        sprintf(extra, " ");
    }

    if (source_escaped != NULL) {
        strcat(extra, " AND sd.source_alias = '");
        strcat(extra, strlwr(source_escaped));
        strcat(extra, "'");
    }

    if (type_escaped != NULL) {
        strcat(extra, " AND sd.type = '");
        strcat(extra, strupr(type_escaped));
        strcat(extra, "'");
    }

    if (signal_match_escaped != NULL) {
        strcat(extra, " AND sd.signal_alias ILIKE '%");
        strcat(extra, signal_match_escaped);
        strcat(extra, "%'");
    }

    if (description_escaped != NULL) {
        strcat(extra, " AND sd.description ILIKE '%");
        strcat(extra, description_escaped);
        strcat(extra, "%'");
    }

    // Construct query
    char query[MAXSQL];
    if (shot > 0) {
        sprintf(query,
                "SELECT sd.signal_alias, sd.generic_name, sd.source_alias, sd.type, sd.description, s.signal_status"
                        " FROM signal_desc sd, data_source ds, signal s "
                        " WHERE ds.source_id=s.source_id AND s.signal_desc_id=sd.signal_desc_id AND ds.exp_number=%d %s "
                        " ORDER BY sd.signal_alias ASC",
                shot, extra);
    } else if (has_extra) {
        sprintf(query,
                "SELECT signal_alias, generic_name, source_alias, type, description FROM signal_desc sd "
                        " WHERE %s"
                        " ORDER BY signal_alias ASC", &extra[5]); // don't add first AND
    } else {
        sprintf(query,
                "SELECT signal_alias, generic_name, source_alias, type, description FROM signal_desc sd "
                        " ORDER BY signal_alias ASC");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Query %s\n", query);

    // Query
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    int nRows = PQntuples(DBQuery);
    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "no rows found\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    int s_signal_alias = PQfnumber(DBQuery, "signal_alias");
    int s_generic_name = PQfnumber(DBQuery, "generic_name");
    int s_source_alias = PQfnumber(DBQuery, "source_alias");
    int s_type = PQfnumber(DBQuery, "type");
    int s_description = PQfnumber(DBQuery, "description");
    int s_signal_status = PQfnumber(DBQuery, "signal_status");

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    if (column) {
        // Create the Returned Structure Definition
        initUserDefinedType(&usertype);            // New structure definition

        strcpy(usertype.name, "DATALISTSIGNALS_C");
        usertype.size = sizeof(DATALISTSIGNALS_C);        // Selected return structure size

        strcpy(usertype.source, "listData");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.idamclass = UDA_TYPE_COMPOUND;

        int offset = 0;

        defineField(&field, "count", "Array element count", &offset, SCALARUINT);
        addCompoundField(&usertype, field);
        defineField(&field, "shot", "shot number", &offset, SCALARUINT);
        addCompoundField(&usertype, field);
        defineField(&field, "pass", "pass number", &offset, SCALARINT);
        addCompoundField(&usertype, field);

        defineField(&field, "signal_name", "signal name", &offset,
                    ARRAYSTRING);    // Array of strings, arbitrary length
        // **** NameSpace collision: remove from libidamr.so
        field.atomictype = UDA_TYPE_STRING;
        addCompoundField(&usertype, field);
        defineField(&field, "generic_name", "generic name", &offset, ARRAYSTRING);
        field.atomictype = UDA_TYPE_STRING;
        addCompoundField(&usertype, field);
        defineField(&field, "source_alias", "source alias name", &offset, ARRAYSTRING);
        field.atomictype = UDA_TYPE_STRING;
        addCompoundField(&usertype, field);
        defineField(&field, "type", "data type classification", &offset, ARRAYSTRING);
        field.atomictype = UDA_TYPE_STRING;
        addCompoundField(&usertype, field);
        defineField(&field, "description", "data description", &offset, ARRAYSTRING);
        field.atomictype = UDA_TYPE_STRING;
        addCompoundField(&usertype, field);

        // Make it so that you only get signal status if you've given an exp num.
        defineField(&field, "signal_status", "signal status", &offset, ARRAYINT);
        field.atomictype = UDA_TYPE_INT;
        addCompoundField(&usertype, field);

        addUserDefinedType(idam_plugin_interface->userdefinedtypelist, usertype);

        UDA_LOG(UDA_LOG_DEBUG, "listData:\n");
        UDA_LOG(UDA_LOG_DEBUG, "Shot: %d\n", shot);
        UDA_LOG(UDA_LOG_DEBUG, "Pass: %d\n", pass);

        DATALISTSIGNALS_C* data;
        data = (DATALISTSIGNALS_C*)malloc(sizeof(DATALISTSIGNALS_C));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATALISTSIGNALS_C), "DATALISTSIGNALS_C");

        data->signal_name = (char**)malloc(nRows * sizeof(char*));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_name, nRows, sizeof(char*), "STRING *");
        data->generic_name = (char**)malloc(nRows * sizeof(char*));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->generic_name, nRows, sizeof(char*), "STRING *");
        data->source_alias = (char**)malloc(nRows * sizeof(char*));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->source_alias, nRows, sizeof(char*), "STRING *");
        data->type = (char**)malloc(nRows * sizeof(char*));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->type, nRows, sizeof(char*), "STRING *");
        data->description = (char**)malloc(nRows * sizeof(char*));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->description, nRows, sizeof(char*), "STRING *");
        data->signal_status = (int*)malloc(nRows * sizeof(int));
        addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_status, nRows, sizeof(int), "INT");

        if (shot > 0) {
            data->shot = shot;
        } else {
            data->shot = 0;
        }
        data->pass = pass;
        data->count = nRows;

        int i = 0;
        int stringLength = 0;
        for (i = 0; i < nRows; i++) {
            stringLength = strlen(PQgetvalue(DBQuery, i, s_signal_alias)) + 1;
            data->signal_name[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->signal_name[i], PQgetvalue(DBQuery, i, s_signal_alias));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_name[i], stringLength, sizeof(char), "STRING");

            stringLength = strlen(PQgetvalue(DBQuery, i, s_generic_name)) + 1;
            data->generic_name[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->generic_name[i], PQgetvalue(DBQuery, i, s_generic_name));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->generic_name[i], stringLength, sizeof(char), "STRING");

            stringLength = strlen(PQgetvalue(DBQuery, i, s_source_alias)) + 1;
            data->source_alias[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->source_alias[i], PQgetvalue(DBQuery, i, s_source_alias));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->source_alias[i], stringLength, sizeof(char), "STRING");

            char work[MAXSQL];
            char* p = PQgetvalue(DBQuery, i, s_type);
            if (p[0] == 'A') {
                strcpy(work, "Analysed");
            } else if (p[0] == 'R') {
                strcpy(work, "Raw");
            } else if (p[0] == 'I') {
                strcpy(work, "Image");
            } else if (p[0] == 'M') {
                strcpy(work, "Modelled");
            } else
                strcpy(work, "Unknown");

            stringLength = strlen(work) + 1;
            data->type[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->type[i], work);
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->type[i], stringLength, sizeof(char), "STRING");

            stringLength = strlen(PQgetvalue(DBQuery, i, s_description)) + 1;
            data->description[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->description[i], PQgetvalue(DBQuery, i, s_description));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->description[i], stringLength, sizeof(char), "STRING");

            if (PQgetisnull(DBQuery, 0, s_signal_status)) {
                data->signal_status[i] = 1;
            } else {
                data->signal_status[i] = (int)atoi(PQgetvalue(DBQuery, i, s_signal_status));
            }
        }

        // Pass Data
        DATA_BLOCK* data_block = idam_plugin_interface->data_block;
        initDataBlock(data_block);

        data_block->data_type = UDA_TYPE_COMPOUND;
        data_block->rank = 0;
        data_block->data_n = 1;
        data_block->data = (char*)data;

        strcpy(data_block->data_desc, "listSignals");
        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");

        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void*)findUserDefinedType(idam_plugin_interface->userdefinedtypelist, "DATALISTSIGNALS_C", 0);

        UDA_LOG(UDA_LOG_DEBUG, "readMeta: Function listSignals called\n");

        PQclear(DBQuery);
        PQfinish(DBConnect);
    } else {
        PQclear(DBQuery);
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Sorry, row cast not yet implemented!\n");
    }

    free(source_escaped);
    free(type_escaped);
    free(signal_match_escaped);
    free(description_escaped);

    return 0;
}
