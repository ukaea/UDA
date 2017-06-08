/*---------------------------------------------------------------
// LIST all data access records from the Signals_Log table
*---------------------------------------------------------------------------------------------------------------*/
#include "listSignals.h"
#include "provenance.h"

#include <stdlib.h>
#include <strings.h>

#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>

int listSignals(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    int err = 0;
    int i, offset = 0, nrows;

    char sql[MAXSQL];
    int stringLength;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    USERDEFINEDTYPE usertype;
    COMPOUNDFIELD field;

    static PGconn* DBConnect = NULL;

    PGresult* DBQuery = NULL;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

        DBConnect = (PGconn*)idam_plugin_interface->sqlConnection;

    } else {
        err = 999;
        IDAM_LOG(UDA_LOG_ERROR, "Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        return err;
    }

    IDAM_LOG(UDA_LOG_DEBUG, "Plugin Interface transferred\n");

//----------------------------------------------------------------------------------------
// Common Name Value pairs

// Keywords have higher priority

//----------------------------------------------------------------------------------------
// Error trap

    err = 0;

    do {

        IDAM_LOG(UDA_LOG_DEBUG, "entering function list\n");

        char* uuid;
        unsigned short uuidOK = 0;

// Name Value pairs (Keywords have higher priority) + Protect against SQL Injection

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            IDAM_LOGF(UDA_LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                      request_block->nameValueList.nameValue[i].value);

            if (STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "uuid") ||
                STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "uid") ||
                STR_IEQUALS(request_block->nameValueList.nameValue[i].name, "doi")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                uuid = request_block->nameValueList.nameValue[i].value;
                uuidOK = 1;
                continue;
            }
        }

        if (!uuidOK) {
            err = 999;
            IDAM_LOG(UDA_LOG_ERROR, "ERROR Provenance list: The client provenance UUID must be specified!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err,
                         "The client provenance UUID must be specified!");
            break;
        }

        sprintf(sql, "SELECT uuid, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceUUID, "
                "logRecord, creation FROM signals_log WHERE uuid='%s';", uuid);

        IDAM_LOGF(UDA_LOG_DEBUG, "list() SQL\n%s\n", sql);

// Execute the SQL

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
            err = 999;
            IDAM_LOG(UDA_LOG_ERROR, "ERROR Provenance list: SQL Failed\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance list", err, "SQL Failed!");
            break;
        }

        nrows = PQntuples(DBQuery);

        if (nrows == 0) {
            IDAM_LOG(UDA_LOG_ERROR, "ERROR Provenance list: No signals_log records found!\n");
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err, "No signals_log records found");
            break;
        }

// Create the Data Structures to be returned	 

        PROVENANCESIGNAL* data = (PROVENANCESIGNAL*)malloc(nrows * sizeof(PROVENANCESIGNAL));

        addMalloc((void*)data, nrows, sizeof(PROVENANCESIGNAL), "PROVENANCESIGNAL");

        PROVENANCESIGNALLIST* list = (PROVENANCESIGNALLIST*)malloc(1 * sizeof(PROVENANCESIGNALLIST));

        list->structVersion = 1;
        list->count = nrows;
        list->uuid = (char*)malloc((strlen(uuid) + 1) * sizeof(char));
        strcpy(list->uuid, uuid);
        list->list = data;

        addMalloc((void*)list, 1, sizeof(PROVENANCESIGNALLIST), "PROVENANCESIGNALLIST");
        addMalloc((void*)list->uuid, 1, (strlen(uuid) + 1) * sizeof(char), "char");

