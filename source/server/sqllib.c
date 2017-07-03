// Query the SQL Database for a User Specified Signal
//
// NOTE: Does Not Use the SQL ILIKE (case Insensitive) pattern matching operator
//       Leads to Ambiguity
//	 Signals contain _ character which is a single character wildcard in SQL syntax.
//
//-------------------------------------------------------------------------------------------------------------------

#include "sqllib.h"
#include "getServerEnvironment.h"

#include <stdlib.h>

#include <logging/logging.h>
#include <server/udaServer.h>
#include <clientserver/errorLog.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <modules/ida/nameIda.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>

// Open the Connection with the PostgreSQL IDAM Database

#ifndef NOTGENERICENABLED

PGconn* openDatabase(const char* host, int port, const char* dbname, const char* user)
{
    char pgport[56];
    sprintf(pgport, "%d", port);

//-------------------------------------------------------------
// Debug Trace Queries

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection: host %s\n", host);
    IDAM_LOGF(UDA_LOG_DEBUG, "                port %s\n", pgport);
    IDAM_LOGF(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    IDAM_LOGF(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    PGconn* DBConnect = NULL;

    if ((DBConnect = PQsetdbLogin(host, pgport, NULL, NULL, dbname, user, NULL)) == NULL) {
        IDAM_LOG(UDA_LOG_DEBUG, "SQL Server Connect Error");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        IDAM_LOG(UDA_LOG_DEBUG, "Bad SQL Server Connect Status");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return DBConnect;

}

PGconn* startSQL()
{
    const ENVIRONMENT* environment = getIdamServerEnvironment();

    const char* pghost = environment->sql_host;
    const char* dbname = environment->sql_dbname;
    const char* user = environment->sql_user;
    char* pswrd = NULL;
    char pgport[56];

    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

    PGconn* DBConnect = NULL;

    sprintf(pgport, "%d", environment->sql_port);

//-------------------------------------------------------------
// Debug Trace Queries

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection: host %s\n", pghost);
    IDAM_LOGF(UDA_LOG_DEBUG, "                port %s\n", pgport);
    IDAM_LOGF(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    IDAM_LOGF(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL) {
        IDAM_LOG(UDA_LOG_DEBUG, "SQL Server Connect Error");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        IDAM_LOG(UDA_LOG_DEBUG, "Bad SQL Server Connect Status");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return (DBConnect);
}

PGconn* startSQL_CPF()
{
    const ENVIRONMENT* environment = getIdamServerEnvironment();

    const char* pghost = environment->sql_host;
    const char* dbname = environment->sql_dbname;
    const char* user = environment->sql_user;
    char* pswrd = NULL;
    char pgport[56];

    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

    PGconn* DBConnect = NULL;

    sprintf(pgport, "%d", environment->sql_port);

//-------------------------------------------------------------
// Debug Trace Queries

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection: host %s\n", pghost);
    IDAM_LOGF(UDA_LOG_DEBUG, "                port %s\n", pgport);
    IDAM_LOGF(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    IDAM_LOGF(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    if ((DBConnect = PQsetdbLogin(pghost, pgport, pgoptions, pgtty, dbname, user, pswrd)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "SQL Server Connect Error");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "startSQL", 1, "Bad SQL Server Connect Status");
        PQfinish(DBConnect);
        return NULL;
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return (DBConnect);
}


void sqlReason(PGconn* DBConnect, char* reason_id, char* reason)
{
    int nrows, ncols;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Build SQL

    reason[0] = '\0';

    strcpy(sql, "SELECT reason_id, description FROM Reason WHERE reason_id = ");
    strcat(sql, reason_id);

//fprintf(stdout,"%s\n",sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        strcpy(reason, "Failure to Execute SQL: ");
        strcat(reason, PQresultErrorMessage(DBQuery));
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlReason", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return;
    }

    nrows = PQntuples(DBQuery);  // Number of Rows
    ncols = PQnfields(DBQuery);     // Number of Columns

    if (nrows == 1 && ncols == 2) {
        strcpy(reason, PQgetvalue(DBQuery, 0, 1));
    } else {
        strcpy(reason, "Unknown Reason");
    }

    PQclear(DBQuery);

    return;
}


void sqlResult(PGconn* DBConnect, char* run_id, char* desc)
{

    int nrows;
    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Build SQL

    desc[0] = '\0';

    strcpy(sql, "SELECT run_id, description FROM Result WHERE run_id = ");
    strcat(sql, run_id);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        strcpy(desc, "Failure to Execute SQL: ");
        strcat(desc, PQresultErrorMessage(DBQuery));
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlResult", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return;
    }

    nrows = PQntuples(DBQuery); // Number of Rows

    if (nrows == 1) strcpy(desc, PQgetvalue(DBQuery, 0, 1));

    PQclear(DBQuery);

    return;
}


void sqlStatusDesc(PGconn* DBConnect, char* status_desc_id, char* desc)
{

    int nrows;
    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Build SQL

    desc[0] = '\0';

    strcpy(sql, "SELECT status_desc_id, description FROM Status_Desc WHERE status_desc_id = ");
    strcat(sql, status_desc_id);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        strcpy(desc, "Failure to Execute SQL: ");
        strcat(desc, PQresultErrorMessage(DBQuery));
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlStatusDesc", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return;
    }

    nrows = PQntuples(DBQuery); // Number of Rows

    if (nrows == 1) strcpy(desc, PQgetvalue(DBQuery, 0, 1));

    PQclear(DBQuery);

    return;
}


void sqlMeta(PGconn* DBConnect, char* table, char* meta_id, char* xml, char* creation)
{

    int nrows, ncols;
    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Test for the Default Value of the Meta Table Primary Key

    xml[0] = '\0';
    creation[0] = '\0';

    if (STR_EQUALS(meta_id, "0")) return;

//-------------------------------------------------------------
// Build SQL

    strcpy(sql, "SELECT meta_id, xml, creation FROM ");
    strcat(sql, table);
    strcat(sql, " WHERE meta_id = ");
    strcat(sql, meta_id);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        strcpy(xml, "<ERROR>Failure to Execute SQL: ");
        strcat(xml, PQresultErrorMessage(DBQuery));
        strcat(xml, "</ERROR>");
        creation[0] = '\0';
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMeta", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return;
    }

    nrows = PQntuples(DBQuery); // Number of Rows
    ncols = PQnfields(DBQuery); // Number of Columns

    if (nrows == 1 && ncols == 3) {
        strcpy(xml, PQgetvalue(DBQuery, 0, 1));
        strcpy(creation, PQgetvalue(DBQuery, 0, 2));
    } else {
        creation[0] = '\0';
        if (nrows == 0)
            strcpy(xml, "<ERROR>No Meta Data Record Found!</ERROR>");
        else
            strcpy(xml, "<ERROR> Too Many Meta Data Records Found!</ERROR>");
    }

    PQclear(DBQuery);
    return;
}

static int preventSQLInjection(PGconn* DBConnect, char** from)
{

// Replace the passed string with an Escaped String
// Free the Original string from Heap

    int err = 0;
    size_t fromCount = strlen(*from);
    char* to = (char*)malloc((2 * fromCount + 1) * sizeof(char));
    PQescapeStringConn(DBConnect, to, *from, fromCount, &err);
    if (err != 0) {
        if (to != NULL) free((void*)to);
        return 1;
    }
    free((void*)*from);
    *from = to;
    return 0;
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
//
// Change History
//
// 19Jan2007	dgm	If the device is MAST and the archive is MAST and the shot number is prior to M4
//			then avoid the database lookup for a signal and data source file: Look up the standard
//                      signal description only. Assume this is an IDA signal and use the name to locate the data
//                      source file.
// 29Mar2007	dgm	Test request_block.tpass for LATEST pass request
// 15Feb2008	dgm	Text based Pass string added
// 28Feb2008	dgm	Test for Length of Signal Name: Must have a length!
// 29Oct2009	dgm	Changed to selection using source_alias (uniqueness constraint changed to compound signal_name
//			AND source_alias). Make all names upper case.
// 11Jun2010	dgm	Split the 3 table join into two steps for improved performance: source+signal_desc then signal.
//			Include case sensitivity check.
// 18Feb2011	dgm	Changed query when signal_alias_type=2 or 3 - compare in lower case only: All recorded
//                      signals should be unique regardless of case for the same source alias.
// 21Feb2011	dgm	Removed use of the Generic_name field for mapped signal names. New table 'signal_map' added.
// 23Feb2011	dgm	Improved matching of case sensitive names: performance + exception handling.
// 23Mar2011	dgm	If the signal begins 'transp/' then prefix a '/' character. Required for legacy reasons.
//-------------------------------------------------------------------------------------------------------------

int sqlSignalDescMap(PGconn* DBConnect, char* originalSignal, int exp_number, int pass, char* originalTPass,
                     int* source_id, SIGNAL_DESC* signal_desc)
{

// The uniqueness constraint for the Signal_Desc table is signal_name + source_alias.
// The signal name is case sensitive because data may be recorded using a case sensitive name and the API requires it.
// Uniqueness is not broken if the name is the same but the case is different.
// The signal_alias does not preserve case sensitivity and will have the source alias prefixed when missing from the signal_name.
// To aid performance, the signal_alias field entries are all Upper case.
// There is no constraint on the uniqueness of the signal_alias so multiple version of the same name can exist.
//
// The correct signal, if ambiguous, is identified when the Data_Source record is matched against the Signal record.
//
// Primarily select a Signal_Desc record by matching the signal name against either the Signal_Alias field or the
// Generic_Name field (constrained by an exp_number range). If no records are found, the signal does not exist in the
// database. If 1 record is found, there is no ambiguity and the exact signal required is found. If more than 1 record
// was found then potentially an exception error has occured because of non-uniqueness. Each match is tested for correctness
// using the data_source and signal tables. Only one match is expected, otherwise an exception error has occured.
//
// A check is made on the signal_map_id field of the selected record. This points to the Signal_Map table which records which
// signal names are mapped to other signal names. These mappings are shot range specific. If the field is populated
// (>0), details of the mapping are extracted and the constraint checked. If the mapped signal also has a mapping, this is
// followed. The last valid mapping is accepted and the details of this signal returned. Tests are made to avoid possible infinite loops!
//
// Signal mappings to Analysed Type data are complicated because of the volatile nature of the Pass number. A Pass constraint
// is not viable - Analysed data should be reprocessed to correct data or the signal's status value set to 'BAD'. (A Pass number
// only applies to Analysed data - not to Raw data.)

// If the Return Code is 0 then a Problem occured: Either the Signal is not recognised or an Exception has occured!

    int nrows, ncols, j, rc = 0, cost, lstr, prefixAdded = 0;
    char* us, * pus;

    PGresult* DBQuery = NULL;

    struct timeval tv_start, tv_end;

    char sql[MAXSQL];

    *source_id = 0;    // If no abiguity then unchanged

//-------------------------------------------------------------
// Escape SIGNAL and TPASS to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDescMap", err, "Unable to Escape the signal name!");
        return 0;
    }

    char* tpass = (char*)malloc((strlen(originalTPass) + 1) * sizeof(char));
    strcpy(tpass, originalTPass);
    if (preventSQLInjection(DBConnect, &tpass)) {
        if (signal != NULL) free((void*)signal);
        if (tpass != NULL) free((void*)tpass);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDescMap", err, "Unable to Escape the tpass string!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Returned Structure

    initSignalDesc(signal_desc);

//-------------------------------------------------------------
// Note on Mixed Case Sensitivity and Prefix: Field signal_alias_type
//
// All signal_alias and generic names should be recorded in single case
// Upper case is chosen as the convention.
//
//	NOPREFIXNOCASE - Bit pattern 00 (0)	=> 	Default alias name type: No Prefix added, case is not important.
//
//	PREFIXNOCASE - Bit pattern 01 (1)	=> 	Source Alias Prefix added, case is not important as above.
//
//	NOPREFIXCASE - Bit pattern 10 (2)	=> 	No Prefix added, but case is important.
//
//	PREFIXCASE - Bit pattern 11 (3)		=> 	Prefix added, case is also important.
//
// For Performance, avoid operators ILIKE etc or casting table fields to upper/lower etc.. Use = instead where possible.
//
//-------------------------------------------------------------
// Note on Identifying the correct record using alias and generic names.
//
// Signal_Desc records have a uniqueness constraint: signal_name+data_source_alias
// Scheduler codes use this to test whether or not a record exists for any signal being processed.
// Scheduler codes do NOT check the signal_alias field.
//
// Signal_alias names (which might not be unique) are used by users to identify the correct signal and data source file.
// There are no shot dependencies because of the uniqueness constraint.
//
// Generic names are short cut names used by users to identify a signal_name.
// Generic names should be unique and case insensitive. They MUST NOT match a signal_alias entry.
// Generic names have a validity constraint: shot number range (not a pass number range)
// Generic names can be shared with multiple signal_name records, but only with non overlapping shot ranges.
// The RANK field is set to 1 if the Generic_Name field is populated, 0 otherwise.
//
// If generic name and constraint records are not unique, the selected signal_desc record cannot be guaranteed to be the
// correct record wanted.
//
// For legacy reasons, TRANSP alias names originally did not begin with a '/'. However, this is now the standard.
// To accommodate this, client specified alias names beginning 'transp/' are changed to '/transp/' in the SQL. This is the
// only exception: all other names must begin '/' when this is required to identify the correct signal alias.
//
//-------------------------------------------------------------
// Summary of table statistics on 24Feb2011: 18322 records
// Generic signals: 131	records all Upper case
// Signal_alias_type 0: 16718 - Single case records (5762 lower; 11004 upper)
// Signal_alias_type 1: 820 - Single case (193 lower, 627 upper)
// Signal_alias_type 2: 769 - Mixed case
// Signal_alias_type 3: 15 - Mixed case
//-------------------------------------------------------------
// Build SQL

    lstr = strlen(signal) * sizeof(char) + 1;
    us = (char*)malloc(lstr);
    strcpy(us, signal);
    strupr(us);            // Upper Case signal Name

// Beginning '/' or 'transp/' ?

    pus = NULL;
    if (signal[0] != '/' && STR_EQUALS(us, "TRANSP/")) {
        prefixAdded = 1;
        pus = (char*)malloc(lstr + 1);
        sprintf(pus, "/%s", us);
    }

//-----
// 7ms  (signal_alias and generic_name are Upper Case)

    if (prefixAdded) {
        sprintf(sql, "SELECT signal_desc_id, meta_id, rank, range_start, range_stop, type, source_alias, "
                "signal_alias, signal_name, generic_name, description, signal_class, "
                "signal_owner, creation, modified, signal_alias_type, signal_map_id "
                " FROM signal_desc WHERE signal_alias = '%s' OR signal_alias = '%s' OR "
                "(generic_name = '%s' AND "
                "((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=%d) OR "
                " (range_stop=0  AND range_start<=%d) OR "
                " (range_start>0 AND range_start<=%d AND range_stop>0 AND range_stop>=%d)))"
                " ORDER BY rank DESC;",
                us, pus, us, exp_number, exp_number, exp_number, exp_number);
        if (pus != NULL) free((void*)pus);
    } else {
        sprintf(sql, "SELECT signal_desc_id, meta_id, rank, range_start, range_stop, type, source_alias, "
                "signal_alias, signal_name, generic_name, description, signal_class, "
                "signal_owner, creation, modified, signal_alias_type, signal_map_id "
                " FROM signal_desc WHERE signal_alias = '%s' OR "
                "(generic_name = '%s' AND "
                "((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=%d) OR "
                " (range_stop=0  AND range_start<=%d) OR "
                " (range_start>0 AND range_start<=%d AND range_stop>0 AND range_stop>=%d)))"
                " ORDER BY rank DESC;",
                us, us, exp_number, exp_number, exp_number, exp_number);
    }

    free((void*)us);

    cost = gettimeofday(&tv_start, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDescMap", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        if (tpass != NULL) free((void*)tpass);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlSignalDescMap +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Rows: %d\n", nrows);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Cols: %d\n", ncols);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Msg : %s\n", PQresultErrorMessage(DBQuery));
    tv_start = tv_end;

    if (nrows == 0) {        // Nothing matched!
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        if (tpass != NULL) free((void*)tpass);
        return rc;
    }

//-------------------------------------------------------------
// Multiple records: Process Exception and return error
//
// nrows == 0 => no match
// nrows == 1 => unambiguous match to signal_alias or generic_name. This is the target group for best performance.
// nrows >= 2 => ambiguous match. Test all possibilites. Only 1 should match the Data_Source and Signal table entries.

// Problems:
// select * from (select signal_alias, count(*) from signal_desc group by signal_alias order by count)
// as a where a.count>=2

    int match = 0, matchCount = 0, matchId = 0, i, ok = 1;

// Test all possibilites for an exception error.

    if (nrows > 1) {

        char* source_alias, * type;
        int signal_desc_id;

// Match using relational entries between signal_desc, data_source and signal tables.

        if (ok) {
            for (i = 0; i < nrows; i++) {
                source_alias = PQgetvalue(DBQuery, i, 6);
                type = PQgetvalue(DBQuery, i, 5);
                signal_desc_id = atoi(PQgetvalue(DBQuery, i, 0));
                match = sqlMatch(DBConnect, signal_desc_id, source_alias, type, exp_number, pass, tpass, source_id);
                if (match) {
                    matchId = i;
                    matchCount++;
                    if (matchCount > 1)break;        // Error - multiple matches
                }
            }
        }

// Report Error: No match found or Inconsistency (multiple matches)

        if (!ok || matchCount > 1) {
            char sqle[MAXSQL];
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDescMap", 999,
                         "Ambiguous signal description database entry found! "
                                 "Please advise the System Administrator.");
            PQescapeString(sqle, sql, (size_t)strlen(sql));
            sprintf(sql, "INSERT INTO Signal_Desc_Exception (exception_id, nrows, signal, exp_number, sql) VALUES("
                    "nextval('signal_desc_exception_id_seq'), %d, '%s', %d, '%s');", nrows, signal, exp_number, sqle);
            PQclear(DBQuery);
            if ((DBQuery = PQexec(DBConnect, sql)) == NULL || PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDescMap", 1, PQresultErrorMessage(DBQuery));
            }
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            if (tpass != NULL) free((void*)tpass);
            return 0;
        }
    }

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlSignalDescMap +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    tv_start = tv_end;

//-------------------------------------------------------------
// Process First Record

    if (matchId != -1) {

        rc = 1;
        j = 0;

// Signal_Desc fields

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->signal_desc_id = atoi(PQgetvalue(DBQuery, matchId, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->meta_id = atoi(PQgetvalue(DBQuery, matchId, j));
            sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, matchId, j++), signal_desc->xml, signal_desc->xml_creation);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->rank = atoi(PQgetvalue(DBQuery, matchId, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->range_start = atoi(PQgetvalue(DBQuery, matchId, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->range_stop = atoi(PQgetvalue(DBQuery, matchId, j++));
        } else { j++; }

        signal_desc->type = *PQgetvalue(DBQuery, matchId, j++);

        strcpy(signal_desc->source_alias, PQgetvalue(DBQuery, matchId, j++));

        strcpy(signal_desc->signal_alias, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->signal_name, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->generic_name, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->description, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->signal_class, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->signal_owner, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->creation, PQgetvalue(DBQuery, matchId, j++));
        strcpy(signal_desc->modified, PQgetvalue(DBQuery, matchId, j++));

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->signal_alias_type = atoi(PQgetvalue(DBQuery, matchId, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, matchId, j)) > 0) {
            signal_desc->signal_map_id = atoi(PQgetvalue(DBQuery, matchId, j++));            // Map signal Name
        } else { j++; }

        PQclear(DBQuery);

// Does this signal map to a different signal for this shot

        if (signal_desc->signal_map_id > 0) {        // This key is not used - it only flags that a mapping exists.
            SIGNAL_DESC signal_desc_map;
            initSignalDesc(&signal_desc_map);

            if (!sqlMapData(DBConnect, signal_desc->signal_desc_id, exp_number, &signal_desc_map)) {
                if (signal != NULL) free((void*)signal);
                if (tpass != NULL) free((void*)tpass);
                return 0;                    // Signal Description Record
            }

            if (signal_desc_map.signal_desc_id != 0) {        // Mapping found
                *signal_desc = signal_desc_map;
            }
        }

    } else {
        PQclear(DBQuery);
    }

    gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlSignalDescMap +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    tv_start = tv_end;

    if (signal != NULL) free((void*)signal);
    if (tpass != NULL) free((void*)tpass);

    return rc;
}

int sqlDataSourceMap(PGconn* DBConnect, int exp_number, int pass, char* originalTPass, int* source_id,
                     DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc)
{

// If the Return Code is 0 then a Problem occured: Perhaps there is No Source File !

    int nrows, ncols, j, cost, rc = 0, err;
    int status_reason_impact_code;
    char* latest = NULL;
    char maxpass[MAXKEY] = "";

    PGresult* DBQuery = NULL;

    struct timeval tv_start, tv_end;

    char sql0[MAXSQL];
    char sql[MAXSQL];

//-------------------------------------------------------------
// Escape TPASS to protect against SQL Injection

    char* tpass = (char*)malloc((strlen(originalTPass) + 1) * sizeof(char));
    strcpy(tpass, originalTPass);
    if (preventSQLInjection(DBConnect, &tpass)) {
        if (tpass != NULL) free((void*)tpass);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceMap", err, "Unable to Escape the tpass string!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Returned Structure

    initDataSource(data_source);

//-------------------------------------------------------------
// Resolve the tpass argument given the Identified signal

    if (*source_id == 0 && strlen(tpass) > 0) {
        if (pass == -1) {
            if ((latest = (char*)strcasestr(tpass, "LATEST")) != NULL) {
                if (strlen(latest) > 6) {
                    if (!sqlLatestPass(DBConnect, signal_desc->source_alias, signal_desc->type, exp_number, maxpass)) {
                        err = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceMap", err,
                                     "Unable to Identify the Latest (i.e. Maximum) Pass Number");
                        if (tpass != NULL) free((void*)tpass);
                        return 0;
                    }
                    latest = latest + 6;        // Only need the remaining text for the SQL query
                } else {
                    latest = NULL;            // Latest is the default behaviour so no need to do anything!
                }
            } else {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceMap", err,
                             "The Pass String does not contain the LATEST directive when one is expected - Please correct");
                if (tpass != NULL) free((void*)tpass);
                return 0;
            }
        } else {
            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceMap", err,
                         "The Pass number requested is Unclear - Please Correct");
            if (tpass != NULL) free((void*)tpass);
            return 0;
        }
    }


//-------------------------------------------------------------
// Build SQL

    if (*source_id == 0) {
        sprintf(sql, "SELECT source_id, reason_id, run_id,  meta_id, status_desc_id, exp_number, pass, source_status, "
                "status_reason_impact_code, type, source_alias, pass_date, format, path, "
                "filename, server, creation, modified FROM Data_Source WHERE exp_number=%d AND "
                "source_alias='%s' AND type='%c' ",
                exp_number, signal_desc->source_alias, signal_desc->type);

        sql0[0] = '\0';
        if (pass > -1) {
            sprintf(sql0, "AND pass = %d", pass);
        } else {
            if (latest != NULL) sprintf(sql0, "AND pass = %s%s", maxpass, latest);
        }
        if (sql0[0] != '\0') {
            strcat(sql, sql0);
        } else {
            strcat(sql, " ORDER BY pass DESC;");        // Highest pass is default (LATEST)
        }
    } else {
        sprintf(sql, "SELECT source_id, reason_id, run_id,  meta_id, status_desc_id, exp_number, pass, source_status, "
                "status_reason_impact_code, type, source_alias, pass_date, format, path, "
                "filename, server, creation, modified FROM Data_Source WHERE source_id=%d;",
                *source_id);
    }

//-------------------------------------------------------------
// Test Performance

    cost = gettimeofday(&tv_start, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceMap", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (tpass != NULL) free((void*)tpass);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlDataSourceMap +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Rows: %d\n", nrows);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Cols: %d\n", ncols);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Msg : %s\n", PQresultErrorMessage(DBQuery));

//-------------------------------------------------------------
// Process First Record

    if (nrows >= 1) {

        rc = 1;
        j = 0;

// Data_source fields

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->source_id = atoi(PQgetvalue(DBQuery, 0, j++));
            *source_id = data_source->source_id;
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->reason_id = atoi(PQgetvalue(DBQuery, 0, j));
            sqlReason(DBConnect, PQgetvalue(DBQuery, 0, j++), data_source->reason_desc);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->run_id = atoi(PQgetvalue(DBQuery, 0, j));
            sqlResult(DBConnect, PQgetvalue(DBQuery, 0, j++), data_source->run_desc);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->meta_id = atoi(PQgetvalue(DBQuery, 0, j));
            sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, j++), data_source->xml, data_source->xml_creation);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->status_desc_id = atoi(PQgetvalue(DBQuery, 0, j));
            data_source->status_desc[0] = '\0';
            j++;                    // Need a Table Query function
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->exp_number = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->pass = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            data_source->status = atoi(PQgetvalue(DBQuery, 0, j++));
        } else {
            data_source->status = DEFAULT_STATUS;            // Source Status Default
            j++;
        }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            status_reason_impact_code = atoi(PQgetvalue(DBQuery, 0, j++));    // code = 100*reason + impact;
            data_source->status_reason_code = status_reason_impact_code / 100;
            data_source->status_impact_code = status_reason_impact_code - 100 * data_source->status_reason_code;
        } else { j++; }

        data_source->type = *PQgetvalue(DBQuery, 0, j++);

        strcpy(data_source->source_alias, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->pass_date, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->format, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->path, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->filename, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->server, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->creation, PQgetvalue(DBQuery, 0, j++));
        strcpy(data_source->modified, PQgetvalue(DBQuery, 0, j++));

    }

    PQclear(DBQuery);
    if (tpass != NULL) free((void*)tpass);

    return rc;
}


