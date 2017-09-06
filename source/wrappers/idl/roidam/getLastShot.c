// Return the Latest Shot Number from the IDAM database
//
//---------------------------------------------------------------------------------------------------------

#include "getLastShot.h"

#include <stdlib.h>

#include <logging/logging.h>
#include <clientserver/udaDefines.h>

int getLastShot(PGconn* DBConnect, int* exp_number, int verbose, int debug) {

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    int nrows;
    char sql[MAXSQL];

    *exp_number = 0;

    sprintf(sql, "SELECT max(exp_number) FROM ExpDateTime;");        // Use the Last entry in the Date & Time table

    if (debug) fprintf(stdout, "SQL: %s\n", sql);

// Execute the Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            IDAM_LOGF(UDA_LOG_ERROR, "Failure to Execute SQL: %s\n", sql);
            IDAM_LOGF(UDA_LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return (-1);
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            IDAM_LOGF(UDA_LOG_ERROR, "Problem with SQL: %s\n", sql);
            IDAM_LOGF(UDA_LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return (-1);
    }

    nrows = PQntuples(DBQuery);

    if (nrows == 0) {
        PQclear(DBQuery);
        return 0;
    }

    *exp_number = atoi(PQgetvalue(DBQuery, 0, 0));

    PQclear(DBQuery);

    return (nrows);
}
