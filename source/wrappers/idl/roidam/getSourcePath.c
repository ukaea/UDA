// Return the Directory Path to a Specific Data Source
//
//---------------------------------------------------------------------------------------------------------

#include "getSourcePath.h"

#include <stdlib.h>

#include <logging/idamLog.h>
#include <clientserver/idamDefines.h>
#include <clientserver/stringUtils.h>

int getSourcePath(PGconn* DBConnect, int exp_number, int pass, char* alias, char* type, int iscase, int verbose,
                  char* path) {
    char pass_str[56] = "";

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    int nrows;
    char sql[MAXSQL];

//---------------------------------------------------------------------------------------------------------
// Construct SQL

    if (iscase) {
        sprintf(sql,
                "SELECT path, filename, access, pass, type FROM Data_Source WHERE exp_number=%d AND source_alias ILIKE '%s'",
                exp_number, alias);
    } else {
        sprintf(sql,
                "SELECT path, filename, access, pass, type FROM Data_Source WHERE exp_number=%d AND source_alias = '%s'",
                exp_number, alias);
    }

    sprintf(pass_str, "%d", pass);

    if (pass >= 0) {
        strcat(sql, " AND pass=");            // Must be an Analysed data File
        strcat(sql, pass_str);
    }

    if (strlen(type) > 0) {
        strcat(sql, " AND type = '");        // user Specified Type takes precedence over Pass or Raw
        strcat(sql, type);
        strcat(sql, "'");
    } else {
        if (pass >= 0) {
            strcat(sql, " AND type='A'");        // Must be an Analysed data File
        } else {
            strcat(sql, " AND type='R'");        // Must be a RAW data File
        }
    }

    strcat(sql, " ORDER BY PASS DESC;");


#ifdef DEBUGSQL
    fprintf(stdout, "getSourcePath SQL: %s\n", sql);
#endif

//-------------------------------------------------------------
// Query the IDAM Database 

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            IDAM_LOGF(LOG_ERROR, "ERROR - Failure to Execute SQL: %s\n", sql);
            IDAM_LOGF(LOG_ERROR, "      - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        PQfinish(DBConnect);
        return -1;
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            IDAM_LOGF(LOG_ERROR, "ERROR - Problem Identifying Database Record: %s\n", sql);
            IDAM_LOGF(LOG_ERROR, "      - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        PQfinish(DBConnect);
        return (-1);
    }

    nrows = PQntuples(DBQuery);

    if (nrows > 0) {
        strncpy(type, PQgetvalue(DBQuery, 0, 4), 1);
        type[1] = '\0';
        if (!strcasecmp(type, "A") || !strcasecmp(type, "R")) {
            if (strlen(PQgetvalue(DBQuery, 0, 0)) == 0) {

                sprintf(path, "%s/%03d/%d/", getenv("MAST_DATA"), exp_number / 1000,
                        exp_number);// IDA Data File High Level Directory

                if (!strcasecmp(type, "A")) {
                    strcat(path, "Pass");
                    strcat(path, PQgetvalue(DBQuery, 0, 3));
                } else {
                    strcat(path, "Raw");
                }

            } else {
                strcpy(path, PQgetvalue(DBQuery, 0, 0));
            }
        } else {
            if (!strcasecmp(type, "I")) {
                strcpy(path, PQgetvalue(DBQuery, 0, 0));
                TrimString(path);
                if (strlen(path) == 0)
                    sprintf(path, "%s/%03d/%d", getenv("MAST_IMAGES"), exp_number / 1000, exp_number);
            } else {
                nrows = -1;
                if (verbose) fprintf(stderr, "ERROR - Unknown Data Source Type: %s\n", type);
            }
        }

        if (nrows > 0) {
            nrows = 1;
            if (!strcmp(PQgetvalue(DBQuery, 0, 2), "A")) nrows = -2;    // Offline: Not Available
            if (!strcmp(PQgetvalue(DBQuery, 0, 2), "L")) nrows = -3;    // Linked: Not Available
            strcat(path, "/");
            strcat(path, PQgetvalue(DBQuery, 0, 1));            // Filename
        }
    }

//------------------------------------------------------------- 
// Housekeeping

    PQclear(DBQuery);
    //PQfinish(DBConnect);

    return (nrows);
}