int sqlSignal(PGconn* DBConnect, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, SIGNAL* signal)
{

// If the Return Code is 0 then a Problem occured: Perhaps there is No Signal record!

    int nrows, ncols, j, rc = 0, cost;
    int status_reason_impact_code;

    PGresult* DBQuery = NULL;

    struct timeval tv_start, tv_end;

    char sql[MAXSQL];
    char* sql0 = "source_id,signal_desc_id,meta_id,status_desc_id,signal_status,status_reason_impact_code,creation,modified";

//-------------------------------------------------------------
// Initialise Returned Structure

    initSignal(signal);

//-------------------------------------------------------------
// Build SQL

    sprintf(sql, "SELECT %s FROM signal WHERE source_id=%d AND signal_desc_id=%d LIMIT 1;",
            sql0, data_source->source_id, signal_desc->signal_desc_id);

//-------------------------------------------------------------
// Test Performance

    cost = gettimeofday(&tv_start, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignal", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlSignal +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Rows: %d\n", nrows);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Cols: %d\n", ncols);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Msg : %s\n", PQresultErrorMessage(DBQuery));

//-------------------------------------------------------------
// Process Record

    if (nrows == 1) {

        rc = 1;
        j = 0;

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal->source_id = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal->signal_desc_id = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal->meta_id = atoi(PQgetvalue(DBQuery, 0, j));
            sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, j++), signal->xml, signal->xml_creation);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal->status_desc_id = atoi(PQgetvalue(DBQuery, 0, j));
            signal->status_desc[0] = '\0';
            j++;                    // Need a Table Query function
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal->status = atoi(PQgetvalue(DBQuery, 0, j++));
        } else {
            signal->status = DEFAULT_STATUS;            // Signal Status Default
            j++;
        }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            status_reason_impact_code = atoi(PQgetvalue(DBQuery, 0, j++));    // code = 100*reason + impact;
            signal->status_reason_code = status_reason_impact_code / 100;
            signal->status_impact_code = status_reason_impact_code - 100 * signal->status_reason_code;
        } else { j++; }

        strcpy(signal->creation, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal->modified, PQgetvalue(DBQuery, 0, j++));

    }

    PQclear(DBQuery);

    return rc;
}


