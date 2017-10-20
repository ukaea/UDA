// roidamlib.c
//
// IDL DLM C Code: RO Meta Data Tool Function Library
//
//---------------------------------------------------------------------
#include "roidamlib.h"

#include <stdlib.h>
#include <libpq-fe.h>
#include <strings.h>

#include <clientserver/stringUtils.h>

PGconn* DBConnect = NULL;

//-------------------------------------------------------------

void logNonAuthorisation(char* user, char* target, char* name) {

}


int checkNameinList(char* user, char* list, char* delim) {
    int ntest = 128;    // Limit the Size of the List
    char* test, * hlist, * clist;

    if ((test = (char*) strcasestr((const char*) list, (const char*) user)) == NULL)
        return (1);    // Is the name contained in the list?

    if (strlen(test) == strlen(user)) return 0;                // name and list are identical (case ignored)

// Check the name is not a component of another name in a List - Parse the list and check each name

    if ((hlist = (char*) malloc((strlen(list) + 1) * sizeof(char))) == NULL) return (2);    // No Heap Available!

    clist = hlist;                // Copy the heap start address for the final free (strtok does strange things!)
    strcpy(clist, list);

    if ((test = strtok(clist, delim)) == NULL) {    // No Delimiters found!
        free((void*) hlist);
        return (1);
    }

    if (STR_IEQUALS(user, test)) {            // match Found on First attempt
        free((void*) hlist);
        return 0;
    }

    while ((test = strtok(NULL, delim)) != NULL &&
           ntest-- < 0) {    // If the name from the list agrees with then is In List
//fprintf(stdout,"%d: %s \n", ntest, test);
        if (STR_IEQUALS(user, test)) {
            free((void*) hlist);
            return 0;
        }
    }
    free((void*) hlist);
    return (1);                    // Name Not Found in List
}


int checkSignalAuthorisation(int isKey, char* whr, char* user, int verbose, FILE* fh) {
    int rc = 0, nrows;
    char source[4] = "";
    char sql[MAXSQL];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

//-------------------------------------------------------------  
// Build SQL

    if (isKey) {
        strcpy(sql, "SELECT signal_desc_id, signal_name, type, signal_owner FROM signal_desc WHERE signal_desc_id = ");
        strcat(sql, whr);
        strcat(sql, ";");
    } else {
        strcpy(sql, "SELECT signal_desc_id, signal_name, type, signal_owner FROM signal_desc WHERE signal_name = '");
        strcat(sql, whr);
        strcat(sql, "';");
    }

//fprintf(stdout,"SQL: %s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Checking User Authorisation\n");
            fprintf(fh, "%s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    if ((DBQueryStatus = PQresultStatus(DBQuery)) != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Executing Security Check %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 0 || nrows > 1) {
        if (verbose) fprintf(fh, "Unable to Locate the Signal Description Record: %s\n", whr);
        PQclear(DBQuery);
        return (1);
    }

    if (strlen(PQgetvalue(DBQuery, 0, 3)) > 0) {
        if (STR_IEQUALS(user, PQgetvalue(DBQuery, 0, 3)) != 1) {
            if (verbose)
                fprintf(fh, "User %s does NOT have permission to update the Signal %s \n", user,
                        PQgetvalue(DBQuery, 0, 1));
            PQclear(DBQuery);
            return (1);
        }
        rc = 0;
    } else {
        rc = 1;
        if (STR_IEQUALS("R", PQgetvalue(DBQuery, 0, 2)) ||
            STR_IEQUALS("A", PQgetvalue(DBQuery, 0, 2))) {    // Is it an IDA File Signal?
            strncpy(source, PQgetvalue(DBQuery, 0, 1), 3);
            source[3] = '\0';
            rc = checkSourceAuthorisation(source, user, verbose, fh);
        }
        if (rc == 1 && verbose)
            fprintf(fh, "Unable to Check if user %s has permission to update the Signal %s \n", user,
                    PQgetvalue(DBQuery, 0, 1));
    }

    PQclear(DBQuery);

    return rc;
}

