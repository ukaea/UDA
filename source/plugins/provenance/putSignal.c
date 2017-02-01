/*---------------------------------------------------------------
// Record data access requests in a temporary database and manage the status of the records: 
//
// putSignal( user=user, uuid=uuid, requestedSignal=requestedSignal, requestedSource=requestedSource, 
//	trueSignal=trueSignal, trueSource=trueSource, trueSourceUUID=trueSourceUUID,
//	logRecord=logRecord, created=created, status=[new|update|close|delete], execMethod=execMethod) 	
//
// Signals_Log table contains the following
// 
// signals_log_id	Primary key
// uuid			UUID reference (may not be related to the uuid_register table)
// requestedSignal	Requested data signal name	 
// requestedSource 	Requested data source name
// trueSignal		Signal within the source file
// trueSource		File path and name
// trueSourceUUID	Data source file's UUID
// logRecord		Log record of the data access
// created		date record was written
// status		0=>open, 1=>closed (Closed when requested by client application)
// execMethod = 1	Execute the SQL commands in the background immediately
//			Unreliable as often the log record is missing - perhaps executed before the record is created. 
// execMethod = 2	Collect the SQL commands together into a file and execute at the end - when 
//			the log record is written. This method is only employed for ADD and UPDATE.
//			Require automatic purge of sql command files
// execMethod = 3	Collect the SQL commands together into a static string buffer and execute at the end - when 
//			the log record is written. This method is only employed for ADD and UPDATE. No files to delete!
//
// The cost of writing to the database after each individual data access is high. This can be mitigated by
// writing SQL commands to a file and executing these together at the end of the process. The UUID can be
// used to identify this file. Alternatively, each SQL command can be executed via a command pipe in the 
// background without blocking. This also has a time cost as a command shell must be forked.

// The cost of method 1 ~5-10 ms	default server setting
// The cost of method 2 ~3-5  ms	
// The cost of method 3 ?     ms

// The default is over-ruled in server startup script by IDAM_PROVENANCE_EXEC_METHOD
// The passed keyword execMethod has highest priority
*---------------------------------------------------------------------------------------------------------------*/
#include "putSignal.h"

#include <stdlib.h>
#include <errno.h>
#include <strings.h>

#include <clientserver/idamTypes.h>

#include "provenance.h"

