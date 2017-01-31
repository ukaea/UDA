// Return the Latest Pass Number from the IDAM database for a specified source alias  
//
//---------------------------------------------------------------------------------------------------------

#include "getLatestSourcePass.h"

#include <stdlib.h>

#include <logging/idamLog.h>
#include <clientserver/idamDefines.h>

int getLatestSourcePass(PGconn* DBConnect, int exp_number, char* source, int verbose, int* pass) {

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    int nrows;
    char sql[MAXSQL];

    sprintf(sql, "SELECT max(pass) FROM Data_Source WHERE exp_number=%d AND source_alias='%s';", exp_number, source);

// Execute the Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            IDAM_LOGF(LOG_ERROR, "Failure to Execute SQL: %s\n", sql);
            IDAM_LOGF(LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return (-1);
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            IDAM_LOGF(LOG_ERROR, "Problem Identifying Data System: %s\n", sql);
            IDAM_LOGF(LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return (-1);
    }

    nrows = PQntuples(DBQuery);

    if (nrows == 0) {
        PQclear(DBQuery);
        return 0;
    }

    *pass = atoi(PQgetvalue(DBQuery, 0, 0));

    PQclear(DBQuery);

    return (nrows);
}