int checkSourceAuthorisation(char* source, char* user, int verbose, FILE* fh) {
    int rc = 0, nrows;
    char sql[MAXSQL];
    char* source_list = NULL;

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

//-------------------------------------------------------------  
// Build SQL

    strcpy(sql, "SELECT username, name, source_list FROM security WHERE username = '");
    strcat(sql, user);
    strcat(sql, "';");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Checking User Authorisation\n");
            fprintf(fh, "%s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    if ((DBQueryStatus = PQresultStatus(DBQuery)) != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Executing Security Check %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 0 || nrows > 1) {
        if (verbose) fprintf(fh, "Unable to Locate the Security Record for the User: %s\n", user);
        PQclear(DBQuery);
        return (1);
    }

    rc = 1;
    source_list = PQgetvalue(DBQuery, 0, 2);

    if (strlen(source_list) > 0) {
        if (strcasestr(source_list, "admin") == NULL) {
            if (strcasestr(source_list, source) != NULL)        // Needs strengthening if non IDA source aliases
                rc = 0;
            else
                rc = 1;
        } else {
            rc = 0;    // Administrator at work!
        }
    }
    if (rc && verbose) fprintf(fh, "User %s does NOT have permission to update the Source %s \n", user, source);

    PQclear(DBQuery);
    return rc;
}

int checkAdminAuthorisation(char* user, int verbose, FILE* fh) {
    int rc = 0, nrows;
    char sql[MAXSQL];
    char* source_list;

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

//-------------------------------------------------------------  
// Build SQL

    strcpy(sql, "SELECT username, name, source_list FROM security WHERE username = '");
    strcat(sql, user);
    strcat(sql, "';");

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Checking User Authorisation\n");
            fprintf(fh, "%s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    if ((DBQueryStatus = PQresultStatus(DBQuery)) != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure Executing Security Check %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 0 || nrows > 1) {
        if (verbose) fprintf(fh, "Unable to Locate the Security Record for the User: %s\n", user);
        PQclear(DBQuery);
        return (1);
    }

    rc = 1;
    source_list = PQgetvalue(DBQuery, 0, 2);
    if (strlen(source_list) > 0) {
        if (strcasestr(source_list, "admin") != NULL) {
            // Administrator at work!
            rc = 0;
        }
    }
    if (rc && verbose) fprintf(fh, "User %s is Not an Administrator\n", user);

    PQclear(DBQuery);
    return rc;
}

//-------------------------------------------------------------------------
// Pattern Matching: Substitute * and ? for % and _ if Not Escaped

void sqlIdamPatternMatch(char* in, char* out) {

    int i, j = 0;
    size_t l = strlen(in);

    for (i = 0; i < l; i++) {
        if (in[i] == '%' || in[i] == '_') {            // Escape SQL Pattern Matching Characters: % -> \%, _ -> \_
            if (i == 0 || (i > 0 && in[i - 1] != '\\')) {
                out[j++] = '\\';
                out[j++] = in[i];
            } else {
                out[j++] = in[i];
            }
        } else {
            if (in[i] == '*') {
                if (i == 0 || (i > 0 && in[i - 1] != '\\')) {
                    out[j++] = '%';                // Replace All occurances of * with %
                } else {
                    out[j++] = in[i];
                }
            } else {
                if (in[i] == '?') {
                    if (i == 0 || (i > 0 && in[i - 1] != '\\')) {
                        out[j++] = '_';
                    } else {
                        out[j++] = in[i];
                    }                    // Replace All occurances of ? with _
                } else {
                    out[j++] = in[i];
                }
            }
        }
    }
    out[j] = '\0';
}


//-------------------------------------------------------------------------
// Simple Query against a specified Table