int putSignal(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    int i, nrows;

    char* env;
    char work[MAXSQL];
    char sql[MAXSQL];
    int stringLength;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;

    static PGconn* DBConnect = NULL;

    PGresult* DBQuery = NULL;

    if (idam_plugin_interface->interfaceVersion == 1) {

        idam_plugin_interface->pluginVersion = 1;

        data_block = idam_plugin_interface->data_block;
        request_block = idam_plugin_interface->request_block;

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

        static struct timeval tv_start, tv_stop;    // Performance
        int msecs, usecs;

        idamLog(LOG_DEBUG, "Provenance: entering function record\n");

        char empty[1] = "";
        char* uuid = empty, * requestedSignal = empty, * requestedSource = empty,
                * trueSignal = empty, * trueSource = empty, * trueSourceUUID = empty,
                * logRecord = empty;
        char status = '?';
        unsigned short uuidOK = 0, requestedSignalOK = 0, requestedSourceOK = 0, trueSignalOK = 0,
                trueSourceOK = 0, trueSourceUUIDOK = 0, logRecordOK = 0, statusOK = 0, execMethodOK = 0;
        unsigned short execMethod = 0;

// Name Value pairs (Keywords have higher priority)

        for (i = 0; i < request_block->nameValueList.pairCount; i++) {
            idamLog(LOG_DEBUG, "[%d] %s = %s\n", i, request_block->nameValueList.nameValue[i].name,
                    request_block->nameValueList.nameValue[i].value);

            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "uuid") ||
                !strcasecmp(request_block->nameValueList.nameValue[i].name, "uid") ||
                !strcasecmp(request_block->nameValueList.nameValue[i].name, "DOI")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                uuid = request_block->nameValueList.nameValue[i].value;
                uuidOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "requestedSignal")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                requestedSignal = request_block->nameValueList.nameValue[i].value;
                requestedSignalOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "requestedSource")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                requestedSource = request_block->nameValueList.nameValue[i].value;
                requestedSourceOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSignal")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                trueSignal = request_block->nameValueList.nameValue[i].value;
                trueSignalOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSource")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                trueSource = request_block->nameValueList.nameValue[i].value;
                trueSourceOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "trueSourceUUID")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                trueSourceUUID = request_block->nameValueList.nameValue[i].value;
                trueSourceUUIDOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "logRecord")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                logRecord = request_block->nameValueList.nameValue[i].value;
                logRecordOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "status")) {
                preventSQLInjection(DBConnect, &request_block->nameValueList.nameValue[i].value, 1);
                status = request_block->nameValueList.nameValue[i].value[0];
                statusOK = 1;
                continue;
            }
            if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "execMethod")) {
                execMethod = (short) atoi(request_block->nameValueList.nameValue[i].value);

                if ((env = getenv("IDAM_PROVENANCE_EXEC_METHOD")) != NULL)
                    execMethod = (short) atoi(env);        // server environment has priority

                execMethodOK = 1;
                continue;
            }

        }

        if (!execMethodOK && (env = getenv("IDAM_PROVENANCE_EXEC_METHOD")) != NULL) {
            execMethod = (short) atoi(env);        // server environment sets an alternative default value
            execMethodOK = 1;
        }

        if (!uuidOK || strlen(uuid) == 0) {
            err = 999;
            idamLog(LOG_ERROR, "ERROR Provenance add: The client provenance UUID must be specified!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err,
                         "The client provenance UUID must be specified!");
            break;
        }

        if (!statusOK) {
            err = 999;
            idamLog(LOG_ERROR, "ERROR Provenance add: The record status must be specified!\n");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err,
                         "The record status must be specified!");
            break;
        }

        idamLog(LOG_DEBUG, "Provenance: passed parameters\n");
        if (uuidOK) idamLog(LOG_DEBUG, "uuid = %s\n", uuid);
        if (requestedSignalOK) idamLog(LOG_DEBUG, "requestedSignal = %s\n", requestedSignal);
        if (requestedSourceOK) idamLog(LOG_DEBUG, " requestedSource = %s\n", requestedSource);
        if (trueSignalOK) idamLog(LOG_DEBUG, "trueSignal = %s\n", trueSignal);
        if (trueSourceOK) idamLog(LOG_DEBUG, "trueSource = %s\n", trueSource);
        if (trueSourceUUIDOK) idamLog(LOG_DEBUG, "trueSourceUUID = %s\n", trueSourceUUID);
        if (logRecordOK) idamLog(LOG_DEBUG, "logRecord = %s\n", logRecord);
        if (statusOK) idamLog(LOG_DEBUG, "Status = %c\n", status);
        if (execMethodOK) idamLog(LOG_DEBUG, "execMethod = %d\n", execMethod);