int sqlGeneric(PGconn* DBConnect, char* originalSignal, int exp_number, int pass, char* originalTPass,
               SIGNAL* signal_str, SIGNAL_DESC* signal_desc,
               DATA_SOURCE* data_source)
{

// If the Return Code is 0 then a Problem occured.

    int rc, rc1 = 0, rc2 = 0, rc3 = 0, cost, source_id;

    struct timeval tv_start, tv_end;

//-------------------------------------------------------------
// Fundamental Test: If no signal then no generic lookup based on signal is possible

    if (strlen(originalSignal) == 0) return 0;

//-------------------------------------------------------------
// Escape SIGNAL and TPASS to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlGeneric", err, "Unable to Escape the signal name!");
        return 0;
    }

    char* tpass = (char*)malloc((strlen(originalTPass) + 1) * sizeof(char));
    strcpy(tpass, originalTPass);
    if (preventSQLInjection(DBConnect, &tpass)) {
        if (signal != NULL) free((void*)signal);
        if (tpass != NULL) free((void*)tpass);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlGeneric", err, "Unable to Escape the tpass string!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Structures

    cost = gettimeofday(&tv_start, NULL);

//-------------------------------------------------------------
// Locate the Data Source, Signal Description and the Signal Records

    rc1 = sqlSignalDescMap(DBConnect, signal, exp_number, pass, tpass, &source_id, signal_desc);

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlGeneric +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    tv_start = tv_end;

// If no record was found and the signal complies with netcdf naming convention but missing a leading '/' character, then
// prepend and try again.

    if (!rc1 && signal[0] != '/' && signal[3] == '/' && exp_number >= MAST_STARTPULSE) {
        char* p = (char*)malloc((strlen(signal) + 2) * sizeof(char));
        sprintf(p, "/%s", signal);
        rc1 = sqlSignalDescMap(DBConnect, p, exp_number, pass, tpass, &source_id, signal_desc);
        free((void*)p);
    }

// Composite Signals and Plugin data don't have sources

    if (rc1 && (signal_desc->type != 'C' && signal_desc->type != 'P')) {
        rc2 = sqlDataSourceMap(DBConnect, exp_number, pass, tpass, &source_id, data_source, signal_desc);
    }

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlGeneric +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    tv_start = tv_end;

    if (rc1 && rc2) rc3 = sqlSignal(DBConnect, data_source, signal_desc, signal_str);

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlGeneric +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    tv_start = tv_end;

    rc = rc1 && rc2 && rc3;

// No associated data source file to identify (Composite Signals and Plugin data)

    if (!rc && rc1 && (signal_desc->type == 'C' || signal_desc->type == 'P')) {
        rc = 1;
    }

//-------------------------------------------------------------
// Check whether or not the Database holds some useful data for this pulse number or signal name.

// Final Attempt: Scenarios
//
// 1> No Signal_Desc record: No Data are available (but might be an attribute value!) if exp_number >= IDAM_STARTPULSE
// 2> No Data_Source record but Signal_Desc record exists: No Data are available if exp_number < IDAM_STARTPULSE
// 3> No Signal record but both Signal_Desc and Data_Source records exist: No Data are available
// 4> No Signal_Desc record: Signal might be an attribute value! if exp_number >= IDAM_STARTPULSE

// Ignore OLD Shots as NOT IN Database: Use shot, pass and alias information to identify the source

    if (!rc && exp_number > 0 && exp_number < MAST_STARTPULSE) {
        if (!rc1) {            // Scenario 1
            rc = sqlNoIdamSignal(DBConnect, signal, exp_number, pass, tpass, signal_str, signal_desc, data_source);
        } else {
            if (rc1 && !rc2) {    // Scenario 2
                rc = sqlNoIdamSignal(DBConnect, signal, exp_number, pass, tpass, signal_str, signal_desc, data_source);
            }
        }
    } else {            // Scenario 4
        if (!rc1)rc = sqlNoIdamSignal(DBConnect, signal, exp_number, pass, tpass, signal_str, signal_desc, data_source);
    }

// Is it a Document - a Binary (Image) or ASCII file ?

    if (!rc && !rc1) rc = sqlDocument(DBConnect, signal, exp_number, pass, signal_desc, data_source);

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlGeneric +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);

    if (signal != NULL) free((void*)signal);
    if (tpass != NULL) free((void*)tpass);

    return rc;
}


int sqlNoIdamSignal(PGconn* DBConnect, char* originalSignal, int exp_number, int pass, char* originalTPass,
                    SIGNAL* signal_str, SIGNAL_DESC* signal_desc, DATA_SOURCE* data_source)
{
//
// This is a last resort attempt to locate data because there is no signal_desc or data_source entries in the database
//
// If the Return Code is 0 then a Problem occured!
//

    int nrows, rc = 0;
    int netcdf = 0;        // Assume the data is either IDA or netCDF4
    char prefix[4];
    char sql[MAXSQL];
    PGresult* DBQuery = NULL;
    static int noRecursion = 0;    // Disable recursive calls

//-------------------------------------------------------------
// Escape SIGNAL and TPASS to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignal", err, "Unable to Escape the signal name!");
        return 0;
    }

    char* tpass = (char*)malloc((strlen(originalTPass) + 1) * sizeof(char));
    strcpy(tpass, originalTPass);
    if (preventSQLInjection(DBConnect, &tpass)) {
        if (signal != NULL) free((void*)signal);
        if (tpass != NULL) free((void*)tpass);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignal", err, "Unable to Escape the tpass string!");
        return 0;
    }
//-------------------------------------------------------------
// Was a Signal_desc record found?

    if (signal_desc->signal_desc_id > 0) {

        rc = 1;

        if (tpass[0] != '\0') rc = 0;    // Unable to resolve tpass field

// Fill Data Source fields (LATEST pass assumed unless specific pass identified)

        data_source->exp_number = exp_number;
        data_source->pass = pass;

        data_source->type = signal_desc->type;
        strcpy(data_source->source_alias, signal_desc->source_alias);

        if (pass >= 0) {
            sprintf(data_source->path, "%s/%03d/%d/Pass%d", getenv("MAST_DATA"), exp_number / 1000, exp_number, pass);
        } else {
            sprintf(data_source->path, "%s/%03d/%d/LATEST", getenv("MAST_DATA"), exp_number / 1000, exp_number);
        }

        if (strlen(signal) > 4 && signal[3] == '_') {
            strcpy(data_source->format, FORMAT_LEGACY);        // Default Legacy file format
            if (STR_EQUALS(data_source->format, "IDA3")) {
                nameIDA(signal_desc->source_alias, exp_number, data_source->filename);
                strlwr(data_source->filename);            // Ensure lower case
                strcat(data_source->path, "/");
                strcat(data_source->path, data_source->filename);    // Add the filename to the Path
            } else {
                rc = 0;            // Don't have a file name model
            }
        } else {
            strcpy(data_source->format, FORMAT_MODERN);        // Default Modern file format
            if (STR_EQUALS(data_source->format, "CDF")) {
                sprintf(data_source->filename, "%s%06d.nc", signal_desc->source_alias, exp_number);
                strlwr(data_source->filename);            // Ensure lower case
                strcat(data_source->path, "/");
                strcat(data_source->path, data_source->filename);    // Add the filename to the Path
            } else {
                rc = 0;            // Don't have a file name model
            }
        }

// Signal fields

        signal_str->signal_desc_id = signal_desc->signal_desc_id;
        signal_str->source_id = 0;

    } else {

// No Signal_Desc Record Found

// Signal Names missing from the IDAM database may be because:
//	1> They are from a very early shot
//	2> They were not captured for some reason by the IDAM scheduler code
//	3> The name is an attribute name - not recorded in the database (signal/variable name only)
//	3.1> Group level attribute
//	3.2> Variable attribute using a 'dot' notation
//	4> The leading putdata group name character '/' is missing.
//	5> The IDA naming convention (alias_tag) has not been used: assume a / character

        rc = 1;

// Identify the Source alias from the signal

        if (signal[0] != '/' && signal[3] == '_') {
            strncpy(prefix, signal, 3);        // Probably IDA		Macro FORMAT_LEGACY
            prefix[3] = '\0';
            netcdf = 0;
            if (tpass[0] != '\0') rc = 0;    // Unable to resolve tpass field
        } else {
            char* p = strchr(&signal[1], '/');
            if (p != NULL) {
                int lp = p - &signal[1];
                if (signal[0] == '/') {
                    strncpy(prefix, &signal[1], lp);    // Probably netCDF4	Macro FORMAT_MODERN
                    prefix[lp] = '\0';
                    netcdf = 1;
                    if (tpass[0] != '\0') {
                        rc = 0;            // Unable to resolve tpass field
                    } else {
                        if ((p = strrchr(signal, '.')) != NULL) {    // Variable level attribute
                            char* attribute = (char*)malloc((strlen(p) + 1) * sizeof(char));
                            strcpy(attribute, &p[1]);
                            p[0] = '\0';                // Remove attribute
                            if (noRecursion == 0) {            // Does the variable exists?
                                noRecursion = 1;
                                rc = sqlGeneric(DBConnect, signal, exp_number, pass, tpass, signal_str, signal_desc,
                                                data_source);
                                noRecursion = 0;
                                if (rc) {                // If found the re-attach the attribute name
                                    strcat(signal_desc->signal_name, ".");
                                    strcat(signal_desc->signal_name,
                                           attribute);        // Put the attribute name back into the signal string
                                    strcat(signal_desc->signal_alias, ".");
                                    strcat(signal_desc->signal_alias, attribute);
                                    free((void*)attribute);
                                    return rc;
                                }
                                strcat(signal, ".");
                                strcat(signal, attribute);    // Put the attribute name back into the signal string
                                free((void*)attribute);
                            }
                        }
                    }
                } else {                // Assume Missing leading '/' character
                    if (noRecursion == 0) {
                        noRecursion = 1;
                        char* prependedSignal = (char*)malloc((strlen(signal) + 2) * sizeof(char));
                        sprintf(prependedSignal, "/%s", signal);
                        rc = sqlGeneric(DBConnect, prependedSignal, exp_number, pass, tpass, signal_str, signal_desc,
                                        data_source);
                        free((void*)prependedSignal);
                        noRecursion = 0;
                        return rc;
                    }
                    //noRecursion = 0;
                    rc = 0;
                }
            } else {
                rc = 0;                // Unable to identify the source alias name
            }
        }

        if (rc) {

// Preserve the case of prefix as not known: user needs to be accurate as no database entry!

            strcpy(signal_desc->signal_alias, prefix);    // Flags to readCDF that no database entry was found
            strcpy(signal_desc->signal_name, signal);

            strcpy(signal_desc->description,
                   "*** No IDAM Database Entry Found: Locating Data using alias name and shot number ***");

// Is there a Data Source File?

            strlwr(prefix);        // Database records source_alias in lower case
            strcpy(signal_desc->source_alias, prefix);

            if (netcdf) {        // Hierarchical
                if (pass >= 0) {
                    sprintf(sql,
                            "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                                    "exp_number=%d and pass=%d and source_alias='%s' and format='%s'", exp_number, pass,
                            prefix, FORMAT_MODERN);
                } else {
                    sprintf(sql,
                            "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                                    "exp_number=%d and source_alias='%s' and format='%s'", exp_number, prefix,
                            FORMAT_MODERN);
                }
            } else {
                if (pass >= 0) {
                    sprintf(sql,
                            "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                                    "exp_number=%d and pass=%d and source_alias='%s' and format='%s'", exp_number, pass,
                            prefix, FORMAT_LEGACY);
                } else {
                    sprintf(sql,
                            "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                                    "exp_number=%d and source_alias='%s' and format='%s'", exp_number, prefix,
                            FORMAT_LEGACY);
                }
            }

// dgm 21May2014
            //PQclear(DBQuery);

            if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
                addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignal", 1, PQresultErrorMessage(DBQuery));
                PQclear(DBQuery);
                if (signal != NULL) free((void*)signal);
                if (tpass != NULL) free((void*)tpass);
                return 0;
            }

            data_source->exp_number = exp_number;
            data_source->pass = pass;
            strcpy(data_source->source_alias, prefix);

            if ((nrows = PQntuples(DBQuery)) > 0) {        // Number of Rows: File Found
                char* p;
                data_source->source_id = atoi(PQgetvalue(DBQuery, 0, 0));
                p = PQgetvalue(DBQuery, 0, 1);
                data_source->type = p[0];
                signal_desc->type = p[0];
                strcpy(data_source->path, PQgetvalue(DBQuery, 0, 3));
                strcpy(data_source->format, PQgetvalue(DBQuery, 0, 2));
                strcpy(data_source->filename, PQgetvalue(DBQuery, 0, 4));
                strcpy(data_source->server, PQgetvalue(DBQuery, 0, 5));
                data_source->meta_id = atoi(PQgetvalue(DBQuery, 0, 6));
                if (data_source->meta_id > 0) {
                    sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 6), data_source->xml, data_source->xml_creation);
                }
            } else {

// No Data Source File was Found: Make best guess of File Location from shot number and file type

                if (pass >= 0) {
                    sprintf(data_source->path, "%s/%03d/%d/Pass%d", getenv("MAST_DATA"), exp_number / 1000, exp_number,
                            pass);
                } else {
                    sprintf(data_source->path, "%s/%03d/%d/LATEST", getenv("MAST_DATA"), exp_number / 1000, exp_number);
                }

                if (netcdf) {
                    strcpy(data_source->format, FORMAT_MODERN);
                    if (STR_EQUALS(data_source->format, "CDF")) {
                        sprintf(data_source->filename, "%s%06d.nc", prefix, exp_number);
                        strcat(data_source->path, "/");
                        strcat(data_source->path, data_source->filename);    // Add the filename to the Path
                    } else {
                        rc = 0;            // Don't have a file name model
                    }
                } else {
                    strcpy(data_source->format, FORMAT_LEGACY);
                    if (STR_EQUALS(data_source->format, "IDA3")) {
                        nameIDA(prefix, exp_number, data_source->filename);
                        strcat(data_source->path, "/");
                        strcat(data_source->path, data_source->filename);    // Add the filename to the Path
                    } else {
                        rc = 0;            // Don't have a file name model
                    }
                }
            }
        }
    }

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);
    if (tpass != NULL) free((void*)tpass);

    return rc;
}