int queryIdamTable(char* sql, int verbose, FILE* fh, char* out) {

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

// Execute the SQL

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    strcpy(out, PQgetvalue(DBQuery, 0, 0));
    PQclear(DBQuery);
    return 0;
}


//-------------------------------------------------------------------------
// Simple Count against a specified Table

int countIdamTable(char* sql, int verbose, FILE* fh) {

    int count = 0;
    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

//-------------------------------------------------------------  
// Execute the SQL

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return 0;
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return 0;
    }

    count = atoi(PQgetvalue(DBQuery, 0, 0));
    PQclear(DBQuery);
    return (count);
}

//-------------------------------------------------------------------------
// Execute SQL without any Returned Rows

int executeIdamSQL(char* sql, int verbose, FILE* fh) {

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

//-------------------------------------------------------------  
// Execute the SQL

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_COMMAND_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (1);
    }
    PQclear(DBQuery);
    return 0;
}

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
// Combine Action Data Structures: Old + New unless the REPLACE Keyword was used - then only replace
// old with new. If old exists where no new exists, then old is preserved.
//
// Splitting Ranges has too many combinations: 9 in exp_number & 9 in range = 81 combinations 
// so to simplify, RO should delete and Start again if a problem exists

void copyXMLRanges(ACTION in, ACTION* out) {
    out->exp_range[0] = in.exp_range[0];
    out->exp_range[1] = in.exp_range[1];
    out->pass_range[0] = in.pass_range[0];
    out->pass_range[1] = in.pass_range[1];
}