// 1> Create a new record if status == new 
// 2> add log data to an existing record if status == update 
// 3> close status for all records if status == close

        static FILE* sqlSet = NULL;
        static char* key = NULL;
        static char* tmpfile = NULL;
        static unsigned short keySeq = 0;

        FILE* ph = NULL;
        char cmd[2048];
        static char sqlBuffer[1024];

        do {            // Transaction Block Error Trap

            if (status == 'n') {    // Create a new record and reset the current primary key in scope
                idamLog(LOG_DEBUG, "Provenance: record() Create a new record\n");

                if (key != NULL) {    // Always renewed for each new record
                    free((void*) key);
                    key = NULL;
                }

                if (execMethodOK) {

// No Primary Key returned so must create a unique key - use the randomised component of the file name + User's UUID	    

                    sprintf(work, "%s/%d/%d", uuid, initTime, keySeq++);    // Create a unique key
                    key = (char*) malloc((strlen(work) + 1) * sizeof(char));
                    strcpy(key, work);

                    if (execMethod == 1) {

// Login password is stored in .pgpass for POSTGRESQL database so no need to set	       

                        if ((env = getenv("IDAM_CLI_SQL")) != NULL)
                            strcpy(cmd, env);                    // Command line sql utility
                        else
                            strcpy(cmd, "psql");

                        sprintf(&cmd[strlen(cmd)],
                                " -d %s -U %s -h %s -p %s -c \""
                                        "INSERT INTO signals_log "
                                        "(uuid, requestedSignal, requestedSource, trueSignal, trueSource, "
                                        "trueSourceUUID, tmpKey) "
                                        "VALUES ('%s','%s','%s','%s','%s','%s','%s');\" > /dev/null 2>&1 &",
                                dbname, user, pghost, pgport,
                                uuid, requestedSignal, requestedSource, trueSignal, trueSource,
                                trueSourceUUID, key);

                        idamLog(LOG_DEBUG, "Provenance: record() SQL\n%s\n", cmd);

                        gettimeofday(&tv_start, NULL);

                        errno = 0;
                        ph = popen(cmd, "r");

                        if (ph == NULL || errno != 0) {
                            execMethodOK = 0;        // Disable exec method
                            err = 999;
                            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", errno, "");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                                         "Cannot execute background SQL command");
                            break;
                        }

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 1 Cost A = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;
                    } else if (execMethod == 2) {

// File directory 

                        char* tmpdir = getenv("IDAM_WORK_DIR");

// Create a temporary file to collect the SQL commands

                        if (tmpdir != NULL) {
                            tmpfile = (char*) malloc((strlen(tmpdir) + 11) * sizeof(char));
                            sprintf(tmpfile, "%s/sqlXXXXXX", tmpdir);
                        } else {
                            tmpfile = (char*) malloc(16 * sizeof(char));
                            strcpy(tmpfile, "/tmp/sqlXXXXXX");
                        }

                        gettimeofday(&tv_start, NULL);

                        errno = 0;
                        if (mkstemp(tmpfile) < 0 || errno != 0) {
                            execMethodOK = 0;        // Disable SQL collection
                            err = 999;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", err,
                                         "Unable to Obtain a Temporary File Name");
                            idamLog(LOG_ERROR, "ERROR Provenance: Unable to Obtain a Temporary File Name\n");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, tmpdir);
                            break;
                        }

                        if ((sqlSet = fopen(tmpfile, "w")) == NULL) {
                            execMethodOK = 0;        // Disable collection
                            err = 999;
                            if (errno != 0) err = errno;
                            addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", err,
                                         "Unable to Open a Temporary File");
                            idamLog(LOG_ERROR, "ERROR Provenance: Unable to Open a Temporary File\n");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err, tmpfile);
                            break;
                        }

                        fprintf(sqlSet, "BEGIN; INSERT INTO signals_log "
                                        "(uuid, requestedSignal, requestedSource, trueSignal, trueSource, "
                                        "trueSourceUUID, tmpKey) "
                                        "VALUES ('%s','%s','%s','%s','%s','%s','%s');",
                                uuid, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceUUID, key);

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 2 Cost A = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;
                    } else if (execMethod == 3) {

                        gettimeofday(&tv_start, NULL);

                        sprintf(sqlBuffer, "BEGIN; INSERT INTO signals_log "
                                        "(uuid, requestedSignal, requestedSource, trueSignal, trueSource, "
                                        "trueSourceUUID, logRecord) "
                                        "VALUES ('%s','%s','%s','%s','%s','%s',",
                                uuid, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceUUID);

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 3 Cost A = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;
                    } else {
                        err = 999;
                        idamLog(LOG_ERROR, "ERROR Provenance add: Incorrect execMethod argument {1|2|3}\n");
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err,
                                     "Incorrect execMethod argument {1|2|3}");
                        break;
                    }


                } else {                // Expensive & blocking database write method

                    sprintf(sql, "BEGIN; "
                                    "INSERT INTO signals_log "
                                    "(uuid, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceUUID) "
                                    "VALUES ('%s','%s','%s','%s','%s','%s');",
                            uuid, requestedSignal, requestedSource, trueSignal, trueSource, trueSourceUUID);
                    idamLog(LOG_DEBUG, "Provenance: record() SQL\n%s\n", sql);

// Execute the SQL

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        err = 1;        // Roll back transaction
                        break;
                    }

                    PQclear(DBQuery);

// Return the Primary Key

                    strcpy(sql, "SELECT signals_log_id FROM signals_log WHERE "
                            "signals_log_id=currval('signals_log_id_seq');");

                    idamLog(LOG_DEBUG, "%s\n", sql);

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_TUPLES_OK) {
                        err = 1;        // Roll Back transaction
                        break;
                    }

                    nrows = PQntuples(DBQuery);

                    if (nrows != 1) {
                        idamLog(LOG_ERROR, "ERROR Provenance new: New signals_log record not found!\n");
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err,
                                     "New signals_log record not found!");
                        break;
                    }