// Extract the SQL data

        for (i = 0; i < nrows; i++) {

            stringLength = strlen(PQgetvalue(DBQuery, i, 0)) + 1;
            data[i].uuid = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].uuid, PQgetvalue(DBQuery, i, 0));
            addMalloc((void*)data[i].uuid, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 1)) + 1;
            data[i].requestedSignal = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].requestedSignal, PQgetvalue(DBQuery, i, 1));
            addMalloc((void*)data[i].requestedSignal, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 2)) + 1;
            data[i].requestedSource = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].requestedSource, PQgetvalue(DBQuery, i, 2));
            addMalloc((void*)data[i].requestedSource, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 3)) + 1;
            data[i].trueSignal = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].trueSignal, PQgetvalue(DBQuery, i, 3));
            addMalloc((void*)data[i].trueSignal, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 4)) + 1;
            data[i].trueSource = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].trueSource, PQgetvalue(DBQuery, i, 4));
            addMalloc((void*)data[i].trueSource, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 5)) + 1;
            data[i].trueSourceUUID = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].trueSourceUUID, PQgetvalue(DBQuery, i, 5));
            addMalloc((void*)data[i].trueSourceUUID, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 6)) + 1;
            data[i].logRecord = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].logRecord, PQgetvalue(DBQuery, i, 6));
            addMalloc((void*)data[i].logRecord, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, i, 7)) + 1;
            data[i].creation = (char*)malloc(stringLength * sizeof(char));
            strcpy(data[i].creation, PQgetvalue(DBQuery, i, 7));
            addMalloc((void*)data[i].creation, 1, stringLength * sizeof(char), "char");

            IDAM_LOGF(UDA_LOG_DEBUG, "uuid           : %s\n\n", data[i].uuid);
            IDAM_LOGF(UDA_LOG_DEBUG, "requestedSignal: %s\n", data[i].requestedSignal);
            IDAM_LOGF(UDA_LOG_DEBUG, "requestedSource: %s\n", data[i].requestedSource);
            IDAM_LOGF(UDA_LOG_DEBUG, "trueSignal     : %s\n", data[i].trueSignal);
            IDAM_LOGF(UDA_LOG_DEBUG, "trueSource     : %s\n", data[i].trueSource);
            IDAM_LOGF(UDA_LOG_DEBUG, "trueSourceUUID : %s\n", data[i].trueSourceUUID);
            IDAM_LOGF(UDA_LOG_DEBUG, "logRecord      : %s\n", data[i].logRecord);
            IDAM_LOGF(UDA_LOG_DEBUG, "creation date  : %s\n", data[i].creation);
        }
        PQclear(DBQuery);

// the Returned Structure Definition

        initUserDefinedType(&usertype);            // New structure definition

        strcpy(usertype.name, "PROVENANCESIGNAL");
        usertype.size = sizeof(PROVENANCESIGNAL);

        strcpy(usertype.source, "Provenance::admin()");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.idamclass = TYPE_COMPOUND;

        offset = 0;

        defineField(&field, "uuid", "Unique Identifier", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "requestedSignal", "requested Signal", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "requestedSource", "requested Source", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "trueSignal", "true Signal", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "trueSource", "true Source", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "trueSourceUUID", "true Source UUID", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "logRecord", " IDAM log Record", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        defineField(&field, "creation", "record creation date", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        addUserDefinedType(userdefinedtypelist, usertype);

        initUserDefinedType(&usertype);            // New structure definition

        strcpy(usertype.name, "PROVENANCESIGNALLIST");
        usertype.size = sizeof(PROVENANCESIGNALLIST);

        strcpy(usertype.source, "Provenance::admin()");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.idamclass = TYPE_COMPOUND;

        offset = 0;

        defineField(&field, "structVersion", "This Data Structure's version number", &offset, SCALARUSHORT);
        addCompoundField(&usertype, field);

        defineField(&field, "count", "The number of records in the list", &offset, SCALARUSHORT);
        addCompoundField(&usertype, field);

        defineField(&field, "uuid", "Unique Identifier", &offset, SCALARSTRING);
        addCompoundField(&usertype, field);

        initCompoundField(&field);
        strcpy(field.name, "list");
        field.atomictype = TYPE_UNKNOWN;
        strcpy(field.type, "PROVENANCESIGNAL");
        strcpy(field.desc, "List of Signals");
        field.pointer = 1;
        field.count = 1;
        field.rank = 0;
        field.shape = NULL;                // Needed when rank >= 1
        field.size = field.count * sizeof(PROVENANCESIGNAL);
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
        data_block->data = (char*)list;

        strcpy(data_block->data_desc, "List of Signals accessed with a UUID");
        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");

        data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void*)findUserDefinedType("PROVENANCESIGNALLIST", 0);

        IDAM_LOG(UDA_LOG_DEBUG, "Function list called\n");

        if (data_block->opaque_block == NULL) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance::list()", err,
                         "failed to locate PROVENANCESIGNALLIST structure!");
            break;
        }

        break;

    } while (0);

    return err;
}