int combineIdamActions(int replace, ACTIONS newactions, ACTIONS* actions) {

    int j, iold, inew, nact, nold, nnew;
    int timeoffsetActionId = 0;
    int documentActionId = 0;
    int calibrationActionId = 0;
    int errormodelActionId = 0;
    int compositeActionId = 0;

    ACTION* act = actions->action, * new;
    void* cptr;

    nold = actions->nactions;
    nnew = newactions.nactions;

    for (inew = 0; inew < nnew; inew++) {    // Scan through All New Actions

        switch (newactions.action[inew].actionType) {

//------------------------------------------------------------------------------------------
// Time Offset: Add New to Old

            case TIMEOFFSETTYPE: {
                new = &newactions.action[inew];
                if (!replace) {
                    if (timeoffsetActionId == 0) {
                        for (iold = 0; iold < nold; iold++) {
                            if (actions->action[iold].actionType == TIMEOFFSETTYPE
                                && timeoffsetActionId < actions->action[iold].actionId)
                                timeoffsetActionId = actions->action[iold].actionId;
                        }
                    }
                } else {
                    for (iold = 0; iold < nold; iold++) {
                        if (actions->action[iold].actionType == TIMEOFFSETTYPE) {
                            actions->action[iold].timeoffset.offset = (double) 0.0;            // Ignore These!
                            actions->action[iold].timeoffset.interval = (double) 0.0;
                            actions->action[iold].timeoffset.method = 0;
                        }
                    }
                }
                nact = actions->nactions + 1;
                act = (ACTION*) realloc((void*) act, nact * sizeof(ACTION));        // Add a New Structure Element
                actions->nactions = nact;
                actions->action = act;
                initAction(&act[nact - 1]);
                act[nact - 1].actionType = TIMEOFFSETTYPE;
                initTimeOffset(&act[nact - 1].timeoffset);
                copyXMLRanges(*new, &act[nact - 1]);
                act[nact - 1].actionId = ++timeoffsetActionId;
                act[nact - 1].timeoffset = new->timeoffset;                // Copy New Action
                printAction(act[nact - 1]);
                break;
            }

//------------------------------------------------------------------------------------------
// Documentation: Add New to Old

            case DOCUMENTATIONTYPE: {
                new = &newactions.action[inew];
                if (!replace) {
                    if (documentActionId == 0) {
                        for (iold = 0; iold < nold; iold++) {
                            if (actions->action[iold].actionType == DOCUMENTATIONTYPE
                                && documentActionId < actions->action[iold].actionId)
                                documentActionId = actions->action[iold].actionId;
                        }
                    }
                } else {
                    for (iold = 0; iold < nold; iold++) {
                        if (actions->action[iold].actionType == DOCUMENTATIONTYPE) {
                            actions->action[iold].documentation.description[0] = '\0';            // Ignore These!
                            actions->action[iold].documentation.label[0] = '\0';
                            actions->action[iold].documentation.units[0] = '\0';
                            if ((cptr = (void*) actions->action[iold].documentation.dimensions) != NULL) {
                                free((void*) cptr);
                                actions->action[iold].documentation.dimensions = NULL;
                                actions->action[iold].documentation.ndimensions = 0;
                            }
                        }
                    }
                }
                nact = actions->nactions + 1;
                act = (ACTION*) realloc((void*) act, nact * sizeof(ACTION));        // Add a New Structure Element
                actions->nactions = nact;
                actions->action = act;
                initAction(&act[nact - 1]);
                act[nact - 1].actionType = DOCUMENTATIONTYPE;
                initDocumentation(&act[nact - 1].documentation);
                copyXMLRanges(*new, &act[nact - 1]);
                act[nact - 1].actionId = ++documentActionId;
                act[nact - 1].documentation = new->documentation;            // Copy New Action
                new->documentation.ndimensions = 0;
                new->documentation.dimensions = NULL;                // Prevent a Double Heap Free
                printAction(act[nact - 1]);
                break;
            }

//------------------------------------------------------------------------------------------
// Calibration: Add New to Old

            case CALIBRATIONTYPE: {
                new = &newactions.action[inew];
                if (!replace) {
                    if (calibrationActionId == 0) {
                        for (iold = 0; iold < nold; iold++) {
                            if (actions->action[iold].actionType == CALIBRATIONTYPE
                                && calibrationActionId < actions->action[iold].actionId)
                                calibrationActionId = actions->action[iold].actionId;
                        }
                    }
                } else {
                    for (iold = 0; iold < nold; iold++) {
                        if (actions->action[iold].actionType == CALIBRATIONTYPE) {
                            actions->action[iold].calibration.factor = (double) 1.0;        // Ignore These!
                            actions->action[iold].calibration.offset = (double) 0.0;
                            if ((cptr = (void*) actions->action[iold].calibration.dimensions) != NULL) {
                                free((void*) cptr);
                                actions->action[iold].calibration.dimensions = NULL;
                                actions->action[iold].calibration.ndimensions = 0;
                            }
                        }
                    }
                }
                nact = actions->nactions + 1;
                act = (ACTION*) realloc((void*) act, nact * sizeof(ACTION));        // Add a New Structure Element
                actions->nactions = nact;
                actions->action = act;
                initAction(&act[nact - 1]);
                act[nact - 1].actionType = CALIBRATIONTYPE;
                initCalibration(&act[nact - 1].calibration);
                copyXMLRanges(*new, &act[nact - 1]);
                act[nact - 1].actionId = ++calibrationActionId;
                act[nact - 1].calibration = new->calibration;                // Copy New Action
                new->calibration.ndimensions = 0;
                new->calibration.dimensions = NULL;                // Prevent a Double Heap Free
                printAction(act[nact - 1]);
                break;
            }

//------------------------------------------------------------------------------------------
// Error Models: Add New to Old

            case ERRORMODELTYPE: {
                new = &newactions.action[inew];
                if (!replace) {
                    if (errormodelActionId == 0) {
                        for (iold = 0; iold < nold; iold++) {
                            if (actions->action[iold].actionType == ERRORMODELTYPE
                                && errormodelActionId < actions->action[iold].actionId)
                                errormodelActionId = actions->action[iold].actionId;
                        }
                    }
                } else {
                    for (iold = 0; iold < nold; iold++) {
                        if (actions->action[iold].actionType == ERRORMODELTYPE) {

                            //if((cptr=(void *)actions->action[iold].errormodel.params) !=NULL) free((void *)cptr);
                            //actions->action[iold].errormodel.params  = NULL;
                            actions->action[iold].errormodel.param_n = 0;
                            for (j = 0; j < actions->action[iold].errormodel.ndimensions; j++) {
                                if (actions->action[iold].errormodel.dimensions[j].dimType == DIMERRORMODELTYPE) {
                                    //if((cptr=(void *)actions->action[iold].errormodel.dimensions[j].dimerrormodel.params) !=NULL){
                                    //   free((void *)cptr);
                                    actions->action[iold].errormodel.dimensions[j].dimerrormodel.param_n = 0;
                                    actions->action[iold].errormodel.dimensions[j].dimerrormodel.model = ERROR_MODEL_UNKNOWN;
                                    //}
                                }
                            }
                            if ((cptr = (void*) actions->action[iold].errormodel.dimensions) != NULL) {
                                free((void*) cptr);
                                actions->action[iold].errormodel.dimensions = NULL;
                                actions->action[iold].errormodel.ndimensions = 0;
                            }
                        }
                    }
                }
                nact = actions->nactions + 1;
                act = (ACTION*) realloc((void*) act, nact * sizeof(ACTION));        // Add a New Structure Element
                actions->nactions = nact;
                actions->action = act;
                initAction(&act[nact - 1]);
                act[nact - 1].actionType = ERRORMODELTYPE;
                initErrorModel(&act[nact - 1].errormodel);
                copyXMLRanges(*new, &act[nact - 1]);
                act[nact - 1].actionId = ++errormodelActionId;
                act[nact - 1].errormodel = new->errormodel;                // Copy New Action
                new->errormodel.ndimensions = 0;
                new->errormodel.dimensions = NULL;                    // Prevent a Double Heap Free
                printAction(act[nact - 1]);
                break;
            }

//------------------------------------------------------------------------------------------
// Error Models: Add New to Old

            case COMPOSITETYPE: {
                new = &newactions.action[inew];
                if (!replace) {
                    if (compositeActionId == 0) {
                        for (iold = 0; iold < nold; iold++) {
                            if (actions->action[iold].actionType == COMPOSITETYPE
                                && compositeActionId < actions->action[iold].actionId)
                                compositeActionId = actions->action[iold].actionId;
                        }
                    }
                } else {
                    for (iold = 0; iold < nold; iold++) {
                        if (actions->action[iold].actionType == COMPOSITETYPE) {
                            actions->action[iold].composite.data_signal[0] = '\0';
                            if ((cptr = (void*) actions->action[iold].composite.dimensions) != NULL) {
                                free((void*) cptr);
                                actions->action[iold].composite.dimensions = NULL;
                                actions->action[iold].composite.ndimensions = 0;
                            }
                        }
                    }
                }
                nact = actions->nactions + 1;
                act = (ACTION*) realloc((void*) act, nact * sizeof(ACTION));        // Add a New Structure Element
                actions->nactions = nact;
                actions->action = act;
                initAction(&act[nact - 1]);
                act[nact - 1].actionType = COMPOSITETYPE;
                initComposite(&act[nact - 1].composite);
                copyXMLRanges(*new, &act[nact - 1]);
                act[nact - 1].actionId = ++compositeActionId;
                act[nact - 1].composite = new->composite;                // Copy New Action
                new->composite.ndimensions = 0;
                new->composite.dimensions = NULL;                    // Prevent a Double Heap Free
                printAction(act[nact - 1]);
                break;
            }

        }        // End of Switch

    }        // End of Old Scan

    return 0;
}

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
// Create XML from the Action Data Structures

