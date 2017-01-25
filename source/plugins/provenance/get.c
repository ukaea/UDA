/*---------------------------------------------------------------
  GET or NEW - issue a new UUID
 
  UUID_Register table contains the following
  
  UUID		Unique Identifier: Year+'/'+Ordinal Date+'/'+Issue Sequence
  Owner	name of the user or group that owns the UUID
  Class 
  Title 
  Description	
  icatRef	ICAT reference Id (foreign key)
  status	UUID status [open, closed, delete | firm | pending]
  creation	Date of registration
*---------------------------------------------------------------------------------------------------------------*/
#include "get.h"

#include <structures/struct.h>
#include <structures/accessors.h>

#include "provenance.h"

int get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    int err = 0;
    int i, offset = 0, nrows;

    char* env;
    char work[MAXSQL];
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

#ifndef USE_PLUGIN_DIRECTLY
// Don't copy the structure if housekeeping is requested - may dereference a NULL or freed pointer!     
        //if (!housekeeping && idam_plugin_interface->environment != NULL) environment = *idam_plugin_interface->environment;
        if (idam_plugin_interface->environment != NULL) environment = *idam_plugin_interface->environment;
#endif

        DBConnect = (PGconn*) idam_plugin_interface->sqlConnection;

    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR Provenance: Plugin Interface Version Unknown\n");

        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        return err;
    }

    idamLog(LOG_DEBUG, "Provenance: Plugin Interface transferred\n");

//----------------------------------------------------------------------------------------
// Common Name Value pairs

// Keywords have higher priority

//----------------------------------------------------------------------------------------
// Error trap

    err = 0;

    do {

        idamLog(LOG_DEBUG, "Provenance: entering function 'new'\n");

        char emptyString[1] = "";
        char* owner = emptyString, * icatRef = emptyString, * class = emptyString, * title = emptyString,
                * description = emptyString;
        unsigned short ownerOK = 0, returnUUIDOK = 0;

// specific Name Value pairs (Keywords have higher priority)

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            idamLog(LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                    request_block->nameValueList.nameValue[i].value);

            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "owner")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                owner = request_block->nameValueList.nameValue[i].value;
                ownerOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "icatRef")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                icatRef = request_block->nameValueList.nameValue[i].value;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "class")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                class = request_block->nameValueList.nameValue[i].value;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "title")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                title = request_block->nameValueList.nameValue[i].value;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "description")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                description = request_block->nameValueList.nameValue[i].value;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "returnUUID")) {
                returnUUIDOK = 1;
                continue;
            }
        }

        if (!ownerOK) {
            err = 999;
            idamLog(LOG_ERROR, "ERROR Provenance new: Insufficient Meta Data not passed - need an owner!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err,
                         "Insufficient Meta Data not passed - need an owner!");
            break;
        }

// Performance and UUID parameters

        struct timeval tv_start, tv_stop;
        int msecs, usecs;
        gettimeofday(&tv_start, NULL);

// Create the Unique Identifier

        struct timeval tv_uid;
        gettimeofday(&tv_uid, NULL);

        //int microsecs = (int)tv_uid.tv_usec;	// If this is loo low resolution, use struct timespec (nano-secs)

        int secs = (int) tv_uid.tv_sec;
        struct tm* uid_time = localtime((time_t*) &secs);

        char year[5];
        char day[4];

        strftime(year, sizeof(year), "%Y", uid_time);        // 4 digit year
        strftime(day, sizeof(day), "%j", uid_time);        // 3 digit day of year

        if ((env = getenv("IDAM_PROVPREFIX")) != NULL)
            sprintf(work, "%s/%s/%s/", env, year, day);
        else
            sprintf(work, "%s/%s/", year, day);


// Create Transaction Block SQL
// Create a new record
// Return key and key of first record today

        sprintf(sql, "BEGIN; INSERT INTO uuid_register (uuid, owner, class, title, description, icatref) "
                        "VALUES (nextval('uuid_temp_seq'),'%s','%s','%s','%s','%s'); END; "
                        "SELECT * FROM "
                        "(SELECT uuid_register_id FROM uuid_register where uuid_register_id = currval('uuid_register_id_seq')) as A, "
                        "(SELECT min(uuid_register_id) as minid FROM uuid_register where creation=current_date) as B;",
                owner, class, title, description, icatRef);

        idamLog(LOG_DEBUG, "%s\n", sql);

// execute

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, "SQL Execution Failed!");
            idamLog(LOG_ERROR, "ERROR Provenance new: SQL Execution Failed\n");
            PQclear(DBQuery);
            break;
        }

// Extract returned values

        nrows = PQntuples(DBQuery);

        if (nrows != 1) {
            idamLog(LOG_ERROR, "ERROR Provenance new: New UUID not available!\n");
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err, "New UUID not available!");
            PQclear(DBQuery);
            break;
        }

        int newKey = atoi(PQgetvalue(DBQuery, 0, 0));
        int firstKey = atoi(PQgetvalue(DBQuery, 0, 1));
        int seq = newKey - firstKey + 1;

        idamLog(LOG_DEBUG, "Provenance new Key  : %d\n", newKey);
        idamLog(LOG_DEBUG, "           First Key: %d\n", firstKey);
        idamLog(LOG_DEBUG, "           Sequence : %d\n", seq);

        PQclear(DBQuery);

// Prepare UUID

        if ((env = getenv("IDAM_PROVPREFIX")) != NULL)
            sprintf(work, "%s/%s/%s/%d", env, year, day, seq);
        else
            sprintf(work, "%s/%s/%d", year, day, seq);

// Update UUID_register table

        sprintf(sql, "BEGIN; UPDATE uuid_register SET uuid = '%s' WHERE uuid_register_id = %d; END;",
                work, newKey);

        idamLog(LOG_DEBUG, "%s\n", sql);