int sqlNoIdamSignalxxx(PGconn* DBConnect, char* originalSignal, int exp_number, int pass,
                       SIGNAL* signal_str, SIGNAL_DESC* signal_desc, DATA_SOURCE* data_source)
{
//
// This is a last resort attempt to locate data as either there are no entries in the database or
// there is only a signal_desc table record - there are no recorded data source files.
//
// If the Return Code is 0 then a Problem occured!
//

    int nrows, ncols, j, rc = 0, cost;
    char* ls, * us;

    int netcdf = 0;        // Assume the data is either IDA or netCDF4
    char prefix[4];

    char sql[MAXSQL];
    char* sqlB = "signal_desc_id, meta_id as b_meta_id, rank, range_start, range_stop, type as b_type, "
            "source_alias as b_source_alias, signal_alias, signal_name, generic_name, description, "
            "signal_class, signal_owner, creation as b_creation, modified as b_modified, signal_alias_type";

    PGresult* DBQuery = NULL;

    struct timeval tv_start, tv_end;

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignalxxx", err, "Unable to Escape the signal name!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Structures

    initSignal(signal_str);
    initSignalDesc(signal_desc);
    initDataSource(data_source);

//-------------------------------------------------------------
// Build SQL: Is there a Signal_Desc record?

    size_t lstr = strlen(signal) * sizeof(char) + 1;
    ls = (char*)malloc(lstr);
    us = (char*)malloc(lstr);
    strcpy(ls, signal);
    strcpy(us, signal);
    strupr(us);            // Upper Case signal Name
    strlwr(ls);            // Lower Case signal Name

    sprintf(sql, "SELECT %s FROM signal_desc WHERE "
            "((signal_alias_type = "
            NOPREFIXNOCASE
            " OR signal_alias_type = "
            PREFIXNOCASE
            ") AND "
                    " (generic_name = '%s' OR signal_alias = '%s' OR generic_name = '%s' OR signal_alias = '%s')) OR "
                    "((signal_alias_type = "
            NOPREFIXCASE
            " OR signal_alias_type = "
            PREFIXCASE
            ") AND "
                    " (generic_name = '%s' OR signal_alias = '%s')) AND "
                    "((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=%d) OR "
                    " (range_stop=0  AND range_start<=%d) OR (range_start>0 AND range_start<=%d AND "
                    "  range_stop>0 AND range_stop>=%d)) ORDER BY generic_name DESC, rank LIMIT 1;",
            sqlB, ls, ls, us, us, signal, signal, exp_number, exp_number, exp_number, exp_number);

    free((void*)ls);
    free((void*)us);

//-------------------------------------------------------------
// Execute SQL

    gettimeofday(&tv_start, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignal", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    ExecStatusType DBQueryStatus;
    gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlNoIdamSignal +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Rows: %d\n", nrows);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Cols: %d\n", ncols);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Msg : %s\n", PQresultErrorMessage(DBQuery));
    DBQueryStatus = PQresultStatus(DBQuery);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Stat: %s\n", PQresStatus(DBQueryStatus));

//-------------------------------------------------------------
// Record Found => Data probably exists in the archive

    if (nrows == 1) {

        rc = 1;
        j = 0;

// Signal_Desc fields

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal_desc->signal_desc_id = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal_desc->meta_id = atoi(PQgetvalue(DBQuery, 0, j));
            sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, j++), signal_desc->xml, signal_desc->xml_creation);
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal_desc->rank = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal_desc->range_start = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        if (strlen(PQgetvalue(DBQuery, 0, j)) > 0) {
            signal_desc->range_stop = atoi(PQgetvalue(DBQuery, 0, j++));
        } else { j++; }

        signal_desc->type = *PQgetvalue(DBQuery, 0, j++);

        strcpy(signal_desc->source_alias, PQgetvalue(DBQuery, 0, j++));

        strcpy(signal_desc->signal_alias, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->signal_name, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->generic_name, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->description, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->signal_class, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->signal_owner, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->creation, PQgetvalue(DBQuery, 0, j++));
        strcpy(signal_desc->modified, PQgetvalue(DBQuery, 0, j++));

        strcat(signal_desc->description,
               "*** No IDAM Database Entry Found: Locating Data using alias name and shot number ***\n");

// Data Source fields

        data_source->exp_number = exp_number;
        data_source->pass = pass;

        data_source->type = signal_desc->type;
        strncpy(data_source->source_alias, signal_desc->signal_name, 3);        // Source alias 3 letters
        data_source->source_alias[3] = '\0';

        sprintf(data_source->path, "%s/%03d/%d/LATEST", getenv("MAST_DATA"), exp_number / 1000, exp_number);

        if (strlen(signal) > 4 && signal[3] == '_') {
            strcpy(data_source->format, FORMAT_LEGACY);        // Default Legacy file format
            if (STR_EQUALS(data_source->format, "IDA3")) {
                nameIDA(signal_desc->source_alias, exp_number, data_source->filename);
                strlwr(data_source->filename);                // Ensure lower case
                strcat(data_source->path, "/");
                strcat(data_source->path, data_source->filename);    // Add the filename to the Path
            } else {
                rc = 0;            // Don't have a file name model
            }
        } else {
            strcpy(data_source->format, FORMAT_MODERN);        // Default Modern file format
            if (STR_EQUALS(data_source->format, "CDF")) {
                sprintf(data_source->filename, "%s%06d.nc", prefix, exp_number);
                strlwr(data_source->filename);                // Ensure lower case
                strcat(data_source->path, "/");
                strcat(data_source->path, data_source->filename);    // Add the filename to the Path
            } else {
                rc = 0;            // Don't have a file name model
            }
        }

// Signal fields

        signal_str->signal_desc_id = signal_desc->signal_desc_id;

//-------------------------------------------------------------
// Record Not Found

    } else {

// Signal Names missing from the IDAM database may be because:
//	1> They are from a very early shot
//	2> They were not captured for some reason by the IDAM scheduler code
//	3> The name is an attribute name - not recorded in the database (signal/variable name only)

        rc = 1;            // Always return with a Best Guess!

// Identify the Source alias from the signal

        if (signal[0] != '/') {
            strncpy(prefix, signal, 3);        // Probably IDA		Macro FORMAT_LEGACY
        } else {
            strncpy(prefix, &signal[1], 3);        // Probably netCDF4	Macro FORMAT_MODERN
        }
        prefix[3] = '\0';
        strlwr(prefix);

// Is there a Data Source File?

        if ((signal[0] == '/' && signal[4] == '/') || (signal[0] != '/' && signal[3] == '/')) {        // Hierarchical
            netcdf = 1;
            if (pass >= 0) {
                sprintf(sql, "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                        "exp_number=%d and pass=%d and source_alias='%s' and format='%s'", exp_number, pass, prefix,
                        FORMAT_MODERN);
            } else {
                sprintf(sql, "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                        "exp_number=%d and source_alias='%s' and format='%s'", exp_number, prefix, FORMAT_MODERN);
            }
        } else {
            if (pass >= 0) {
                sprintf(sql, "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                        "exp_number=%d and pass=%d and source_alias='%s' and format='%s'", exp_number, pass, prefix,
                        FORMAT_LEGACY);
            } else {
                sprintf(sql, "SELECT source_id, type, format, path, filename, server, meta_id FROM data_source WHERE "
                        "exp_number=%d and source_alias='%s' and format='%s'", exp_number, prefix, FORMAT_LEGACY);
            }
        }

        PQclear(DBQuery);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoIdamSignal", 1, PQresultErrorMessage(DBQuery));
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return 0;
        }

        data_source->exp_number = exp_number;
        data_source->pass = pass;
        strcpy(data_source->source_alias, prefix);

        strcpy(signal_desc->signal_alias, prefix);    // Flags to readCDF that no database entry was found
        strcpy(signal_desc->signal_name, signal);
        strcpy(signal_desc->description,
               "*** No IDAM Database Entry Found: Locating Data using alias name and shot number ***");

        if ((nrows = PQntuples(DBQuery)) > 0) {        // Number of Rows: File Found
            char* p;
            data_source->source_id = atoi(PQgetvalue(DBQuery, 0, 0));
            p = PQgetvalue(DBQuery, 0, 1);
            data_source->type = p[0];
            signal_desc->type = p[0];
            strcpy(data_source->path, PQgetvalue(DBQuery, 0, 3));
            strcpy(data_source->format, PQgetvalue(DBQuery, 0, 2));
            strcpy(data_source->filename, PQgetvalue(DBQuery, 0, 4));
            strcpy(data_source->server, PQgetvalue(DBQuery, 0, 5));
            data_source->meta_id = atoi(PQgetvalue(DBQuery, 0, 6));
            if (data_source->meta_id > 0) {
                sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 6), data_source->xml, data_source->xml_creation);
            }
        } else {

// No Data Source File was Found: Make best guess of File Location from shot number and file type

            sprintf(data_source->path, "%s/%03d/%d/LATEST", getenv("MAST_DATA"), exp_number / 1000, exp_number);

            if (netcdf) {
                strcpy(data_source->format, FORMAT_MODERN);
                if (STR_EQUALS(data_source->format, "CDF")) {
                    sprintf(data_source->filename, "%s%06d.nc", prefix, exp_number);
                    strcat(data_source->path, "/");
                    strcat(data_source->path, data_source->filename);    // Add the filename to the Path
                } else {
                    rc = 0;            // Don't have a file name model
                }
            } else {
                strcpy(data_source->format, FORMAT_LEGACY);
                if (STR_EQUALS(data_source->format, "IDA3")) {
                    nameIDA(prefix, exp_number, data_source->filename);
                    strcat(data_source->path, "/");
                    strcat(data_source->path, data_source->filename);    // Add the filename to the Path
                } else {
                    rc = 0;            // Don't have a file name model
                }
            }
        }
    }

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);

    return rc;
}