int createIdamActionXML(ACTIONS actions, char* xml) {
    int i, j, iact;
    int timeoffsetActionId = 1;
    int documentActionId = 1;
    int calibrationActionId = 1;
    int errormodelActionId = 1;
    int compositeActionId = 1;

    char xml_str[256] = "";
    char lfcr[3] = "  ";

    lfcr[0] = 10;
    lfcr[1] = 13;

    strcpy(xml, "<?xml version=\"1.0\"?>");    // XML Header
    strcat(xml, lfcr);
    strcat(xml, "<action>");
    strcat(xml, lfcr);
    strcat(xml, "  <signal>");
    strcat(xml, lfcr);

    for (iact = 0; iact < actions.nactions; iact++) {    // Scan through All Actions

        switch (actions.action[iact].actionType) {

//------------------------------------------------------------------------------------------
// Time Offset: 

            case TIMEOFFSETTYPE: {
                if (actions.action[iact].timeoffset.offset == 0.0 && actions.action[iact].timeoffset.method == 0) break;
                if (actions.action[iact].timeoffset.method == 0) {
                    sprintf(xml_str, "    <time_offset id=\"%d\" method=\"0\" value=\"%f\" "
                                    "exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\" />",
                            timeoffsetActionId++, actions.action[iact].timeoffset.offset,
                            actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                            actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                } else {
                    sprintf(xml_str, "    <time_offset id=\"%d\" method=\"1\" start=\"%f\" interval=\"%f\" "
                                    "exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\" />",
                            timeoffsetActionId++, actions.action[iact].timeoffset.offset,
                            actions.action[iact].timeoffset.interval,
                            actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                            actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                }
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                break;
            }

//------------------------------------------------------------------------------------------   
// Documentation:

            case DOCUMENTATIONTYPE: {

                if (strlen(actions.action[iact].documentation.description) == 0 &&
                    strlen(actions.action[iact].documentation.label) == 0 &&
                    strlen(actions.action[iact].documentation.units) == 0 &&
                    actions.action[iact].documentation.ndimensions == 0)
                    break;

                sprintf(xml_str,
                        "    <documentation id=\"%d\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                        documentActionId++, actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                        actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                if (strlen(actions.action[iact].documentation.description) > 0) {
                    sprintf(xml_str, "      <description>%s</description>",
                            actions.action[iact].documentation.description);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }
                if (strlen(actions.action[iact].documentation.label) > 0) {
                    sprintf(xml_str, "      <label>%s</label>", actions.action[iact].documentation.label);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }
                if (strlen(actions.action[iact].documentation.units) > 0) {
                    sprintf(xml_str, "      <units>%s</units>", actions.action[iact].documentation.units);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }
                for (i = 0; i < actions.action[iact].documentation.ndimensions; i++) {
                    sprintf(xml_str, "      <dimension dimid=\"%d\">",
                            actions.action[iact].documentation.dimensions[i].dimid);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                    if (strlen(actions.action[iact].documentation.dimensions[i].dimdocumentation.label) > 0) {
                        sprintf(xml_str, "        <label>%s</label>",
                                actions.action[iact].documentation.dimensions[i].dimdocumentation.label);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                    if (strlen(actions.action[iact].documentation.dimensions[i].dimdocumentation.units) > 0) {
                        sprintf(xml_str, "        <units>%s</units>",
                                actions.action[iact].documentation.dimensions[i].dimdocumentation.units);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                    strcat(xml, "      </dimension>");
                    strcat(xml, lfcr);
                }
                strcat(xml, "    </documentation>");
                strcat(xml, lfcr);
                break;
            }

//------------------------------------------------------------------------------------------   
// Calibration

            case CALIBRATIONTYPE: {

                if (actions.action[iact].calibration.factor == (double) 1.0 &&
                    actions.action[iact].calibration.offset == (double) 0.0 &&
                    actions.action[iact].calibration.ndimensions == 0)
                    break;

                if (strlen(actions.action[iact].calibration.target) > 0)
                    sprintf(xml_str,
                            "    <calibration id=\"%d\" target=\"%s\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                            calibrationActionId++, actions.action[iact].calibration.target,
                            actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                            actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                else
                    sprintf(xml_str,
                            "    <calibration id=\"%d\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                            calibrationActionId++,
                            actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                            actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                if (actions.action[iact].calibration.factor != (double) 1.0) {
                    sprintf(xml_str, "      <factor>%f</factor>", actions.action[iact].calibration.factor);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }
                if (actions.action[iact].calibration.offset != (double) 0.0) {
                    sprintf(xml_str, "      <offset>%f</offset>", actions.action[iact].calibration.offset);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }
                if (strlen(actions.action[iact].calibration.units) > 0) {
                    sprintf(xml_str, "      <units>%s</units>", actions.action[iact].calibration.units);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                }

                for (i = 0; i < actions.action[iact].calibration.ndimensions; i++) {
                    sprintf(xml_str, "      <dimension dimid=\"%d\">",
                            actions.action[iact].calibration.dimensions[i].dimid);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                    if (actions.action[iact].calibration.dimensions[i].dimcalibration.factor != (double) 1.0) {
                        sprintf(xml_str, "        <factor>%f</factor>",
                                actions.action[iact].calibration.dimensions[i].dimcalibration.factor);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                    if (actions.action[iact].calibration.dimensions[i].dimcalibration.offset != (double) 0.0) {
                        sprintf(xml_str, "        <offset>%f</offset>",
                                actions.action[iact].calibration.dimensions[i].dimcalibration.offset);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                    if (strlen(actions.action[iact].calibration.dimensions[i].dimcalibration.units) > 0) {
                        sprintf(xml_str, "        <units>%s</units>",
                                actions.action[iact].calibration.dimensions[i].dimcalibration.units);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                    strcat(xml, "      </dimension>");
                    strcat(xml, lfcr);
                }

                strcat(xml, "    </calibration>");
                strcat(xml, lfcr);
                break;
            }

//------------------------------------------------------------------------------------------   
// Error Model

            case ERRORMODELTYPE: {
                if (actions.action[iact].errormodel.model == ERROR_MODEL_UNKNOWN &&
                    actions.action[iact].errormodel.param_n == 0 &&
                    actions.action[iact].errormodel.ndimensions == 0)
                    break;

                sprintf(xml_str,
                        "    <errormodel id=\"%d\" model=\"%d\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                        errormodelActionId++, actions.action[iact].errormodel.model,
                        actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                        actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                strcat(xml, "      <params>");
                if (actions.action[iact].errormodel.param_n == 1) {
                    sprintf(xml_str, "%f", actions.action[iact].errormodel.params[0]);
                    strcat(xml, xml_str);
                } else {
                    for (i = 0; i < actions.action[iact].errormodel.param_n; i++) {
                        if (i == actions.action[iact].errormodel.param_n - 1)
                            sprintf(xml_str, "%f", actions.action[iact].errormodel.params[i]);
                        else
                            sprintf(xml_str, "%f,", actions.action[iact].errormodel.params[i]);
                        strcat(xml, xml_str);
                    }
                }
                strcat(xml, "</params>");
                strcat(xml, lfcr);
                for (i = 0; i < actions.action[iact].errormodel.ndimensions; i++) {
                    sprintf(xml_str, "      <dimension dimid=\"%d\" model=\"%d\">",
                            actions.action[iact].errormodel.dimensions[i].dimid,
                            actions.action[iact].errormodel.dimensions[i].dimerrormodel.model);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                    strcat(xml, "        <params>");
                    if (actions.action[iact].errormodel.dimensions[i].dimerrormodel.param_n == 1) {
                        sprintf(xml_str, "%f", actions.action[iact].errormodel.dimensions[i].dimerrormodel.params[0]);
                        strcat(xml, xml_str);
                    } else {
                        for (j = 0; j < actions.action[iact].errormodel.dimensions[i].dimerrormodel.param_n; j++) {
                            if (j == actions.action[iact].errormodel.dimensions[i].dimerrormodel.param_n - 1)
                                sprintf(xml_str, "%f",
                                        actions.action[iact].errormodel.dimensions[i].dimerrormodel.params[i]);
                            else
                                sprintf(xml_str, "%f,",
                                        actions.action[iact].errormodel.dimensions[i].dimerrormodel.params[i]);
                            strcat(xml, xml_str);
                        }
                    }
                    strcat(xml, "</params>");
                    strcat(xml, lfcr);
                    strcat(xml, "      </dimension>");
                    strcat(xml, lfcr);
                }
                strcat(xml, "    </errormodel>");
                strcat(xml, lfcr);
                break;
            }

//------------------------------------------------------------------------------------------   
// Composite Signal

            case COMPOSITETYPE: {

                if (strlen(actions.action[iact].composite.data_signal) == 0 &&
                    actions.action[iact].composite.ndimensions == 0)
                    break;

                sprintf(xml_str, "    <composite id=\"%d\" data=\"%s\"",
                        compositeActionId++, actions.action[iact].composite.data_signal);
                strcat(xml, xml_str);
                if (strlen(actions.action[iact].composite.error_signal) > 0) {
                    sprintf(xml_str, " error=\"%s\"", actions.action[iact].composite.error_signal);
                    strcat(xml, xml_str);
                }
                if (strlen(actions.action[iact].composite.aserror_signal) > 0) {
                    sprintf(xml_str, " aserror=\"%s\"", actions.action[iact].composite.aserror_signal);
                    strcat(xml, xml_str);
                }
                sprintf(xml_str, " exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                        actions.action[iact].exp_range[0], actions.action[iact].exp_range[1],
                        actions.action[iact].pass_range[0], actions.action[iact].pass_range[1]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                for (i = 0; i < actions.action[iact].composite.ndimensions; i++) {
                    sprintf(xml_str, "      <composite_dim dimid=\"%d\"",
                            actions.action[iact].composite.dimensions[i].dimid);
                    strcat(xml, xml_str);
                    if (strlen(actions.action[iact].composite.dimensions[i].dimcomposite.dim_signal) > 0) {
                        sprintf(xml_str, " dim=\"%s\"",
                                actions.action[iact].composite.dimensions[i].dimcomposite.dim_signal);
                        strcat(xml, xml_str);
                    }
                    if (strlen(actions.action[iact].composite.dimensions[i].dimcomposite.dim_error) > 0) {
                        sprintf(xml_str, " error=\"%s\"",
                                actions.action[iact].composite.dimensions[i].dimcomposite.dim_error);
                        strcat(xml, xml_str);
                    }
                    if (strlen(actions.action[iact].composite.dimensions[i].dimcomposite.dim_aserror) > 0) {
                        sprintf(xml_str, " aserror=\"%s\"",
                                actions.action[iact].composite.dimensions[i].dimcomposite.dim_aserror);
                        strcat(xml, xml_str);
                    }
                    if (actions.action[iact].composite.dimensions[i].dimcomposite.to_dim >= 0) {
                        sprintf(xml_str, " to_dim=\"%d\"",
                                actions.action[iact].composite.dimensions[i].dimcomposite.to_dim);
                        strcat(xml, xml_str);
                    }
                    if (actions.action[iact].composite.dimensions[i].dimcomposite.from_dim >= 0) {
                        sprintf(xml_str, " from_dim=\"%d\"",
                                actions.action[iact].composite.dimensions[i].dimcomposite.from_dim);
                        strcat(xml, xml_str);
                    }
                    strcat(xml, " />");
                    strcat(xml, lfcr);
                }
                strcat(xml, "    </composite>");
                strcat(xml, lfcr);
                break;
            }

//------------------------------------------------------------------------------------------
// Tail

        }        // end of switch
    }        // end of loop over actions

    strcat(xml, "  </signal>");
    strcat(xml, lfcr);
    strcat(xml, "</action>");            // XML Tail
    return 0;
} 