// Execute

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, "SQL Execution Failed!");
            idamLog(LOG_ERROR, "ERROR Provenance new: SQL Execution Failed\n");
            PQclear(DBQuery);
            break;
        }

        PQclear(DBQuery);

// Performance

        gettimeofday(&tv_stop, NULL);
        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 + (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 + (int) (tv_stop.tv_usec - tv_start.tv_usec);
        idamLog(LOG_DEBUG, "Provenance: new() SQL Cost = %d (ms), %d (microsecs)\n", msecs, usecs);

// Read the UUID_register table record

        sprintf(sql, "SELECT uuid, owner, class, title, description, icatref, status, creation "
                "FROM uuid_register WHERE uuid_register_id = %d;", newKey);

        idamLog(LOG_DEBUG, "%s\n", sql);

// Execute

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, "SQL Execution Failed!");
            idamLog(LOG_ERROR, "ERROR Provenance new: SQL Execution Failed\n");
            PQclear(DBQuery);
            break;
        }

// Write Return Structure

        nrows = PQntuples(DBQuery);

        if (nrows != 1) {
            idamLog(LOG_ERROR, "ERROR Provenance new: New UUID not available!\n");
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err, "New UUID not available!");
            PQclear(DBQuery);
            break;
        }

        if (returnUUIDOK) {

            stringLength = strlen(PQgetvalue(DBQuery, 0, 0)) + 1;
            data_block->data = (char*) malloc(stringLength * sizeof(char));
            strcpy(data_block->data, PQgetvalue(DBQuery, 0, 0));

            PQclear(DBQuery);

            idamLog(LOG_DEBUG, "Provenance uuid: %s\n", data_block->data);

// Pass the Data back	 

            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = stringLength;

            strcpy(data_block->data_desc, "New Provenance UUID");

        } else {

// Create the Data Structure to be returned	 

            PROVENANCEUUID* data = NULL;
            data = (PROVENANCEUUID*) malloc(sizeof(PROVENANCEUUID));    // Structured Data Must be a heap variable

            stringLength = strlen(PQgetvalue(DBQuery, 0, 0)) + 1;
            data->uuid = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->uuid, PQgetvalue(DBQuery, 0, 0));
            addMalloc((void*) data->uuid, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 1)) + 1;
            data->owner = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->owner, PQgetvalue(DBQuery, 0, 1));
            addMalloc((void*) data->owner, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 2)) + 1;
            data->class = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->class, PQgetvalue(DBQuery, 0, 2));
            addMalloc((void*) data->class, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 3)) + 1;
            data->title = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->title, PQgetvalue(DBQuery, 0, 3));
            addMalloc((void*) data->title, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 4)) + 1;
            data->description = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->description, PQgetvalue(DBQuery, 0, 4));
            addMalloc((void*) data->description, 1, stringLength * sizeof(char), "char");

            stringLength = strlen(PQgetvalue(DBQuery, 0, 5)) + 1;
            data->icatRef = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->icatRef, PQgetvalue(DBQuery, 0, 5));
            addMalloc((void*) data->icatRef, 1, stringLength * sizeof(char), "char");

            data->status = PQgetvalue(DBQuery, 0, 6)[0];

            stringLength = strlen(PQgetvalue(DBQuery, 0, 7)) + 1;
            data->creation = (char*) malloc(stringLength * sizeof(char));
            strcpy(data->creation, PQgetvalue(DBQuery, 0, 7));
            addMalloc((void*) data->creation, 1, stringLength * sizeof(char), "char");

            PQclear(DBQuery);

            idamLog(LOG_DEBUG, "Provenance uuid: %s\n", data->uuid);
            idamLog(LOG_DEBUG, "owner          : %s\n", data->owner);
            idamLog(LOG_DEBUG, "class          : %s\n", data->class);
            idamLog(LOG_DEBUG, "title          : %s\n", data->title);
            idamLog(LOG_DEBUG, "description    : %s\n", data->description);
            idamLog(LOG_DEBUG, "icatRefId      : %s\n", data->icatRef);
            idamLog(LOG_DEBUG, "status         : %c\n", data->status);
            idamLog(LOG_DEBUG, "creation       : %s\n", data->creation);

// the Returned Structure Definition

            data->structVersion = 1;                // This structure's version number

            initUserDefinedType(&usertype);            // New structure definition

            strcpy(usertype.name, "PROVENANCEUUID");
            usertype.size = sizeof(PROVENANCEUUID);

            strcpy(usertype.source, "Provenance::admin()");
            usertype.ref_id = 0;
            usertype.imagecount = 0;                // No Structure Image data
            usertype.image = NULL;
            usertype.idamclass = TYPE_COMPOUND;

            offset = 0;

            defineField(&field, "structVersion", "This Data Structure's version number", &offset, SCALARUSHORT);
            addCompoundField(&usertype, field);
            defineField(&field, "uuid", "Unique Identifier", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "owner", "Owner", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "class", "Class", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "title", "Title", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "description", "Description", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "icatRef", "ICAT Reference ID", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);
            defineField(&field, "status", "Status", &offset, SCALARCHAR);
            addCompoundField(&usertype, field);
            defineField(&field, "creation", "Creation or registration date", &offset, SCALARSTRING);
            addCompoundField(&usertype, field);

            addUserDefinedType(userdefinedtypelist, usertype);

// Register the Pointer to the Data Structure 	 

            addMalloc((void*) data, 1, sizeof(PROVENANCEUUID), "PROVENANCEUUID");

// Pass the Data back	 

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "A Provenance UUID Record");
            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("PROVENANCEUUID", 0);

        }

        idamLog(LOG_DEBUG, "Provenance: exiting function new\n");

    } while (0);

    return err;
}