int sqlComposite(PGconn* DBConnect, char* originalSignal, int exp_number, SIGNAL_DESC* signal_desc_str)
{

// Composite Signals have Signal Type 'C'

    int nrows, ncols;

    char sql[
            MAXSQL] = "SELECT meta_id, rank, range_start, range_stop, signal_alias, signal_name, generic_name, description,"
                    "type, creation FROM signal_desc WHERE type = 'C' AND generic_name = '";

    char expno[MAXKEY] = "";
    char fvalue[MAXMETA] = "";

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlComposite", err, "Unable to Escape the signal name!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Structures

    initSignalDesc(signal_desc_str);

//-------------------------------------------------------------
// Build SQL

    sprintf(expno, "%d", exp_number);

    strcat(sql, signal);
    strcat(sql, "' OR signal_alias = upper('");
    strcat(sql, signal);
    strcat(sql, "')) AND ((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=");
    strcat(sql, expno);
    strcat(sql, ") OR (range_stop=0  AND range_start<=");
    strcat(sql, expno);
    strcat(sql, ") OR (range_start>0 AND range_start<=");
    strcat(sql, expno);
    strcat(sql, " AND range_stop>0 AND range_stop>=");
    strcat(sql, expno);
    strcat(sql, ")) ORDER BY generic_name DESC, rank LIMIT 1;");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlComposite", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows != 1 || ncols != 10) {
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return 0;
    }

    if (strlen(PQgetvalue(DBQuery, 0, 0)) > 0) {
        signal_desc_str->meta_id = atoi(PQgetvalue(DBQuery, 0, 0));
        sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 0), signal_desc_str->xml, signal_desc_str->xml_creation);
    }

    if (strlen(PQgetvalue(DBQuery, 0, 1)) > 0) signal_desc_str->rank = atoi(PQgetvalue(DBQuery, 0, 1));
    if (strlen(PQgetvalue(DBQuery, 0, 2)) > 0) signal_desc_str->range_start = atoi(PQgetvalue(DBQuery, 0, 2));
    if (strlen(PQgetvalue(DBQuery, 0, 3)) > 0) signal_desc_str->range_stop = atoi(PQgetvalue(DBQuery, 0, 3));
    strcpy(signal_desc_str->signal_alias, PQgetvalue(DBQuery, 0, 4));
    strcpy(signal_desc_str->signal_name, PQgetvalue(DBQuery, 0, 5));
    strcpy(signal_desc_str->generic_name, PQgetvalue(DBQuery, 0, 6));
    strcpy(signal_desc_str->description, PQgetvalue(DBQuery, 0, 7));
    strcpy(fvalue, PQgetvalue(DBQuery, 0, 8));
    signal_desc_str->type = fvalue[0];
    strcpy(signal_desc_str->creation, PQgetvalue(DBQuery, 0, 9));

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);

    return 1;
}


// Image/Documents: No signal data just a source file record

int sqlDocument(PGconn* DBConnect, char* originalSignal, int exp_number, int pass,
                SIGNAL_DESC* signal_desc_str, DATA_SOURCE* data_source_str)
{
//
// If the Return Code is 0 then a Problem occured.
//

    int nrows, rc = 0;

    char sql[MAXSQL] = "SELECT source_id, meta_id, pass, source_status,"
            "type, source_alias, pass_date, format, path, filename, server, creation "
            "FROM data_source WHERE exp_number=";

    char expno[MAXKEY] = "";
    char passno[MAXKEY] = "";

    PGresult* DBQuery = NULL;
    //ExecStatusType DBQueryStatus;

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDocument", err, "Unable to Escape the signal name!");
        return 0;
    }
//-------------------------------------------------------------
// Initialise Structures

    initSignalDesc(signal_desc_str);
    initDataSource(data_source_str);

//-------------------------------------------------------------
// Build SQL

    sprintf(expno, "%d", exp_number);
    sprintf(passno, "%d", pass);

    strcat(sql, expno);

    if (pass > -1) {
        strcat(sql, " AND pass=");
        strcat(sql, passno);
    }

    strcat(sql, " AND source_alias = lower('");
    strcat(sql, signal);
    strcat(sql, "')  ORDER BY pass DESC LIMIT 1;");

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDocument", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return rc;
    }

//-------------------------------------------------------------
// SQL Results

    nrows = PQntuples(DBQuery); // Number of Rows

    if (nrows == 1) {

        rc = 1;

        if (strlen(PQgetvalue(DBQuery, 0, 0)) > 0) data_source_str->source_id = atoi(PQgetvalue(DBQuery, 0, 0));
        if (strlen(PQgetvalue(DBQuery, 0, 1)) > 0) {
            data_source_str->meta_id = atoi(PQgetvalue(DBQuery, 0, 1));
            sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 1), data_source_str->xml, data_source_str->xml_creation);
        }
        data_source_str->exp_number = exp_number;
        if (strlen(PQgetvalue(DBQuery, 0, 2)) > 0) data_source_str->pass = atoi(PQgetvalue(DBQuery, 0, 2));

        if (strlen(PQgetvalue(DBQuery, 0, 3)) > 0) {
            data_source_str->status = atoi(PQgetvalue(DBQuery, 0, 3));
        } else {
            data_source_str->status = DEFAULT_STATUS;
        }

        data_source_str->type = PQgetvalue(DBQuery, 0, 4)[0];

        strcpy(data_source_str->source_alias, PQgetvalue(DBQuery, 0, 5));
        strcpy(data_source_str->pass_date, PQgetvalue(DBQuery, 0, 6));
        strcpy(data_source_str->format, PQgetvalue(DBQuery, 0, 7));
        strcpy(data_source_str->path, PQgetvalue(DBQuery, 0, 8));
        strcpy(data_source_str->filename, PQgetvalue(DBQuery, 0, 9));
        strcpy(data_source_str->server, PQgetvalue(DBQuery, 0, 10));
        strcpy(data_source_str->creation, PQgetvalue(DBQuery, 0, 11));

        strcpy(signal_desc_str->signal_alias, signal);
        strcpy(signal_desc_str->signal_name, signal);
        strcpy(signal_desc_str->generic_name, "");
        strcpy(signal_desc_str->description, "Document");
        strcpy(signal_desc_str->creation, data_source_str->creation);

        signal_desc_str->type = data_source_str->type;

    }

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);

    return rc;
}


// Identify Methods for Accessing MAST Archives

int sqlArchive(PGconn* DBConnect, char* archive, DATA_SOURCE* data_source_str)
{

//-------------------------------------------------------------
// Initialise Structures

    initDataSource(data_source_str);

    strcpy(data_source_str->archive, archive);
    strcpy(data_source_str->device_name, "MAST");
    strcpy(data_source_str->format, "SQL");
    strcpy(data_source_str->filename, "CMData");

    return 1;
}

//-------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------

int sqlExternalGeneric(PGconn* DBConnect, char* originalArchive, char* originalDevice, char* originalSignal,
                       int exp_number, int pass,
                       SIGNAL* signal_str, SIGNAL_DESC* signal_desc_str, DATA_SOURCE* data_source_str)
{

    char fname[MAXNAME] = "";
    char fvalue[MAXMETA] = "";

    int nrows, ncols, j, rc = 0;

    char sql[MAXSQL];

    char expno[MAXKEY] = "";

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Escape SIGNAL and ARCHIVE and DEVICE to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlExternalGeneric", err, "Unable to Escape the signal name!");
        return 0;
    }

    char* archive = (char*)malloc((strlen(originalArchive) + 1) * sizeof(char));
    strcpy(archive, originalArchive);
    if (preventSQLInjection(DBConnect, &archive)) {
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlExternalGeneric", err, "Unable to Escape the archive name!");
        return 0;
    }

    char* device = (char*)malloc((strlen(originalDevice) + 1) * sizeof(char));
    strcpy(device, originalDevice);
    if (preventSQLInjection(DBConnect, &device)) {
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        if (device != NULL) free((void*)device);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlExternalGeneric", err, "Unable to Escape the device name!");
        return 0;
    }

//-------------------------------------------------------------
// Initialise Structures

    initSignal(signal_str);
    initSignalDesc(signal_desc_str);
    initDataSource(data_source_str);

//-------------------------------------------------------------
// Build SQL

    strcpy(sql, "SELECT source_id, archive, device_name, source_meta_id, userid, format,"
            "path, filename, server, c.signal_desc_id, signal_meta_id,"
            "signal_desc_meta_id, rank, range_start, range_stop,"
            "signal_name, generic_name, description, "
            "source_creation, signal_creation, signal_desc_creation "

            "FROM (SELECT * FROM ("

            "SELECT source_id as signal_source_id,signal_desc_id, meta_id as signal_meta_id,"
            "creation as signal_creation FROM External_Signal) a,("

            "SELECT source_id,archive,device_name,meta_id as source_meta_id,"
            "userid,format,filename,path,server,"
            "creation as source_creation "
            "FROM External_Data_Source WHERE device_name = '");
    strcat(sql, device);
    strcat(sql, "' AND archive = '");
    strcat(sql, archive);
    strcat(sql, "'");

    strcat(sql, ") b WHERE a.signal_source_id = b.source_id) c,("
            "SELECT signal_desc_id,signal_name,generic_name,rank,"
            "range_start,range_stop,description,meta_id as signal_desc_meta_id,"
            "creation as signal_desc_creation "
            "FROM External_Signal_Desc WHERE ");

    sprintf(expno, "%d", exp_number);

    strcat(sql, "generic_name = '");
    strcat(sql, signal);
    strcat(sql, "' AND (range_start=0 OR range_start<=");
    strcat(sql, expno);
    strcat(sql, ") AND (range_stop=0 OR range_stop>=");
    strcat(sql, expno);
    strcat(sql, ") ");

    strcat(sql, ") d WHERE c.signal_desc_id=d.signal_desc_id ORDER BY generic_name DESC, rank LIMIT 1");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlExternalGeneric", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        if (device != NULL) free((void*)device);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 1 && ncols == 21) {
        rc = 1;
        for (j = 0; j < ncols; j++) {
            strcpy(fname, PQfname(DBQuery, j));
            strcpy(fvalue, PQgetvalue(DBQuery, 0, j));

            switch (j) {
                case 0 :
                    if (strlen(fvalue) > 0) {
                        data_source_str->source_id = atoi(fvalue);
                        signal_str->source_id = atoi(fvalue);
                    }
                    break;
                case 1 :
                    strcpy(data_source_str->archive, fvalue);
                    break;
                case 2 :
                    strcpy(data_source_str->device_name, fvalue);
                    break;
                case 3 :
                    if (strlen(fvalue) > 0) {
                        data_source_str->meta_id = atoi(fvalue);
                        sqlMeta(DBConnect, "External_Meta", fvalue, data_source_str->xml,
                                data_source_str->xml_creation);
                    }
                    break;
                case 4 :
                    strcpy(data_source_str->userid, fvalue);
                    break;
                case 5 :
                    strcpy(data_source_str->format, fvalue);
                    break;
                case 6 :
                    strcpy(data_source_str->path, fvalue);
                    break;
                case 7 :
                    strcpy(data_source_str->filename, fvalue);
                    break;
                case 8 :
                    strcpy(data_source_str->server, fvalue);
                    break;
                case 9 :
                    if (strlen(fvalue) > 0) {
                        signal_str->signal_desc_id = atoi(fvalue);
                        signal_desc_str->signal_desc_id = atoi(fvalue);
                    }
                    break;
                case 10 :
                    if (strlen(fvalue) > 0) {
                        signal_str->meta_id = atoi(fvalue);
                        sqlMeta(DBConnect, "External_Meta", fvalue, signal_str->xml, signal_str->xml_creation);
                    }
                    break;
                case 11 :
                    if (strlen(fvalue) > 0) {
                        signal_desc_str->meta_id = atoi(fvalue);
                        sqlMeta(DBConnect, "External_Meta", fvalue, signal_desc_str->xml,
                                signal_desc_str->xml_creation);
                    }
                    break;
                case 12 :
                    if (strlen(fvalue) > 0) signal_desc_str->rank = atoi(fvalue);
                    break;
                case 13 :
                    if (strlen(fvalue) > 0) signal_desc_str->range_start = atoi(fvalue);
                    break;
                case 14 :
                    if (strlen(fvalue) > 0) signal_desc_str->range_stop = atoi(fvalue);
                    break;
                case 15 :
                    strcpy(signal_desc_str->signal_name, fvalue);
                    break;
                case 16 :
                    strcpy(signal_desc_str->generic_name, fvalue);
                    break;
                case 17 :
                    strcpy(signal_desc_str->description, fvalue);
                    break;
                case 18 :
                    strcpy(data_source_str->creation, fvalue);
                    break;
                case 19 :
                    strcpy(signal_str->creation, fvalue);
                    break;
                case 20 :
                    strcpy(signal_desc_str->creation, fvalue);
                    break;
                default:
                    break;
            }
        }

        strcpy(signal_desc_str->signal_alias, signal_desc_str->generic_name);

    } else {    // Attempt to Read Signal via External Source Table Only

        rc = sqlNoSignal(DBConnect, archive, device, signal, -1, -1,
                         signal_str, signal_desc_str, data_source_str);

        data_source_str->exp_number = exp_number;
        data_source_str->pass = pass;

        strcpy(signal_desc_str->signal_name, signal);
        strcpy(signal_desc_str->signal_alias, signal);
        strcpy(signal_desc_str->generic_name, signal);
    }

    data_source_str->status = 'A';
    signal_str->status = 'A';
    data_source_str->type = 'E';    // External

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);
    if (archive != NULL) free((void*)archive);
    if (device != NULL) free((void*)device);

    return rc;
}

// When a Signal Entry Does NOT EXIST!

