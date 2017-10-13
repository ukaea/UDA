// Return the Shot Date and Time from the IDAM database
//
//---------------------------------------------------------------------------------------------------------

#include "getExpDateTime.h"

#include <logging/logging.h>
#include <clientserver/udaDefines.h>

int getExpDateTime(PGconn* DBConnect, int exp_number, char* shotdate, char* shottime, int verbose) {

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    int nrows;
    char sql[MAXSQL];

    sprintf(sql, "SELECT exp_date, exp_time FROM ExpDateTime WHERE exp_number=%d;", exp_number);

// Execute the Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            UDA_LOG(UDA_LOG_ERROR, "Failure to Execute SQL: %s\n", sql);
            UDA_LOG(UDA_LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return -1;
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            UDA_LOG(UDA_LOG_ERROR, "Problem Identifying Data System: %s\n", sql);
            UDA_LOG(UDA_LOG_ERROR, "%s\n", PQresultErrorMessage(DBQuery));
        }
        return (-1);
    }

    nrows = PQntuples(DBQuery);

    if (nrows == 0) {
        PQclear(DBQuery);
        return 0;
    }

    strcpy(shotdate, PQgetvalue(DBQuery, 0, 0));
    strcpy(shottime, PQgetvalue(DBQuery, 0, 1));

    PQclear(DBQuery);

    return nrows;
}