// Extract the SQL data

                    stringLength = strlen(PQgetvalue(DBQuery, 0, 0)) + 1;
                    key = (char*) malloc(stringLength * sizeof(char));
                    strcpy(key, PQgetvalue(DBQuery, 0, 0));
                    PQclear(DBQuery);

                    idamLog(LOG_DEBUG, "Provenance key: %s\n", key);

// Complete the transaction

                    sprintf(sql, "END;");

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        free((void*) key);
                        key = NULL;
                        err = 1;
                        break;
                    }

                    PQclear(DBQuery);
                }

            } else

            if (status == 'u') {    // update an existing record using the key from the ADD step

                idamLog(LOG_DEBUG, "Provenance: record() update an existing record with the Server Log record\n");

                if (!logRecordOK) {
                    err = 999;
                    idamLog(LOG_ERROR, "ERROR Provenance add: No Log record!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err, "No Log record!");
                    break;
                }

                if (execMethodOK) {
                    if (execMethod == 1) {

                        if ((env = getenv("IDAM_CLI_SQL")) != NULL)
                            strcpy(cmd, env);
                        else
                            strcpy(cmd, "psql");

                        sprintf(&cmd[strlen(cmd)],
                                " -d %s -U %s -h %s -p %s -c \""
                                        "UPDATE signals_log SET logRecord = '%s' WHERE tmpKey = '%s';\" > /dev/null 2>&1 &",
                                dbname, user, pghost, pgport, logRecord, key);

                        idamLog(LOG_DEBUG, "Provenance: update() SQL\n%s\n", cmd);

                        gettimeofday(&tv_start, NULL);

                        errno = 0;
                        ph = popen(cmd, "r");

                        if (ph == NULL || errno != 0) {
                            execMethodOK = 0;        // Disable exec method
                            err = 999;
                            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", errno, "");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                                         "Cannot execute background SQL command");
                            break;
                        }

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 1 Cost B = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;

                    } else if (execMethod == 2) {

                        if (sqlSet == NULL) break;    // No first pass (due to a failed access) so ignore this step

                        gettimeofday(&tv_start, NULL);

                        fprintf(sqlSet, "UPDATE signals_log SET logRecord = '%s' WHERE tmpKey = '%s'; END;", logRecord,
                                key);
                        fclose(sqlSet);
                        sqlSet = NULL;

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 2 Cost B = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;

                        if ((env = getenv("IDAM_CLI_SQL")) != NULL)
                            strcpy(work, env);
                        else
                            strcpy(work, "psql");

                        idamLog(LOG_DEBUG, "Provenance: database parameters\n");
                        idamLog(LOG_DEBUG, "CLI     : %s\n", work);
                        idamLog(LOG_DEBUG, "db name : %s\n", dbname);
                        idamLog(LOG_DEBUG, "db user : %s\n", user);
                        idamLog(LOG_DEBUG, "db host : %s\n", pghost);
                        idamLog(LOG_DEBUG, "db port : %s\n", pgport);
                        idamLog(LOG_DEBUG, "cmd file: %s\n", tmpfile);

                        sprintf(cmd,
                                "%s -d %s -U %s -h %s -p %s -f %s > /dev/null 2>&1 &",
                                work, dbname, user, pghost, pgport, tmpfile);

                        idamLog(LOG_DEBUG, "Provenance: update() SQL\n%s\n", cmd);

                        gettimeofday(&tv_start, NULL);

                        errno = 0;
                        ph = popen(cmd, "r");

                        if (ph == NULL || errno != 0) {
                            execMethodOK = 0;        // Disable exec method
                            err = 999;
                            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", errno, "");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                                         "Cannot execute background SQL command");
                            break;
                        }

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 2 Cost C = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;

// Delete the SQL collection

                    } else

                    if (execMethod == 3) {

                        gettimeofday(&tv_start, NULL);

                        if ((env = getenv("IDAM_CLI_SQL")) != NULL)
                            strcpy(work, env);
                        else
                            strcpy(work, "psql");

                        sprintf(cmd,
                                "%s '%s'); END;\" "
                                        "> /dev/null 2>&1 &",
                                sqlBuffer, logRecord);

                        idamLog(LOG_DEBUG, "Provenance: update() SQL\n%s\n", cmd);

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 3 Cost B = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;

                        errno = 0;
                        ph = popen(cmd, "r");

                        if (ph == NULL || errno != 0) {
                            execMethodOK = 0;        // Disable exec method
                            err = 999;
                            if (errno != 0) addIdamError(&idamerrorstack, SYSTEMERRORTYPE, "Provenance", errno, "");
                            addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance", err,
                                         "Cannot execute background SQL command");
                            break;
                        }

                        gettimeofday(&tv_stop, NULL);
                        msecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec) / 1000;
                        usecs = (int) (tv_stop.tv_sec - tv_start.tv_sec) * 1000000 +
                                (int) (tv_stop.tv_usec - tv_start.tv_usec);
                        idamLog(LOG_DEBUG, "Provenance: update() execMethod 3 Cost C = %d (ms), %d (microsecs)\n",
                                msecs, usecs);
                        tv_start = tv_stop;
                    }

                } else {

                    sprintf(sql, "BEGIN; "
                            "UPDATE signals_log SET logRecord = '%s' WHERE signals_log_id = %s;"
                            "END;", logRecord, key);

                    idamLog(LOG_DEBUG, "Provenance: record() SQL\n%s\n", sql);

// Execute the SQL

                    if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                        PQclear(DBQuery);
                        idamLog(LOG_ERROR, "ERROR Provenance add: signals_log table update failed!\n");
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err,
                                     "signals_log table update failed!");
                        break;
                    }
                }

            } else if (status == 'c') {    // close all records for future deletion and execute collected SQL statements

                idamLog(LOG_DEBUG, "Provenance: record() Close all records\n");

                sprintf(sql, "BEGIN; "
                        "UPDATE signals_log SET status = 1 WHERE uuid = '%s';"
                        "END;", uuid);

                idamLog(LOG_DEBUG, "Provenance: record() SQL\n%s\n", sql);

// Execute the SQL

                if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                    PQclear(DBQuery);
                    idamLog(LOG_ERROR, "ERROR Provenance add: signals_log status update failed!\n");
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err,
                                 "signals_log status update failed!");
                    break;
                }

            } else if (status == 'd') {    // Delete closed records (Protection against malicious intent? user field?)
                idamLog(LOG_DEBUG, "Provenance: record() Delete closed records\n");

                sprintf(sql, "BEGIN; "
                        "DELETE FROM signals_log WHERE status = 1 AND uuid = '%s';" // and user='%s'
                        "END;", uuid);

                idamLog(LOG_DEBUG, "Provenance: record() SQL\n%s\n", sql);

// Execute the SQL

                if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                    PQclear(DBQuery);
                    idamLog(LOG_ERROR, "ERROR Provenance add: signals_log deletion failed!\n");
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance new", err, "signals_log deletion failed!");
                    break;
                }
            }


        } while (0);    // End of SQL Transaction Error Trap

        if (err != 0 && (!execMethodOK || status == 'd')) {    // Rollback the Transaction

            if (err == 1) {
                err = 999;
                if (DBQuery == NULL) {
                    idamLog(LOG_ERROR, "ERROR Provenance add: Database Query Failed!\n");
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err, "Database Query Failed!");
                } else if (PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                    idamLog(LOG_ERROR, "ERROR Provenance add: %s\n", PQresultErrorMessage(DBQuery));
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "Provenance add", err, PQresultErrorMessage(DBQuery));
                }
            }

            PQclear(DBQuery);

            sprintf(sql, "ROLLBACK; END;");
            idamLog(LOG_DEBUG, "%s\n", sql);

            DBQuery = PQexec(DBConnect, sql);
            PQclear(DBQuery);

            break;
        }

// Return 

        data_block->data_type = TYPE_INT;
        data_block->rank = 0;
        data_block->data_n = 1;
        data_block->data = (char*) malloc(sizeof(int));
        int* pi = (int*) data_block->data;
        pi[0] = 0;

        strcpy(data_block->data_desc, "data access logged against UUID");
        strcpy(data_block->data_label, "");
        strcpy(data_block->data_units, "");

        idamLog(LOG_DEBUG, "Provenance: exiting function record\n");

        break;

    } while (0);

    return err;
}