int sqlNoSignal(PGconn* DBConnect, char* originalArchive, char* originalDevice, char* originalSignal, int exp_number,
                int pass,
                SIGNAL* signal_str, SIGNAL_DESC* signal_desc_str, DATA_SOURCE* data_source_str)
{

    char fname[MAXNAME] = "";
    char fvalue[MAXMETA] = "";

    int nrows, ncols, j, rc = 0;

    char sql[MAXSQL];

    char expno[MAXKEY] = "";
    char passno[MAXKEY] = "";

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Escape SIGNAL and ARCHIVE and DEVICE to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoSignal", err, "Unable to Escape the signal name!");
        return 0;
    }

    char* archive = (char*)malloc((strlen(originalArchive) + 1) * sizeof(char));
    strcpy(archive, originalArchive);
    if (preventSQLInjection(DBConnect, &archive)) {
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoSignal", err, "Unable to Escape the archive name!");
        return 0;
    }

    char* device = (char*)malloc((strlen(originalDevice) + 1) * sizeof(char));
    strcpy(device, originalDevice);
    if (preventSQLInjection(DBConnect, &device)) {
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        if (device != NULL) free((void*)device);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoSignal", err, "Unable to Escape the device name!");
        return 0;
    }

//-------------------------------------------------------------
// Build SQL

    if (archive[0] != '\0' && device[0] != '\0' && exp_number == -1) {

        strcpy(sql, "SELECT source_id,archive,device_name,meta_id,"
                "userid,format,path,filename,server,creation  "
                "FROM External_Data_Source WHERE device_name = '");
        strcat(sql, device);
        strcat(sql, "' AND archive = '");
        strcat(sql, archive);
        strcat(sql, "';");
    } else {
        strcpy(sql, "SELECT source_id,config_id,source_alias,meta_id,"
                "exp_number,pass,pass_date,status,type,format,filename,"
                "path,server,access,reprocess,creation,reason_id,run_id "
                "FROM data_source WHERE exp_number=");

        sprintf(expno, "%d", exp_number);
        strcat(sql, expno);

        if (pass > -1) {
            sprintf(passno, "%d", pass);
            strcat(sql, " AND pass=");
            strcat(sql, passno);
        }

        strcat(sql, " ORDER BY pass DESC LIMIT 1;");

    }

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlNoSignal", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        if (archive != NULL) free((void*)archive);
        if (device != NULL) free((void*)device);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 1 && (ncols == 10 || ncols == 18)) {
        rc = 1;
        for (j = 0; j < ncols; j++) {
            strcpy(fname, PQfname(DBQuery, j));
            strcpy(fvalue, PQgetvalue(DBQuery, 0, j));

            if (ncols == 10) {
                switch (j) {
                    case 0 :
                        if (strlen(fvalue) > 0) {
                            data_source_str->source_id = atoi(fvalue);
                            signal_str->source_id = atoi(fvalue);
                        }
                        break;
                    case 1 :
                        strcpy(data_source_str->archive, fvalue);
                        break;
                    case 2 :
                        strcpy(data_source_str->device_name, fvalue);
                        break;
                    case 3 :
                        if (strlen(fvalue) > 0) {
                            data_source_str->meta_id = atoi(fvalue);
                            sqlMeta(DBConnect, "External_Meta", fvalue, data_source_str->xml,
                                    data_source_str->xml_creation);
                        }
                        break;
                    case 4 :
                        strcpy(data_source_str->userid, fvalue);
                        break;
                    case 5 :
                        strcpy(data_source_str->format, fvalue);
                        break;
                    case 6 :
                        strcpy(data_source_str->path, fvalue);
                        break;
                    case 7 :
                        strcpy(data_source_str->filename, fvalue);
                        break;
                    case 8 :
                        strcpy(data_source_str->server, fvalue);
                        break;
                    case 9 :
                        strcpy(data_source_str->creation, fvalue);
                        break;
                    default:
                        break;
                }
            } else {
                switch (j) {
                    case 0 :
                        if (strlen(fvalue) > 0) {
                            data_source_str->source_id = atoi(fvalue);
                            signal_str->source_id = atoi(fvalue);
                        }
                        break;
                    case 1 :
                        if (strlen(fvalue) > 0) {
                            data_source_str->config_id = atoi(fvalue);
                        }
                        break;
                    case 2 :
                        if (strlen(fvalue) > 0) {
                            strcpy(data_source_str->source_alias, fvalue);
                        }
                        break;
                    case 3 :
                        if (strlen(fvalue) > 0) {
                            data_source_str->meta_id = atoi(fvalue);
                            sqlMeta(DBConnect, "Meta", fvalue, data_source_str->xml, data_source_str->xml_creation);
                        }
                        break;
                    case 4 :
                        if (strlen(fvalue) > 0) data_source_str->exp_number = atoi(fvalue);
                        break;
                    case 5 :
                        if (strlen(fvalue) > 0) data_source_str->pass = atoi(fvalue);
                        break;
                    case 6 :
                        strcpy(data_source_str->pass_date, fvalue);
                        break;
                    case 7 :
                        data_source_str->status = fvalue[0];
                        break;
                    case 8 :
                        data_source_str->type = fvalue[0];
                        break;
                    case 9 :
                        strcpy(data_source_str->format, fvalue);
                        break;
                    case 10 :
                        strcpy(data_source_str->filename, fvalue);
                        break;
                    case 11 :
                        strcpy(data_source_str->path, fvalue);
                        break;
                    case 12 :
                        strcpy(data_source_str->server, fvalue);
                        break;
                    case 13 :
                        break;
                    case 14 :
                        break;
                    case 15 :
                        strcpy(data_source_str->creation, fvalue);
                        break;
                    case 16 :
                        if (strlen(fvalue) > 0) data_source_str->reason_id = atoi(fvalue);
                        break;
                    case 17 :
                        if (strlen(fvalue) > 0) data_source_str->run_id = atoi(fvalue);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);
    if (archive != NULL) free((void*)archive);
    if (device != NULL) free((void*)device);

    return rc;
}


int sqlDataSystem(PGconn* DBConnect, int pkey, DATA_SYSTEM* str)
{

    char fname[MAXNAME] = "";
    char fvalue[MAXMETA] = "";
    char system_id[MAXKEY];

    int nrows, ncols, j, rc = 0;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;
    //ExecStatusType DBQueryStatus;

//-------------------------------------------------------------
// Initialise Structures

    initDataSystem(str);

//-------------------------------------------------------------
// Build SQL

    sprintf(system_id, "%d", pkey);

    strcpy(sql, "SELECT system_id, version, type, device_name, system_name, "
            "system_desc, creation, meta_id "
            "FROM data_system WHERE system_id=");
    strcat(sql, system_id);
    strcat(sql, " ORDER BY version DESC LIMIT 1");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSystem", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 1 && ncols == 8) {
        rc = 1;
        for (j = 0; j < ncols; j++) {
            strcpy(fname, PQfname(DBQuery, j));
            strcpy(fvalue, PQgetvalue(DBQuery, 0, j));

            switch (j) {
                case 0 :
                    if (strlen(fvalue) > 0) str->system_id = atoi(fvalue);
                    break;
                case 1 :
                    if (strlen(fvalue) > 0) str->version = atoi(fvalue);
                    break;
                case 2 :
                    str->type = fvalue[0];
                    break;
                case 3 :
                    strcpy(str->system_name, fvalue);
                    break;
                case 4 :
                    strcpy(str->device_name, fvalue);
                    break;
                case 5 :
                    strcpy(str->system_desc, fvalue);
                    break;
                case 6 :
                    strcpy(str->creation, fvalue);
                    break;
                case 7 :
                    if (strlen(fvalue) > 0) {
                        str->meta_id = atoi(fvalue);
                        sqlMeta(DBConnect, "Meta", fvalue, str->xml, str->xml_creation);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    PQclear(DBQuery);

    return rc;
}


int sqlSystemConfig(PGconn* DBConnect, int pkey, SYSTEM_CONFIG* str)
{

    char fname[MAXNAME] = "";
    char fvalue[MAXMETA] = "";
    char config_id[MAXKEY];

    int nrows, ncols, j, rc = 0;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;
    //ExecStatusType DBQueryStatus;

//-------------------------------------------------------------
// Initialise Structures

    initSystemConfig(str);

//-------------------------------------------------------------
// Build SQL

    sprintf(config_id, "%d", pkey);

    strcpy(sql, "SELECT config_id, system_id, config_name, "
            "config_desc, creation, meta_id "
            "FROM System_Config WHERE config_id=");
    strcat(sql, config_id);
    strcat(sql, ";");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSystemConfig", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 1 && ncols == 6) {
        rc = 1;
        for (j = 0; j < ncols; j++) {
            strcpy(fname, PQfname(DBQuery, j));
            strcpy(fvalue, PQgetvalue(DBQuery, 0, j));

            switch (j) {
                case 0 :
                    if (strlen(fvalue) > 0) str->config_id = atoi(fvalue);
                    break;
                case 1 :
                    if (strlen(fvalue) > 0) str->system_id = atoi(fvalue);
                    break;
                case 2 :
                    strcpy(str->config_name, fvalue);
                    break;
                case 3 :
                    strcpy(str->config_desc, fvalue);
                    break;
                case 4 :
                    strcpy(str->creation, fvalue);
                    break;
                case 5 :
                    if (strlen(fvalue) > 0) {
                        str->meta_id = atoi(fvalue);
                        sqlMeta(DBConnect, "Meta", fvalue, str->xml, str->xml_creation);
                    }
                    break;

                default:
                    break;
            }
        }
    }

    PQclear(DBQuery);

    return rc;
}


int sqlLatestPass(PGconn* DBConnect, char* source_alias, char type, int exp_number, char* maxpass)
{

// Return the LATEST Pass Number

    int nrows;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Build SQL

    sprintf(sql, "SELECT max(pass) FROM data_source WHERE exp_number=%d AND source_alias='%s' and type='%c';",
            exp_number, source_alias, type);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlLatestPass", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 1) {
        strcpy(maxpass, PQgetvalue(DBQuery, 0, 0));
    } else {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlLatestPass", 1, "No Record returned when 1 expected!");
        PQclear(DBQuery);
        return 0;
    }

    PQclear(DBQuery);

    return 1;
}


int sqlLatestPassx(PGconn* DBConnect, char* signal, int exp_number, char* maxpass)
{

// Return the required Pass Number as a String if LATEST-n Requested

    int nrows;

    char sql[MAXSQL] = "SELECT max(data_source.pass) FROM data_source, signal, signal_desc "
            "WHERE data_source.source_id=signal.source_id AND data_source.exp_number=";

    char expno[MAXKEY] = "";

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Build SQL

    sprintf(expno, "%d", exp_number);

    strcat(sql, expno);

    strcat(sql, " AND data_source.type=signal_desc.type "
            "AND signal.signal_desc_id=signal_desc.signal_desc_id "
            "AND (signal_desc.generic_name = upper('");            // Names should be in Upper Case
    strcat(sql, signal);
    strcat(sql, "') OR signal_desc.signal_alias = upper('");
    strcat(sql, signal);

    strcat(sql,
           "')) AND ((signal_desc.range_start=0 AND signal_desc.range_stop=0) OR (signal_desc.range_start=0 AND signal_desc.range_stop>=");
    strcat(sql, expno);
    strcat(sql, ") OR (signal_desc.range_stop=0  AND signal_desc.range_start<=");
    strcat(sql, expno);
    strcat(sql, ") OR (signal_desc.range_start>0 AND signal_desc.range_start<=");
    strcat(sql, expno);
    strcat(sql, " AND signal_desc.range_stop>0 AND signal_desc.range_stop>=");
    strcat(sql, expno);

    strcat(sql, "));");

    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlLatestPass", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 1) {
        strcpy(maxpass, PQgetvalue(DBQuery, 0, 0));
    } else {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlLatestPass", 1, "No Generic Record found for this Signal");
        PQclear(DBQuery);
        return 0;
    }

    PQclear(DBQuery);

    return 1;
}


int sqlDataSourceAlias(PGconn* DBConnect, char* originalSignal, char** alias)
{

// If the Return int is 0 then a Problem occured: Perhaps the Signal was not recognised!

    int nrows, rc = 0;
    char* ls, * us;

    PGresult* DBQuery = NULL;

    char sql[MAXSQL];

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    char* signal = (char*)malloc((strlen(originalSignal) + 1) * sizeof(char));
    strcpy(signal, originalSignal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSourceAlias", err, "Unable to Escape the signal name!");
        return 0;
    }
//-------------------------------------------------------------
// Build SQL observing rules on Mixed Case Sensitivity and Prefix

    size_t lstr = strlen(signal) * sizeof(char) + 1;
    ls = (char*)malloc(lstr);
    us = (char*)malloc(lstr);
    strcpy(ls, signal);
    strcpy(us, signal);
    strupr(us);            // Upper Case signal Name
    strlwr(ls);            // Lower Case signal Name

    sprintf(sql, "SELECT source_alias FROM signal_desc WHERE "
            "((signal_alias_type = "
            NOPREFIXNOCASE
            " OR signal_alias_type = "
            PREFIXNOCASE
            ") AND "
                    " (generic_name = '%s' OR signal_alias = '%s' OR generic_name = '%s' OR signal_alias = '%s')) OR "
                    "((signal_alias_type = "
            NOPREFIXCASE
            " OR signal_alias_type = "
            PREFIXCASE
            ") AND "
                    " (generic_name = '%s' OR signal_alias = '%s')) "
                    "ORDER BY generic_name DESC, signal_alias_type LIMIT 1;",
            ls, ls, us, us, signal, signal);

    free((void*)ls);
    free((void*)us);

//-------------------------------------------------------------
// Execute SQL

    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSourceAlias", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

//-------------------------------------------------------------
// Process Record

    if (nrows == 1) {

        if ((lstr = strlen(PQgetvalue(DBQuery, 0, 0))) == 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSourceAlias", 1, "The Source_Alias name is Missing.");
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return 0;
        }

        rc = 1;
        *alias = (char*)malloc((lstr + 1) * sizeof(char));
        strcpy(*alias, PQgetvalue(DBQuery, 0, 0));
    } else {
        rc = 0;
        *alias = NULL;
    }

    PQclear(DBQuery);
    if (signal != NULL) free((void*)signal);

    return rc;
}

int sqlSignalDesc(PGconn* DBConnect, char* signal_desc_id, SIGNAL_DESC* signal_desc)
{
//
// Return a signal_desc record given a primary key.
//
// If the Return Code is 0 then a Problem occured!
//

    int nrows;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Initialise Structure

    initSignalDesc(signal_desc);

//-------------------------------------------------------------
// Build SQL

    sprintf(sql, "SELECT signal_desc_id,meta_id,rank,range_start,range_stop,"
            "type,source_alias,signal_alias,signal_name,generic_name,description,signal_class,signal_owner,"
            "creation,modified, signal_alias_type, signal_map_id FROM signal_desc WHERE signal_desc_id=%s;",
            signal_desc_id);

//-------------------------------------------------------------
// Execute SQL

    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDesc", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return 0;
    }

    nrows = PQntuples(DBQuery); // Number of Rows

//-------------------------------------------------------------
// Copy record to Data Structure

    if (nrows != 1) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlSignalDesc", 1,
                     "No (or Multiple) Signal_Desc table records found!");
        PQclear(DBQuery);
        return 0;
    }

// Signal_Desc fields

    signal_desc->signal_desc_id = atoi(PQgetvalue(DBQuery, 0, 0));

    signal_desc->meta_id = atoi(PQgetvalue(DBQuery, 0, 1));
    if (signal_desc->meta_id > 0) {
        sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 1), signal_desc->xml, signal_desc->xml_creation);
    }

    signal_desc->rank = atoi(PQgetvalue(DBQuery, 0, 2));
    signal_desc->range_start = atoi(PQgetvalue(DBQuery, 0, 3));
    signal_desc->range_stop = atoi(PQgetvalue(DBQuery, 0, 4));

    signal_desc->type = *PQgetvalue(DBQuery, 0, 5);

    strcpy(signal_desc->source_alias, PQgetvalue(DBQuery, 0, 6));
    strcpy(signal_desc->signal_alias, PQgetvalue(DBQuery, 0, 7));
    strcpy(signal_desc->signal_name, PQgetvalue(DBQuery, 0, 8));
    strcpy(signal_desc->generic_name, PQgetvalue(DBQuery, 0, 9));
    strcpy(signal_desc->description, PQgetvalue(DBQuery, 0, 10));
    strcpy(signal_desc->signal_class, PQgetvalue(DBQuery, 0, 11));
    strcpy(signal_desc->signal_owner, PQgetvalue(DBQuery, 0, 12));
    strcpy(signal_desc->creation, PQgetvalue(DBQuery, 0, 13));
    strcpy(signal_desc->modified, PQgetvalue(DBQuery, 0, 14));

    if (strlen(PQgetvalue(DBQuery, 0, 15)) > 0) signal_desc->signal_alias_type = atoi(PQgetvalue(DBQuery, 0, 15));
    if (strlen(PQgetvalue(DBQuery, 0, 16)) > 0) {
        signal_desc->signal_map_id = atoi(PQgetvalue(DBQuery, 0, 16));
    }            // Map signal Name

    PQclear(DBQuery);

    return 1;
}

int sqlDataSource(PGconn* DBConnect, char* source_id, DATA_SOURCE* data_source)
{
//
// Return a data_source record given a primary key.
//
// If the Return Code is 0 then a Problem occured!
//

    int nrows;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Initialise Structure

    initDataSource(data_source);

//-------------------------------------------------------------
// Build SQL

    sprintf(sql, "SELECT source_id, reason_id, run_id,  meta_id, status_desc_id, exp_number, pass, source_status, "
            "status_reason_impact_code, type, source_alias, pass_date, format, path, "
            "filename, server, creation, modified FROM data_source WHERE source_id=%s;", source_id);

//-------------------------------------------------------------
// Execute SQL

    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSource", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return 0;
    }

    nrows = PQntuples(DBQuery); // Number of Rows

//-------------------------------------------------------------
// Copy record to Data Structure

    if (nrows != 1) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlDataSource", 1,
                     "No (or Multiple) Signal_Desc table records found!");
        PQclear(DBQuery);
        return 0;
    }

    data_source->source_id = atoi(PQgetvalue(DBQuery, 0, 0));
    data_source->reason_id = atoi(PQgetvalue(DBQuery, 0, 1));
    data_source->run_id = atoi(PQgetvalue(DBQuery, 0, 2));
    data_source->meta_id = atoi(PQgetvalue(DBQuery, 0, 3));
    if (data_source->meta_id > 0) {
        sqlMeta(DBConnect, "Meta", PQgetvalue(DBQuery, 0, 3), data_source->xml, data_source->xml_creation);
    }

    data_source->status_desc_id = atoi(PQgetvalue(DBQuery, 0, 4));
    data_source->exp_number = atoi(PQgetvalue(DBQuery, 0, 5));
    data_source->pass = atoi(PQgetvalue(DBQuery, 0, 6));
    data_source->status = atoi(PQgetvalue(DBQuery, 0, 7));
    data_source->status_impact_code = atoi(PQgetvalue(DBQuery, 0, 8));

    data_source->type = *PQgetvalue(DBQuery, 0, 9);

    strcpy(data_source->source_alias, PQgetvalue(DBQuery, 0, 10));
    strcpy(data_source->pass_date, PQgetvalue(DBQuery, 0, 11));
    strcpy(data_source->format, PQgetvalue(DBQuery, 0, 12));
    strcpy(data_source->path, PQgetvalue(DBQuery, 0, 13));
    strcpy(data_source->filename, PQgetvalue(DBQuery, 0, 14));
    strcpy(data_source->server, PQgetvalue(DBQuery, 0, 15));
    strcpy(data_source->creation, PQgetvalue(DBQuery, 0, 16));
    strcpy(data_source->modified, PQgetvalue(DBQuery, 0, 17));

    PQclear(DBQuery);

    return 1;
}


int sqlAltData(PGconn* DBConnect, REQUEST_BLOCK request_block, int rank, SIGNAL_DESC* signal_desc, char* mapping)
{

// Identify an Alternative Signal Name and Source given a Legacy Name
//
// Convention:
//
// Data must reside within the same data Archive
// Data must be for the same Device
//
// If the source is a private file, then ignore any xml corrections saved with the Signal_Desc record
// Otherwise, respect xml corrections and merge new mapping xml into the signal_desc xml
//
// If the Legacy signal name is Not found and the access method is GENERIC, query the Signal_Desc table
// for the source alias of the legacy name. If found, check there are no entries in the Signal_Alt table
// against this source_alias. If there are, then No mapping exists and the data cannot be provided. If there
// are none, then execute the normal GENERIC name access method for the data.
//
// If the access method is Not GENERIC, i.e. it is for a private file, then as no mapping exists the user
// must be requesting a data item from the private file using a non-legacy name.


    int nrows, rc = 0;
    char* ls, * us;
    char* signal, * alias;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    signal = (char*)malloc((strlen(request_block.signal) + 1) * sizeof(char));
    strcpy(signal, request_block.signal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlAltData", err, "Unable to Escape the signal name!");
        return 0;
    }

//-------------------------------------------------------------
// Build SQL: Locate the legacy name mapping

    size_t lstr = strlen(signal) * sizeof(char) + 1;
    ls = (char*)malloc(lstr);
    us = (char*)malloc(lstr);
    strcpy(ls, signal);
    strcpy(us, signal);
    strupr(us);            // Upper Case signal Name
    strlwr(ls);            // Lower Case signal Name

    if (rank == 0) {
        sprintf(sql, "SELECT signal_desc_alt_id, mapping_xml FROM Signal_Alt_View WHERE  "
                "((signal_alias_type = "
                NOPREFIXNOCASE
                " OR signal_alias_type = "
                PREFIXNOCASE
                ") AND "
                        " (generic_name = '%s' OR signal_alias = '%s' OR generic_name = '%s' OR signal_alias = '%s')) OR "
                        "((signal_alias_type = "
                NOPREFIXCASE
                " OR signal_alias_type = "
                PREFIXCASE
                ") AND "
                        " (generic_name = '%s' OR signal_alias = '%s')) "
                        "ORDER BY rank LIMIT 1;", ls, ls, us, us, signal, signal);
    } else {
        sprintf(sql, "SELECT signal_desc_alt_id, mapping_xml FROM Signal_Alt_View WHERE  "
                "((signal_alias_type = "
                NOPREFIXNOCASE
                " OR signal_alias_type = "
                PREFIXNOCASE
                ") AND "
                        " (generic_name = '%s' OR signal_alias = '%s' OR generic_name = '%s' OR signal_alias = '%s')) OR "
                        "((signal_alias_type = "
                NOPREFIXCASE
                " OR signal_alias_type = "
                PREFIXCASE
                ") AND "
                        " (generic_name = '%s' OR signal_alias = '%s')) AND rank = %d "
                        "ORDER BY rank LIMIT 1;", ls, ls, us, us, signal, signal, rank);
    }

    IDAM_LOGF(UDA_LOG_DEBUG, "sqlAltData: %s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlAltData", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
        return 0;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 1) {            // Mapping found
        if ((rc = sqlSignalDesc(DBConnect, PQgetvalue(DBQuery, 0, 0), signal_desc)) == 0) {
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return 0;
        }
        strcpy(mapping, PQgetvalue(DBQuery, 0, 1));

        IDAM_LOGF(UDA_LOG_DEBUG, "Alt Signal Mapping:\n%s\n", mapping);
        printSignalDesc(*signal_desc);

        PQclear(DBQuery);

    } else {                // No Mapping Found

        mapping[0] = '\0';

// Scenarios
// 1> Generic Request
//	1.1> If the source alias is used for any mapping then no data can be located -> return an error
//	1.2> If the source alias is Not used, then this is a normal request for data
// 2> Formated Request
//	2.1> this is a normal request for data

        PQclear(DBQuery);

// If the file is Private, then Regular data access using the client provided name should proceed

        if (request_block.request != REQUEST_READ_GENERIC) {
            strcpy(signal_desc->signal_name,
                   request_block.signal);    // Alias or Generic have no context wrt private files
            if (signal != NULL) free((void*)signal);
            return 1;
        }

// Identify the Data Source Alias

        if ((rc = sqlDataSourceAlias(DBConnect, signal, &alias)) != 1) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlAltData", 1, "No Source Alias name found for this Signal");
            if (signal != NULL) free((void*)signal);
            return 0;
        }

// Does this Source Alias have Names mapped against it?

        if (rank == 0) {
            sprintf(sql, "SELECT signal_desc_alt_id FROM Signal_Alt_View WHERE source_alias = '%s' LIMIT 1;", alias);
        } else {
            sprintf(sql,
                    "SELECT signal_desc_alt_id FROM Signal_Alt_View WHERE source_alias = '%s' and rank = %d LIMIT 1;",
                    alias, rank);
        }

        if (alias != NULL)free((void*)alias);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlAltData", 1, PQresultErrorMessage(DBQuery));
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return 0;
        }

// Reject data access if a match occurs

        if ((nrows = PQntuples(DBQuery)) > 0) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlAltData", 1, "No Legacy Signal Mapping found");
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return 0;
        }

// Regular data access using the client provided names should proceed

        signal_desc->signal_alias[0] = '\0';    // No mapped signal substitute to target in the Generic Lookup

        PQclear(DBQuery);
        if (signal != NULL) free((void*)signal);
    }

    return 1;
}

//int sqlMapData(PGconn *DBConnect, int signal_map_id, int exp_number, SIGNAL_DESC *signal_desc){

int sqlMapData(PGconn* DBConnect, int signal_desc_id, int exp_number, SIGNAL_DESC* signal_desc)
{

// Identify the Mapping between Signal names subject to shot range constraints
// A Pass constraint is not viable - Analysed data should be reprocessed to correct data or
// the signal's status value set to 'BAD'.
//
// For any given signal_desc record, there can be multiple Signal_Map records, each with a different shot range.
// There can only be one valid shot range. If more are found, an error is raised.
//
// All related records are found using the signal_desc_id foreign key.
//
// If a legitimate mapping is found, return all information in the signal_desc structure. Return code 1.
// If a legitimate mapping is not found, return code=1 but signal_desc.signal_desc_id=0;
// If a problem the return code=0.

    int i, nrows, rc = 0;
    char sql[MAXSQL];
    static int depth = 0;
    static int keyList[MAXMAPDEPTH + 1];

    SIGNAL_DESC signal_desc_map;

    PGresult* DBQuery = NULL;

    signal_desc->signal_desc_id = 0;
    signal_desc_map.signal_desc_id = 0;

    keyList[depth] = signal_desc_id;

    IDAM_LOGF(UDA_LOG_DEBUG, "sqlMapData: %d\n", signal_desc_id);

//-------------------------------------------------------------
// Build SQL: Locate the signal name mapping

    sprintf(sql, "SELECT signal_desc_map_id FROM Signal_Map WHERE signal_desc_id=%d AND "
            "((range_start=0 AND range_stop=0) OR (range_start=0 AND range_stop>=%d) OR "
            "(range_stop=0  AND range_start<=%d) OR (range_start>0 AND range_start<=%d AND "
            "range_stop>0 AND range_stop>=%d))", signal_desc_id,
            exp_number, exp_number, exp_number, exp_number);

    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapData", 999, "Database Query failed.");
        depth = 0;
        PQclear(DBQuery);
        return rc;
    }

    nrows = PQntuples(DBQuery);            // Number of Rows

    if (nrows == 1) {                // Single valid Mapping found
        if (!sqlSignalDesc(DBConnect, PQgetvalue(DBQuery, 0, 0), signal_desc)) {    // Does this map to another?
            depth = 0;
            PQclear(DBQuery);
            return 0;
        }
        if (signal_desc->signal_map_id > 0) {    // Another mapping! Drill down to final valid  mapping.

            if (depth++ <= MAXMAPDEPTH) {
                initSignalDesc(&signal_desc_map);

                if (depth > 1) {
                    for (i = 0; i < depth; i++) {
                        if (keyList[i] ==
                            signal_desc->signal_desc_id) {    // Mapped back to itself? Use current mapping
                            depth--;

                            IDAM_LOGF(UDA_LOG_DEBUG, "sqlMapData[%d] Name Mapped to Itself\n", depth);

                            *signal_desc = signal_desc_map;
                            rc = 1;
                            PQclear(DBQuery);
                            return rc;
                        }
                    }
                }

                keyList[depth] = signal_desc->signal_desc_id;

                if (!sqlMapData(DBConnect, signal_desc->signal_desc_id, exp_number, &signal_desc_map)) {
                    depth = 0;
                    PQclear(DBQuery);
                    return 0;
                }
                depth--;

                if (signal_desc_map.signal_desc_id != 0) {
                    *signal_desc = signal_desc_map;
                }        // Update with the latest mapping

            } else {
                depth = 0;
                PQclear(DBQuery);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapData", 1,
                             "Signal Name Mapping is recursive - Depth exceeded!");
                return 0;
            }
        }
        rc = 1;
    } else {
        if (nrows == 0) {
            rc = 1;        // No valid mapping found
        } else {
            rc = 0;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapData", 999,
                         "Multiple valid Signal Name mappings were found when only one was expected - Please contact the system administrator.");
        }
    }

    PQclear(DBQuery);

    return rc;
}

int sqlMapPrivateData(PGconn* DBConnect, REQUEST_BLOCK request_block, SIGNAL_DESC* signal_desc)
{

// Identify the Mapping between Signal names (new to legacy or vice-versa) within Private Files ignoring shot number range
// and pass number constraints.

// Alias or Generic names have no context wrt private files so only 'true' names are examined.

// If more than one record is found and the names are different, an error is raised because of the ambiguity.

// If a legitimate (single) mapping is found, the mapped name is returned in the signal_desc structure. Return code 1.
// If a problem the returned code=0.

    int i, nrows, rc = 0;
    char sql[MAXSQL];

    PGresult* DBQuery = NULL;
    //ExecStatusType DBQueryStatus;

    IDAM_LOG(UDA_LOG_DEBUG, "sqlMapPrivateData\n");

//-------------------------------------------------------------
// Escape SIGNAL to protect against SQL Injection

    char* signal = (char*)malloc((strlen(request_block.signal) + 1) * sizeof(char));
    strcpy(signal, request_block.signal);
    if (preventSQLInjection(DBConnect, &signal)) {
        if (signal != NULL) free((void*)signal);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapPrivateData", err, "Unable to Escape the signal name!");
        return 0;
    }

//-------------------------------------------------------------
// Build SQL: Locate a signal name mapping ignoring shot and pass number constraints
// Assume old maps to new, then new to old

    for (i = 0; i < 2; i++) {

        if (i == 0)
            sprintf(sql, "SELECT new_name FROM Signal_Map_View WHERE old_name='%s'", signal);
        else
            sprintf(sql, "SELECT old_name FROM Signal_Map_View WHERE new_name='%s'", signal);

        IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapPrivateData", 999, "Database Query failed.");
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return rc;
        }

        nrows = PQntuples(DBQuery);        // Number of Rows

        if (nrows == 1) {                // Single valid Mapping found
            rc = 1;
            strcpy(signal_desc->signal_name, PQgetvalue(DBQuery, 0, 0));
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return rc;
        } else if (nrows > 1) {                // Ambiguous if the names differ: check that all names match
            int j;
            for (j = 1; j < nrows; j++) {
                if (strcmp(PQgetvalue(DBQuery, j, 0), PQgetvalue(DBQuery, 0, 0)) != 0) {
                    addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMapPrivateData", 999,
                                 "Signal Name Mapping is Ambiguous!");
                    PQclear(DBQuery);
                    if (signal != NULL) free((void*)signal);
                    return rc;
                }
            }
            rc = 1;
            strcpy(signal_desc->signal_name,
                   PQgetvalue(DBQuery, 0, 0));    // The names must be the same so no ambiguity
            PQclear(DBQuery);
            if (signal != NULL) free((void*)signal);
            return rc;

        } else {

            PQclear(DBQuery);
        }
    }

    if (signal != NULL) free((void*)signal);

    return rc;
}


int sqlMatch(PGconn* DBConnect, int signal_desc_id, char* originalSourceAlias, char* type,
             int exp_number, int pass, char* originalTPass, int* source_id)
{
//
// If the Return Code is 0 then a Problem occured: Perhaps there is No Signal!
//

    int nrows, ncols, rc = 0, cost;
    char* latest = NULL;

    char sql0[MAXSQL];
    char sql[MAXSQL];
    char maxpass[MAXKEY] = "";

    PGresult* DBQuery = NULL;

    struct timeval tv_start, tv_end;

    *source_id = 0;

//-------------------------------------------------------------
// Escape SOURCE_ALIAS and TPASS to protect against SQL Injection

    char* source_alias = (char*)malloc((strlen(originalSourceAlias) + 1) * sizeof(char));
    strcpy(source_alias, originalSourceAlias);
    if (preventSQLInjection(DBConnect, &source_alias)) {
        if (source_alias != NULL) free((void*)source_alias);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", err, "Unable to Escape the source alias name!");
        return 0;
    }

    char* tpass = (char*)malloc((strlen(originalTPass) + 1) * sizeof(char));
    strcpy(tpass, originalTPass);
    if (preventSQLInjection(DBConnect, &tpass)) {
        if (source_alias != NULL) free((void*)source_alias);
        if (tpass != NULL) free((void*)tpass);
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", err, "Unable to Escape the tpass string!");
        return 0;
    }

//-------------------------------------------------------------
// Resolve the tpass argument given the Identified signal

    if (strlen(tpass) > 0) {
        if (pass == -1) {
            if ((latest = (char*)strcasestr(tpass, "LATEST")) != NULL) {
                if (strlen(latest) > 6) {
                    if (!sqlLatestPass(DBConnect, source_alias, *type, exp_number, maxpass)) {
                        rc = 999;
                        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", rc,
                                     "Unable to Identify the Latest (Maximum Valued) Pass Number");
                        if (source_alias != NULL) free((void*)source_alias);
                        if (tpass != NULL) free((void*)tpass);
                        return rc;
                    }
                    latest = latest + 6;        // Only need the remaining text for the SQL query
                } else {
                    latest = NULL;            // Latest is the default behaviour so no need to do anything!
                }
            } else {
                rc = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", rc,
                             "The Pass String does not contain the LATEST directive when one is expected - Please correct");
                if (source_alias != NULL) free((void*)source_alias);
                if (tpass != NULL) free((void*)tpass);
                return rc;
            }
        } else {
            rc = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", rc,
                         "The Pass number requested is Unclear - Please Correct");
            if (source_alias != NULL) free((void*)source_alias);
            if (tpass != NULL) free((void*)tpass);
            return rc;
        }
    }

//-------------------------------------------------------------
// Build SQL

    sprintf(sql, "SELECT data_source.source_id, data_source.pass FROM data_source, signal, signal_desc WHERE "
            "signal_desc.signal_desc_id=%d AND data_source.exp_number=%d AND data_source.source_alias='%s' AND "
            "data_source.type='%s' ",
            signal_desc_id, exp_number, source_alias, type);

    sql0[0] = '\0';
    if (pass > -1) {
        sprintf(sql0, "AND pass = %d ", pass);
    } else {
        if (latest != NULL) sprintf(sql0, "AND pass = %s%s ", maxpass, latest);
    }
    if (sql0[0] != '\0') strcat(sql, sql0);

    sprintf(sql0,
            "AND signal.signal_desc_id=%d AND data_source.source_id=signal.source_id ORDER BY data_source.pass DESC",
            signal_desc_id);
    strcat(sql, sql0);

//-------------------------------------------------------------
// Test Performance

    cost = gettimeofday(&tv_start, NULL);
    IDAM_LOGF(UDA_LOG_DEBUG, "%s\n", sql);

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        addIdamError(&idamerrorstack, CODEERRORTYPE, "sqlMatch", 1, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (source_alias != NULL) free((void*)source_alias);
        if (tpass != NULL) free((void*)tpass);
        return rc;
    }

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    cost = gettimeofday(&tv_end, NULL);
    cost = (int)(tv_end.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_end.tv_usec - tv_start.tv_usec) / 1000;
    IDAM_LOG(UDA_LOG_DEBUG, "+++ sqlMatch +++\n");
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Time: %d (ms)\n", cost);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Rows: %d\n", nrows);
    IDAM_LOGF(UDA_LOG_DEBUG, "No. Cols: %d\n", ncols);
    IDAM_LOGF(UDA_LOG_DEBUG, "SQL Msg : %s\n", PQresultErrorMessage(DBQuery));

//-------------------------------------------------------------
// Process First Record

    if (nrows == 1) {
        rc = 1;
        *source_id = atoi(PQgetvalue(DBQuery, 0, 0));
    }

    PQclear(DBQuery);
    if (source_alias != NULL) free((void*)source_alias);
    if (tpass != NULL) free((void*)tpass);

    return rc;
}


//==============================================================================================================
#else

PGconn* startSQL() {
    return NULL;
}

PGconn* startSQL_CPF() {
    return NULL;
}

void PQfinish(PGconn *DBConnect) {}

void sqlReason(PGconn *DBConnect, char *reason_id, char *reason) {
    return;
}

void sqlResult(PGconn *DBConnect, char *run_id, char *desc) {
    return;
}

void sqlStatusDesc(PGconn *DBConnect, char *status_desc_id, char *desc) {
    return;
}

void sqlMeta(PGconn *DBConnect, char * table, char *meta_id, char *xml, char *creation) {
    return;
}

int sqlGeneric(PGconn *DBConnect, char *signal, int exp_number, int pass, char *tpass,
               SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
               DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlNoIdamSignal(PGconn *DBConnect, char *signal, int exp_number, int pass, char *tpass,
                    SIGNAL *signal_str,  SIGNAL_DESC *signal_desc_str,
                    DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlComposite(PGconn *DBConnect, char *signal, int exp_number, SIGNAL_DESC *signal_desc_str) {
    return 0;
}

int sqlDocument(PGconn *DBConnect, char *signal, int exp_number, int pass,
                SIGNAL_DESC *signal_desc_str, DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlExternalGeneric(PGconn *DBConnect, char *archive, char *device, char *signal, int exp_number, int pass,
                       SIGNAL *signal_str, SIGNAL_DESC *signal_desc_str, DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlNoSignal(PGconn *DBConnect, char *archive, char *device, char *signal, int exp_number, int pass,
                SIGNAL *signal_str, SIGNAL_DESC *signal_desc_str, DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlDataSystem(PGconn *DBConnect, int pkey, DATA_SYSTEM *str) {
    return 0;
}

int sqlSystemConfig(PGconn *DBConnect, int pkey, SYSTEM_CONFIG *str) {
    return 0;
}

int sqlArchive(PGconn *DBConnect, char *archive, DATA_SOURCE *data_source_str) {
    return 0;
}

int sqlLatestPass(PGconn *DBConnect, char *source_alias, char type, int exp_number, char *maxpass) {
    return(-1);
}

int sqlAltData(PGconn *DBConnect, REQUEST_BLOCK request_block, int rank, SIGNAL_DESC *signal_desc,char *mapping) {
    return 0;
}

int sqlMapPrivateData(PGconn *DBConnect, REQUEST_BLOCK request_block, SIGNAL_DESC *signal_desc) {
    return 0;
}

#endif





