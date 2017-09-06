#include "roidam.h"		// DLM Header

#include <stdlib.h>
#include <unistd.h>
#include <libpq-fe.h>		// SQL Library Header

#include <client/getEnvironment.h>
#include <clientserver/userid.h>
#include <clientserver/stringUtils.h>

#include "roidamlib.h"
#include "getExpDateTime.h"
#include "getLastShot.h"
#include "getLatestSourcePass.h"
#include "getSourcePath.h"

#define NDEBUG        0

static PGconn* DBConnect;

// Function Prototypes

extern IDL_VPTR IDL_CDECL rosignalxml(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL rosinglesignalxml(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL rosourcestatus(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL rosignalstatus(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL rosignaldesc(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL rosignalclass(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL listsources(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL listallsources(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL listsignals(int argc, IDL_VPTR argv[], char* argk);

//extern IDL_VPTR IDL_CDECL listshots(int argc, IDL_VPTR argv[], char *argk);
extern IDL_VPTR IDL_CDECL listreasons(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL listimpacts(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL roclose(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL getidamlastshot(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL getidamshotdatetime(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL getidamlatestsourcepass(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL getidampath(int argc, IDL_VPTR argv[], char* argk);

extern IDL_VPTR IDL_CDECL roidamhelp(int argc, IDL_VPTR argv[], char* argk);

static int createIdamKWXML(char* signal, KW_RESULT kw, char* xml);

static IDL_SYSFUN_DEF2 roidam_functions[] = {
        {{(IDL_FUN_RET) rosignalxml},             "ROSIGNALXML",             1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) rosinglesignalxml},       "ROSINGLESIGNALXML",       3, 3, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) rosourcestatus},          "ROSOURCESTATUS",          2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) rosignalstatus},          "ROSIGNALSTATUS",          2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) rosignaldesc},            "ROSIGNALDESC",            1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) rosignalclass},           "ROSIGNALCLASS",           1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) listsources},             "LISTSOURCES",             0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) listallsources},          "LISTALLSOURCES",          0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) listsignals},             "LISTSIGNALS",             0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) listreasons},             "LISTREASONS",             0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) listimpacts},             "LISTIMPACTS",             0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) roclose},                 "ROCLOSE",                 0, 0, 0,                         0},
        {{(IDL_FUN_RET) getidamlastshot},         "GETIDAMLASTSHOT",         0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) getidamshotdatetime},     "GETIDAMSHOTDATETIME",     1, 1, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) getidamlatestsourcepass}, "GETIDAMLATESTSOURCEPASS", 2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) getidampath},             "GETIDAMPATH",             2, 2, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
        {{(IDL_FUN_RET) roidamhelp},              "ROIDAMHELP",              0, 0, IDL_SYSFUN_DEF_F_KEYWORDS, 0},
};

int roidam_startup(void) {

    if (!IDL_SysRtnAdd(roidam_functions, TRUE, ARRLEN(roidam_functions))) { return IDL_FALSE; }    // TRUE for Functions

    IDL_ExitRegister(roidam_exit_handler);    // Register the exit handler

    return (IDL_TRUE);
}

int IDL_Load(void) {

    if (!IDL_SysRtnAdd(roidam_functions, TRUE, ARRLEN(roidam_functions))) { return IDL_FALSE; }
    return (IDL_TRUE);
}

// Called when IDL is shutdown

void roidam_exit_handler(void) {
    // Nothing to do!
}

// Free Heap

void freeMem(UCHAR* memPtr) {
    if (NDEBUG) fprintf(stdout, "freeMem: Free Address  : %p\n", (void*) memPtr);
    free((void*) memPtr);
}

//----------------------------------------------------------------------------
// Externals

FILE* dbgout = NULL;        // Debug Log
FILE* errout = NULL;        // Error Log
FILE* logout = NULL;        // Standard Log Output

FILE* dbglog = NULL;
FILE* errlog = NULL;        // Error Log
FILE* stdlog = NULL;

char clientname[STRING_LENGTH] = "";
ENVIRONMENT environment;            // Holds local environment variable values

int debugon = 0; //DEBUG;
int verbose = 0;

//----------------------------------------------------------------------------
// Function Prototypes

IDL_VPTR returnIdamSQL(char* sql, int verbose, int debug, FILE* fh);

IDL_VPTR returnIdamSourceSQL(char* sql, int verbose, int debug, FILE* fh);

IDL_VPTR returnIdamSignalSQL(char* sql, int verbose, int debug, FILE* fh);

IDL_VPTR returnIdamSignalDescSQL(char* sql, int verbose, int debug, FILE* fh);

//============================================================================
//============================================================================
// Self Documentation

IDL_VPTR IDL_CDECL roidamhelp(int argc, IDL_VPTR argv[], char* argk) {
    fprintf(stdout, "Documentation URL: http://fuslwn/trweb/docs/IDAM_RO_IDL_Tools.pdf\n");
    return (IDL_GettmpLong(0));
}


//============================================================================
//----------------------------------------------------------------------------
// SQL Database Connection

PGconn* startSQL() {

    char pgport[56];
    char* pswrd = NULL;
    char* pgoptions = NULL;    //"connect_timeout=5";
    char* pgtty = NULL;

    PGconn* DBConnect = NULL;

//-------------------------------------------------------------
// Identify SQL Service
    fprintf(stdout, "get IDAM Client Environment\n");

    getIdamClientEnvironment(&environment);

//-------------------------------------------------------------
// Connect to the Database Server

    sprintf(pgport, "%d", environment.sql_port);

    if ((DBConnect = PQsetdbLogin(environment.sql_host, pgport, pgoptions, pgtty,
                                  environment.sql_dbname, environment.sql_user, pswrd)) == NULL) {
        fprintf(stdout, "PQsetdbLogin returned NULL\n");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        fprintf(stdout, "Connection was bad!\n");
        PQfinish(DBConnect);
        return NULL;
    }

    return DBConnect;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL rosinglesignalxml(int argc, IDL_VPTR argv[], char* argk) {
//
// Assign a META_ID Key for existing XML to a specific signal's record
//
// Arguments: 	#1 String containing the targeted signal's name
//		#2 Array of shot numbers (Long)
//		#3 Meta Table record Key (Long)
//
//		Pass number is optional - default is latest
//-------------------------------------------------------------------------

    int i, rc, nrows, lstr;
    int ndims, nshots, meta_id;

    int* shotlist;
    static int startup = 1;

    char sql_signal[MAXNAME], sql[MAXSQL], rsql[MAXSQL];
    char old_meta_id[MAXNAME];
    char signal_desc_id[MAXNAME];
    char signal_type[MAXNAME];
    char data_source_id[MAXNAME];

    char* signal, * source;

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DELETE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(delete)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"SOURCE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_source), IDL_KW_OFFSETOF(source)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_pass = 0;
    kw.is_source = 0;

    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.delete = 0;
    kw.commit = 0;

    signal = NULL;
    source = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

// Passed Arguments:  Signal (String)

    IDL_ENSURE_STRING(argv[0]);
    IDL_ENSURE_SCALAR(argv[0]);
    signal = IDL_STRING_STR(&(argv[0]->value.str));
    if (kw.debug)fprintf(stdout, "Signal : %s\n", signal);

    IDL_ENSURE_ARRAY(argv[1]);
    if (argv[1]->type != IDL_TYP_LONG) {
        if (kw.verbose)fprintf(stdout, "Error: The Pulse Number Array must be of Type LONG.\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((ndims = (int) argv[1]->value.arr->n_dim) != 1) {            // Number of Dimensions
        if (kw.verbose)
            fprintf(stdout, "Error: The Pulse Number Array is Multi-Dimensional. Please change Rank to 1.\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    nshots = (int) argv[1]->value.arr->n_elts;                // Number of Elements
    shotlist = (int*) argv[1]->value.arr->data;                // List

    if (kw.debug)fprintf(stdout, "No. Shots : %d\n", nshots);

    IDL_ENSURE_SCALAR(argv[2]);
    meta_id = IDL_LongScalar(argv[2]);
    if (kw.debug) fprintf(stdout, "Meta_Id : %d\n", meta_id);

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.is_source) source = IDL_STRING_STR(&kw.source);

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.is_pass) fprintf(stdout, "Pass Number: %d\n", (int) kw.pass);
        if (kw.is_source) fprintf(stdout, "Data Source: %s\n", source);
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-------------------------------------------------------------
// Check the META Table Record Exists (0 is OK => delete the entry in the Record)
//

    if (meta_id > 0) {

        sprintf(sql, "SELECT xml FROM Meta WHERE meta_id = %d;", meta_id);

        if (kw.debug) fprintf(stdout, "%s\n", sql);

        PQclear(DBQuery);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        DBQueryStatus = PQresultStatus(DBQuery);

        if (DBQueryStatus != PGRES_TUPLES_OK) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        nrows = PQntuples(DBQuery);        // Number of Rows

        if (nrows != 1) {
            if (kw.verbose)
                fprintf(stdout, "No Record is present in the META Table with the Key: %d"
                        " ... Please check your Specification\n", meta_id);
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        fprintf(stdout, "Meta ID: %d\n", meta_id);
        fprintf(stdout, "XML: %s\n", PQgetvalue(DBQuery, 0, 0));

        PQclear(DBQuery);
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Signal Name for Querying

    if (strlen(signal) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Signal name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(signal, sql_signal);
    } else {
        lstr = strlen(signal);
        if (signal[0] == '*' || signal[0] == '?' ||
            (signal[lstr - 2] != '\\' && (signal[lstr - 1] == '*' || signal[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(signal, sql_signal);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_signal, signal);
            strupr(sql_signal);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_signal);

//--------------------------------------------------------------------------
// Identify the Generic Signal from the the Signal_Desc table

    strcpy(sql, "SELECT signal_desc_id, type FROM signal_desc WHERE ");

    if (kw.match) {
        strcat(sql, "signal_name ILIKE '");
    } else {
        strcat(sql, "upper(signal_name) = '");
    }

    strcat(sql, sql_signal);
    strcat(sql, "';");

    if (kw.debug) fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (kw.debug)fprintf(stdout, "%d rows returned by the Query\n", nrows);

    if (nrows == 0) {
        if (kw.verbose)fprintf(stdout, "No Signal was Identified with a Name like %s\n", signal);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(0));    // Not an Error
    }

    if (nrows > 1) {
        if (kw.verbose)
            fprintf(stdout,
                    "Too Many Signal Description Records were Identified when only one was expected ... Please refine your Specification\n");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// SIGNAL_DESC Key

    strcpy(signal_desc_id, PQgetvalue(DBQuery, 0, 0));
    strcpy(signal_type, PQgetvalue(DBQuery, 0, 1));

    if (kw.debug) {
        fprintf(stdout, "Signal Desc ID: %s\n", signal_desc_id);
        fprintf(stdout, "Signal Type   : %s\n", signal_type);
    }

//--------------------------------------------------------------------------
// Identify the Data Source for Each Shot Number in the List
// Take the LATEST (Highest Pass No.) if no pass number was specified.

    for (i = 0; i < nshots; i++) {    // Loop over all Shots

        if (kw.is_source) {
            if (kw.is_pass) {
                sprintf(sql,
                        "SELECT source_id FROM data_Source WHERE source_alias ilike '%s' and type = '%s' and exp_number = %d and pass = %ld ",
                        source, signal_type, shotlist[i], (long)kw.pass);
            } else {
                sprintf(sql,
                        "SELECT source_id, pass FROM data_Source WHERE source_alias ilike '%s' and type = '%s' and exp_number = %d "
                                "ORDER by pass DESC", source, signal_type, shotlist[i]);
            }
        } else {
            if (kw.is_pass) {
                sprintf(sql, "SELECT source_id FROM data_Source WHERE type = '%s' and exp_number = %d and pass = %ld ",
                        signal_type, shotlist[i], (long)kw.pass);
            } else {
                sprintf(sql, "SELECT source_id, pass FROM data_Source WHERE type = '%s' and exp_number = %d "
                        "ORDER by pass DESC", signal_type, shotlist[i]);
            }
        }

        if (kw.debug) fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Query

        PQclear(DBQuery);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        DBQueryStatus = PQresultStatus(DBQuery);

        if (DBQueryStatus != PGRES_TUPLES_OK) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

// Extract the Resultset

        nrows = PQntuples(DBQuery);        // Number of Rows

        if (nrows == 0) {
            if (kw.verbose) {
                if (kw.is_source)
                    fprintf(stdout, "No Data Source (Shot %d, Alias %s) was Identified for %s\n", shotlist[i], source,
                            signal);
                else
                    fprintf(stdout, "No Data Source (Shot %d) was Identified for %s\n", shotlist[i], signal);
            }
            continue;
        }

        if (kw.is_pass && nrows > 1) {
            if (kw.verbose) {
                fprintf(stdout,
                        "Too Many Data Source Records were Identified when only one was expected ... Please check your Specification\n");
                if (kw.is_source)
                    fprintf(stdout, "Signal %s, Source %s, Shot %d, Pass %ld, Type %s\n", signal, source, shotlist[i],
                            (long)kw.pass, signal_type);
                else
                    fprintf(stdout, "Signal %s, Shot %d, Pass %ld, Type %s\n", signal, shotlist[i], (long)kw.pass,
                            signal_type);
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

// DATA_SOURCE Key

        strcpy(data_source_id, PQgetvalue(DBQuery, 0, 0));

        if (kw.debug) fprintf(stdout, "Data Source ID: %s\n", data_source_id);

//-------------------------------------------------------------
// Existing Meta_id Key value (for Rollback)

        sprintf(sql, "SELECT meta_id FROM Signal WHERE signal_desc_id = %s and source_id = %s;",
                signal_desc_id, data_source_id);

        if (kw.debug) fprintf(stdout, "%s\n", sql);

// Execute the Query

        PQclear(DBQuery);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        DBQueryStatus = PQresultStatus(DBQuery);

        if (DBQueryStatus != PGRES_TUPLES_OK) {
            if (kw.verbose) {
                fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
                fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

// Extract the Resultset

        nrows = PQntuples(DBQuery);        // Number of Rows

        if (nrows > 1) {
            if (kw.verbose) {
                fprintf(stdout, "Database Error: Too Many Signal Records were Identified when only one was expected "
                        "... Please inform the System Administrator.\n");
                if (kw.is_source)
                    fprintf(stdout, "Signal %s, Source %s, Shot %d, Pass %ld, Type %s\n", signal, source, shotlist[i],
                            (long)kw.pass, signal_type);
                else
                    fprintf(stdout, "Signal %s, Shot %d, Pass %ld, Type %s\n", signal, shotlist[i], (long)kw.pass,
                            signal_type);
            }
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        if (nrows == 0) {
            if (kw.verbose)fprintf(stdout, "No Signal Record was Identified ... Processing the Next Listed Shot.\n");
            if (kw.is_source)
                fprintf(stdout, "Signal %s, Source %s, Shot %d, Pass %ld, Type %s\n", signal, source, shotlist[i],
                        (long)kw.pass, signal_type);
            else
                fprintf(stdout, "Signal %s, Shot %d, Pass %ld, Type %s\n", signal, shotlist[i], (long)kw.pass, signal_type);
            continue;
        }

// Existing Key

        strcpy(old_meta_id, PQgetvalue(DBQuery, 0, 0));
        TrimString(old_meta_id);
        if (STR_EQUALS(old_meta_id, "")) strcpy(old_meta_id, "null");
        if (kw.debug) fprintf(stdout, "Old META ID: %s\n", old_meta_id);

//-------------------------------------------------------------
// Update the Specific Signal Record

        sprintf(sql, "BEGIN; UPDATE Signal SET meta_id = %d WHERE signal_desc_id = %s and source_id = %s; ",
                meta_id, signal_desc_id, data_source_id);
        sprintf(rsql, "'UPDATE Signal SET meta_id=%s WHERE signal_desc_id = %s and source_id = %s;' ",
                old_meta_id, signal_desc_id, data_source_id);

        strcat(sql, "INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1, Key2) VALUES('");
        strcat(sql, clientname);
        strcat(sql, "','Signal','meta_id',");
        strcat(sql, rsql);
        strcat(sql, ",'");
        strcat(sql, signal_desc_id);
        strcat(sql, "','");
        strcat(sql, data_source_id);
        strcat(sql, "'); END;");

        if (kw.debug) fprintf(stdout, "%s\n", sql);

        if (kw.commit) {
            if ((rc = checkSignalAuthorisation(1, signal_desc_id, clientname, kw.verbose, stdout)) == 1) {
                if ((rc = checkAdminAuthorisation(clientname, kw.verbose, stdout)) == 1) {
                    if (kw.verbose)
                        fprintf(stdout, "ERROR - No Change made to Single Signal Meta ID Field - No Authorisation\n");
                    IDL_KW_FREE;
                    PQclear(DBQuery);
                    return (IDL_GettmpLong(1));
                }
            }
            if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                if (kw.verbose)fprintf(stdout, "ERROR - No Change made to Single Signal Meta ID Field!\n");
                IDL_KW_FREE;
                PQclear(DBQuery);
                return (IDL_GettmpLong(1));
            }
        }

    }    // End of shot loop

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);

    return (IDL_GettmpLong(0));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//void initActions(ACTIONS *act){
//   act->nactions = 0; 		// Number of Action blocks
//   act->action   = NULL;	// Array of Action blocks
//}

IDL_VPTR IDL_CDECL rosignalxml(int argc, IDL_VPTR argv[], char* argk) {
//
//
//-------------------------------------------------------------------------

    int i, ndims = 0, rc, nrows, ncols;
    size_t lstr;
    int isNew, isXML;
    int dim[IDL_MAX_ARRAY_DIM];

    char sql_signal[MAXNAME], sql[MAXSQL], returnsql[MAXSQL], xml[MAXMETA];

    char* signal;
    char* signal_xml;

    const char xml_default[] = "<?xml version=\"1.0\"?><action><signal></signal></action>";
    size_t lxml_default = strlen(xml_default) + 12;    // Empty tags with LFs and CRs

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    static int startup = 1;

    ACTIONS actions, newactions;    // XML Data Structures

// Array Keyword Descriptors

    static IDL_KW_ARR_DESC_R expDesc = {IDL_KW_OFFSETOF(shotrange), 1, 2, IDL_KW_OFFSETOF(nshotrange)};
    static IDL_KW_ARR_DESC_R passDesc = {IDL_KW_OFFSETOF(passrange), 1, 2, IDL_KW_OFFSETOF(npassrange)};

    static IDL_KW_ARR_DESC_R doc_dim_id_Desc = {IDL_KW_OFFSETOF(doc_dim_id), 1, 8, IDL_KW_OFFSETOF(ndoc_dim_id)};
    static IDL_KW_ARR_DESC_R doc_dim_label_Desc = {IDL_KW_OFFSETOF(doc_dim_label), 1, 8,
                                                   IDL_KW_OFFSETOF(ndoc_dim_label)};
    static IDL_KW_ARR_DESC_R doc_dim_units_Desc = {IDL_KW_OFFSETOF(doc_dim_units), 1, 8,
                                                   IDL_KW_OFFSETOF(ndoc_dim_units)};

    static IDL_KW_ARR_DESC_R cal_dim_id_Desc = {IDL_KW_OFFSETOF(cal_dim_id), 1, 32, IDL_KW_OFFSETOF(ncal_dim_id)};
    static IDL_KW_ARR_DESC_R cal_dim_factor_Desc = {IDL_KW_OFFSETOF(cal_dim_factor), 1, 32,
                                                    IDL_KW_OFFSETOF(ncal_dim_factor)};
    static IDL_KW_ARR_DESC_R cal_dim_offset_Desc = {IDL_KW_OFFSETOF(cal_dim_offset), 1, 32,
                                                    IDL_KW_OFFSETOF(ncal_dim_offset)};
    static IDL_KW_ARR_DESC_R cal_dim_target_Desc = {IDL_KW_OFFSETOF(cal_dim_target), 1, 32,
                                                    IDL_KW_OFFSETOF(ncal_dim_target)};
    static IDL_KW_ARR_DESC_R cal_dim_units_Desc = {IDL_KW_OFFSETOF(cal_dim_units), 1, 32,
                                                   IDL_KW_OFFSETOF(ncal_dim_units)};

    static IDL_KW_ARR_DESC_R err_params_Desc = {IDL_KW_OFFSETOF(err_params), 1, MAXERRPARAMS,
                                                IDL_KW_OFFSETOF(nerr_params)};
    static IDL_KW_ARR_DESC_R err_dim_id_Desc = {IDL_KW_OFFSETOF(err_dim_id), 1, 8, IDL_KW_OFFSETOF(nerr_dim_id)};
    static IDL_KW_ARR_DESC_R err_dim_model_Desc = {IDL_KW_OFFSETOF(err_dim_model), 1, 8,
                                                   IDL_KW_OFFSETOF(nerr_dim_model)};

    static IDL_KW_ARR_DESC_R comp_dim_id_Desc = {IDL_KW_OFFSETOF(comp_dim_id), 1, 8, IDL_KW_OFFSETOF(ncomp_dim_id)};
    static IDL_KW_ARR_DESC_R comp_dim_signal_Desc = {IDL_KW_OFFSETOF(comp_dim_signal), 1, 8,
                                                     IDL_KW_OFFSETOF(ncomp_dim_signal)};
    static IDL_KW_ARR_DESC_R comp_dim_error_Desc = {IDL_KW_OFFSETOF(comp_dim_error), 1, 8,
                                                    IDL_KW_OFFSETOF(ncomp_dim_error)};
    static IDL_KW_ARR_DESC_R comp_dim_aserror_Desc = {IDL_KW_OFFSETOF(comp_dim_aserror), 1, 8,
                                                      IDL_KW_OFFSETOF(ncomp_dim_aserror)};
    static IDL_KW_ARR_DESC_R comp_dim_from_Desc = {IDL_KW_OFFSETOF(comp_dim_from), 1, 8,
                                                   IDL_KW_OFFSETOF(ncomp_dim_from)};
    static IDL_KW_ARR_DESC_R comp_dim_to_Desc = {IDL_KW_OFFSETOF(comp_dim_to), 1, 8, IDL_KW_OFFSETOF(ncomp_dim_to)};

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"ALIAS         ", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(alias)},
             {"CAL_DIM_FACTOR", IDL_TYP_DOUBLE, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_cal_dim_factor),
              IDL_CHARA(cal_dim_factor_Desc)},
             {"CAL_DIM_ID", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_cal_dim_id), IDL_CHARA(cal_dim_id_Desc)},
             {"CAL_DIM_OFFSET", IDL_TYP_DOUBLE, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_cal_dim_offset),
              IDL_CHARA(cal_dim_offset_Desc)},
             {"CAL_DIM_TARGET", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_cal_dim_target),
              IDL_CHARA(cal_dim_target_Desc)},
             {"CAL_DIM_UNITS", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_cal_dim_units),
              IDL_CHARA(cal_dim_units_Desc)},
             {"CAL_FACTOR", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_cal_factor),
              IDL_KW_OFFSETOF(cal_factor)},
             {"CAL_OFFSET", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_cal_offset),
              IDL_KW_OFFSETOF(cal_offset)},
             {"CAL_TARGET", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_cal_target),
              IDL_KW_OFFSETOF(cal_target)},
             {"CAL_UNITS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_cal_units), IDL_KW_OFFSETOF(cal_units)},
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"COMP_ASERROR", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comp_aserror),
              IDL_KW_OFFSETOF(comp_aserror)},
             {"COMP_DIM_ASERROR", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_aserror),
              IDL_CHARA(comp_dim_aserror_Desc)},
             {"COMP_DIM_ERROR", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_error),
              IDL_CHARA(comp_dim_error_Desc)},
             {"COMP_DIM_FROM", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_from),
              IDL_CHARA(comp_dim_from_Desc)},
             {"COMP_DIM_ID", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_id),
              IDL_CHARA(comp_dim_id_Desc)},
             {"COMP_DIM_SIGNAL", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_signal),
              IDL_CHARA(comp_dim_signal_Desc)},
             {"COMP_DIM_TO", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_comp_dim_to),
              IDL_CHARA(comp_dim_to_Desc)},
             {"COMP_ERROR", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comp_error),
              IDL_KW_OFFSETOF(comp_error)},
             {"COMP_NEW_ALIAS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comp_new_alias),
              IDL_KW_OFFSETOF(comp_new_alias)},
             {"COMP_SIGNAL", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_comp_signal),
              IDL_KW_OFFSETOF(comp_signal)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DELETE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(delete)},
             {"DOC_DESC", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_doc_desc), IDL_KW_OFFSETOF(doc_desc)},
             {"DOC_DIM_ID", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_doc_dim_id), IDL_CHARA(doc_dim_id_Desc)},
             {"DOC_DIM_LABEL", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_doc_dim_label),
              IDL_CHARA(doc_dim_label_Desc)},
             {"DOC_DIM_UNITS", IDL_TYP_STRING, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_doc_dim_units),
              IDL_CHARA(doc_dim_units_Desc)},
             {"DOC_LABEL", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_doc_label), IDL_KW_OFFSETOF(doc_label)},
             {"DOC_UNITS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_doc_units), IDL_KW_OFFSETOF(doc_units)},
             {"ERR_DIM_ID", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_err_dim_id), IDL_CHARA(err_dim_id_Desc)},
             {"ERR_DIM_MODEL", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_err_dim_model),
              IDL_CHARA(err_dim_model_Desc)},
             {"ERR_DIM_PARAMS", IDL_TYP_FLOAT, 1, IDL_KW_VIN, IDL_KW_OFFSETOF(is_err_dim_params),
              IDL_KW_OFFSETOF(err_dim_params)},
             {"ERR_MODEL", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_err_model), IDL_KW_OFFSETOF(err_model)},
             {"ERR_PARAMS", IDL_TYP_FLOAT, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_err_params), IDL_CHARA(err_params_Desc)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"META_ID", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_meta_id), IDL_KW_OFFSETOF(meta_id)},
             {"MULTIPLE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(multiple)},
             {"PRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_passrange), IDL_CHARA(passDesc)},
             {"QUERY", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_query), IDL_KW_OFFSETOF(query)},
             {"REPLACE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(replace)},
             {"SRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_shotrange), IDL_CHARA(expDesc)},
             {"TIME_INTERVAL", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_time_interval),
              IDL_KW_OFFSETOF(time_interval)},
             {"TIME_OFFSET", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_time_offset),
              IDL_KW_OFFSETOF(time_offset)},
             {"TIME_START", IDL_TYP_DOUBLE, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_time_start),
              IDL_KW_OFFSETOF(time_start)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {"XML", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_xml), IDL_KW_OFFSETOF(xml)},
             {NULL}};

    KW_RESULT kw;

    kw.is_xml = 0;
    kw.is_query = 0;
    kw.is_status = 0;
    kw.is_time_offset = 0;
    kw.is_time_start = 0;
    kw.is_time_interval = 0;
    kw.is_passrange = 0;
    kw.is_shotrange = 0;
    kw.is_meta_id = 0;

    kw.is_doc_label = 0;
    kw.is_doc_units = 0;
    kw.is_doc_desc = 0;
    kw.is_doc_dim_id = 0;
    kw.is_doc_dim_label = 0;
    kw.is_doc_dim_units = 0;

    kw.is_cal_factor = 0;
    kw.is_cal_offset = 0;
    kw.is_cal_target = 0;
    kw.is_cal_units = 0;
    kw.is_cal_dim_id = 0;
    kw.is_cal_dim_factor = 0;
    kw.is_cal_dim_offset = 0;
    kw.is_cal_dim_target = 0;
    kw.is_cal_dim_units = 0;

    kw.is_comp_new_alias = 0;
    kw.is_comp_signal = 0;
    kw.is_comp_error = 0;
    kw.is_comp_aserror = 0;
    kw.is_comp_dim_id = 0;
    kw.is_comp_dim_signal = 0;
    kw.is_comp_dim_error = 0;
    kw.is_comp_dim_aserror = 0;
    kw.is_comp_dim_to = 0;
    kw.is_comp_dim_from = 0;

    kw.npassrange = 0;
    kw.passrange[0] = -1;
    kw.passrange[1] = -1;

    kw.nshotrange = 0;
    kw.shotrange[0] = -1;
    kw.shotrange[1] = -1;

    kw.ndoc_dim_id = 0;
    kw.doc_dim_id[0] = -1;

    kw.alias = 0;
    kw.match = 0;
    kw.multiple = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.delete = 0;
    kw.replace = 0;
    kw.commit = 0;

    kw.meta_id = 0;

    kw.is_err_model = 0;
    kw.is_err_params = 0;
    kw.is_err_dim_id = 0;
    kw.is_err_dim_model = 0;
    kw.is_err_dim_params = 0;

    kw.time_offset = 0.0;
    kw.time_start = 0.0;
    kw.time_interval = 0.0;
    kw.cal_factor = 1.0;
    kw.cal_offset = 0.0;

    signal = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

// Passed Arguments: String

    if (argv[0]->type == IDL_TYP_STRING) {
        IDL_ENSURE_STRING(argv[0]);
        IDL_ENSURE_SCALAR(argv[0]);
        signal = IDL_STRING_STR(&(argv[0]->value.str));                // Signal
        if (kw.debug)fprintf(stdout, "Signal : %s\n", signal);
    }

//-----------------------------------------------------------------------
// Process Keyword Arrays

    if (kw.is_err_dim_params) {
        ndims = 0;
        if (kw.err_dim_params->flags & IDL_V_ARR) {
            ndims = (int) kw.err_dim_params->value.arr->n_dim;
            if (kw.err_dim_params->type == IDL_TYP_FLOAT) {
                if (kw.debug)fprintf(stdout, "Array is of Type: FLOAT\n");

            } else {
                if (kw.err_dim_params->type == IDL_TYP_DOUBLE) {
                    if (kw.debug)fprintf(stdout, "Array is of Type: DOUBLE\n");
                }
            }
        }
    }

//--------------------------------------------------------------------------
// Test Arrays are Complete:

// Documentation: Labels and Units can have less items but the sequence is preserved => User must pad with null strings

    if (kw.is_doc_dim_id && ((kw.is_doc_dim_label && !(kw.ndoc_dim_label <= kw.ndoc_dim_id))
                             || (kw.is_doc_dim_units && !(kw.ndoc_dim_units <= kw.ndoc_dim_id)))) {
        if (kw.verbose) fprintf(stdout, "Dimensional Documentation Arrays have Inconsistent Lengths!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Time Offset: No Test Required

// Calibration:

    if (kw.is_cal_dim_id && ((kw.is_cal_dim_factor && !(kw.ncal_dim_factor <= kw.ncal_dim_id))
                             || (kw.is_cal_dim_offset && !(kw.ncal_dim_offset <= kw.ncal_dim_id))
                             || (kw.is_cal_dim_target && !(kw.ncal_dim_target <= kw.ncal_dim_id))
                             || (kw.is_cal_dim_units && !(kw.ncal_dim_units <= kw.ncal_dim_id)))) {
        if (kw.verbose) fprintf(stdout, "Dimensional Calibration Arrays have Inconsistent Lengths!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Error Model:

    if (kw.is_err_dim_id && (!kw.is_err_dim_params || !kw.is_err_dim_model || !(kw.nerr_dim_model <= kw.nerr_dim_id))) {
        if (kw.verbose) fprintf(stdout, "Dimensional Error Model Arrays are Missing or have Inconsistent Lengths!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.is_err_dim_id) {
        if (!(kw.err_dim_params->flags & IDL_V_ARR)) {
            if (kw.verbose) fprintf(stdout, "Dimensional Error Model Parameter Argument is Not an Array!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        ndims = (int) kw.err_dim_params->value.arr->n_dim;            // Number of Dimensions
        if (!(ndims == 2 && dim[1] <= kw.nerr_dim_id)) {
            if (kw.verbose)
                fprintf(stdout, "Dimensional Error Model Parameter Array is Not 2D or have Inconsistent Lengths!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Composite:

    if (kw.is_comp_dim_id && ((kw.is_comp_dim_signal && !(kw.ncomp_dim_signal <= kw.ncomp_dim_id))
                              || (kw.is_comp_dim_error && !(kw.ncomp_dim_error <= kw.ncomp_dim_id))
                              || (kw.is_comp_dim_aserror && !(kw.ncomp_dim_aserror <= kw.ncomp_dim_id))
                              || (kw.is_comp_dim_to && !(kw.ncomp_dim_to <= kw.ncomp_dim_id))
                              || (kw.is_comp_dim_from && !(kw.ncomp_dim_from <= kw.ncomp_dim_id)))) {
        if (kw.verbose) fprintf(stdout, "Dimensional Composite Signal Arrays have Inconsistent Lengths!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// True Pass Range?

    if (kw.is_passrange) {
        if (kw.npassrange != 2) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.passrange[1] < kw.passrange[0] && kw.passrange[1] >= -1) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// True Shot Range?

    if (kw.is_shotrange) {
        if (kw.nshotrange != 2) {
            if (kw.verbose) fprintf(stdout, "Exp. Number Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.shotrange[1] < kw.shotrange[0] && kw.shotrange[1] > 0) {
            if (kw.verbose) fprintf(stdout, "Exp. Number Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Query Values

    if (kw.is_query && strlen(IDL_STRING_STR(&kw.query)) == 0) {
        if (kw.verbose) fprintf(stdout, "Query Value is Invalid. Please Check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Signal Name for Querying

    if (strlen(signal) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Signal name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(signal, sql_signal);
    } else {
        lstr = strlen(signal);
        if (signal[0] == '*' || signal[0] == '?' ||
            (signal[lstr - 2] != '\\' && (signal[lstr - 1] == '*' || signal[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(signal, sql_signal);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_signal, signal);
            strupr(sql_signal);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_signal);

//--------------------------------------------------------------------------
// Prepare the Query: Extract existing XML from the Signal_Desc table

    strcpy(sql, "SELECT signal_desc_id, meta_id, source_alias, signal_desc_xml, signal_type, "
            "signal_name, signal_alias, generic_name FROM signal_desc_meta WHERE ");

    if (kw.match) {
        if (kw.alias)
            strcat(sql, "signal_alias ILIKE '");
        else
            strcat(sql, "signal_name ILIKE '");
    } else {
        if (kw.alias)
            strcat(sql, "upper(signal_alias) = '");
        else
            strcat(sql, "upper(signal_name) = '");
    }

    strcat(sql, sql_signal);

// Add any additional useer supplied query

    if (kw.is_query) {
        strcat(sql, "' AND ");
        strcat(sql, IDL_STRING_STR(&kw.query));
    }

    strcat(sql, "'");

    strcpy(returnsql, sql);        // Preserve the SQL to return Updated record details to the User

    if (kw.debug) fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (kw.debug)fprintf(stdout, "%d rows returned by the Query\n", nrows);

    if (nrows == 0 && !kw.is_comp_new_alias) {
        if (kw.verbose)fprintf(stdout, "No Signals were Identified with a Name like %s\n", signal);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(0));    // Not an Error
    }

    if (kw.is_comp_new_alias && nrows > 0) {
        if (kw.verbose)
            fprintf(stdout, "The New Composite Signal Specified Already Exists ... Please change your Specification\n");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (nrows > 1 && !kw.multiple) {
        if (kw.verbose)
            fprintf(stdout,
                    "Too Many Signal Description Records were Identified when only one was expected ... Please refine your Specification\n");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (ncols == 0 && !kw.is_comp_new_alias) {
        if (kw.verbose)fprintf(stdout, "ERROR - No Columns were Returned by the Query %s\n", sql);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Are All Records Composite Records?

    if (!kw.is_comp_new_alias && (kw.is_comp_signal || kw.is_comp_error || kw.is_comp_aserror)) {
        for (i = 0; i < nrows; i++) {
            if (strcasecmp(PQgetvalue(DBQuery, i, 4), "C") != 0) {
                if (kw.verbose)
                    fprintf(stdout,
                            "ERROR - Not all Records Identified are Composite Signal Records ... Please refine your Specification\n");
                IDL_KW_FREE;
                PQclear(DBQuery);
                return (IDL_GettmpLong(1));
            }
        }
    }

//-------------------------------------------------------------
// Parse Existing XML, Create Data Structures, Combine and create new XML
//
// Replace original XML
// Add a new XML Document
// Add a new Signal Desc Record

    if (kw.is_comp_new_alias) {        // Only 1 New record to be created

        if ((rc = createIdamKWXML(signal, kw, xml)) == 1) {        // Create XML from the Keywords and Passed parameters
            if (kw.verbose)fprintf(stdout, "ERROR - Problem Creating XML from Passed Parameters!\n");
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }

        if (kw.debug)fprintf(stdout, "Composite XML: %s\n", xml);

        if (strlen(xml) > 0) {
            strcpy(sql, "BEGIN; INSERT INTO Meta (meta_id,xml) VALUES (nextval('meta_id_seq'),'");
            strcat(sql, xml);
            strcat(sql,
                   "'); INSERT INTO Signal_Desc (meta_id,signal_alias,signal_name,generic_name,type,source_alias,signal_owner) VALUES(");
            strcat(sql, "currval('meta_id_seq'),upper('");
        } else {
            strcpy(sql,
                   "BEGIN; INSERT INTO Signal_Desc (signal_alias,signal_name,generic_name,type,source_alias,signal_owner) VALUES(upper('");
        }
        strcat(sql, signal);
        strcat(sql, "'),upper('");
        strcat(sql, signal);
        strcat(sql, "'),upper('");
        strcat(sql, signal);
        strcat(sql, "'),'C','");
        strcat(sql, IDL_STRING_STR(&kw.comp_new_alias));
        strcat(sql, "',upper('");
        strcat(sql, clientname);
        strcat(sql, "')); INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1) VALUES('");
        strcat(sql, clientname);
        strcat(sql, "','Signal_Desc','*','DELETE FROM Signal_Desc WHERE signal_alias = upper(\\'");
        strcat(sql, signal);
        strcat(sql, "\\') AND signal_name = upper(\\'");
        strcat(sql, signal);
        strcat(sql, "\\') AND generic_name = upper(\\'");
        strcat(sql, signal);
        strcat(sql, "\\') AND type=\\'C\\'',currval('signal_desc_id_seq')); END;");

        if (kw.debug) fprintf(stdout, "%s\n", sql);

        if (kw.commit) {
            if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                if (kw.verbose)fprintf(stdout, "ERROR - New Composite Signal NOT Created!\n");
                IDL_KW_FREE;
                PQclear(DBQuery);
                return (IDL_GettmpLong(1));
            }
        }

    } else {

//==================================================================================
//   isNew   isXML   delete   Outcome
//     0       0       0      do nothing
//     0       0       1      Delete previous XML
//     1       0       0      No Previous and No keyword passed XML so do Nothing
//     1       0       1      No Previous and No keyword passed XML so do Nothing
//     0       1       0      Combine Original with the keyword passed XML
//     0       1       1      Replace the original XML with the keyword passed XML
//     1       1       0      No Previous so write New record
//     1       1       1      No Previous so write New record (Ignore delete request)
//==================================================================================

        for (i = 0; i < nrows; i++) {

// Create Data Structures from new XML

            signal_xml = PQgetvalue(DBQuery, i, 3);            // XML From the current Signal_Desc record

            isNew = 0;
            if (strlen(signal_xml) == 0) isNew = 1;            // No Previous XML so any XML must be New

            if ((rc = createIdamKWXML(signal, kw, xml)) ==
                1) {    // Create XML from the Keywords and Passed parameters
                if (kw.verbose)fprintf(stdout, "ERROR - Problem Creating XML From Passed Parameters!\n");
                IDL_KW_FREE;
                PQclear(DBQuery);
                return (IDL_GettmpLong(1));
            }

            isXML = 0;
            if (strlen(xml) > lxml_default) isXML = 1;        // New XML passed by Keywords

            if (kw.debug) {
                fprintf(stdout, "Previous XML? %d\n", !isNew);
                fprintf(stdout, "KW XML?       %d\n", isXML);
                fprintf(stdout, "Delete?       %d\n", (int) kw.delete);
                fprintf(stdout, "original xml[%d] = %s\n", i, signal_xml);
                fprintf(stdout, "new xml[%d]      = %s\n", i, xml);
                fprintf(stdout, "length xml[%d]   = %zu (%zu)\n", i, strlen(xml), lxml_default);
            }

            if (isNew && !isXML) continue;                // No XML to Change: No Previous and No New
            if (!isNew && !isXML && !kw.delete) continue;        // No New XML and No Delete of Original XML Requested

            if (kw.debug) fprintf(stdout, "Combining New and Original XML\n");

            if (!isNew && !kw.delete && isXML) {        // Ignore Previous XML if Delete is Requested
                initActions(&actions);
                initActions(&newactions);
                parseDoc(signal_xml,
                         &actions);        // Create an Array of action blocks from the current Signal_desc record
                parseDoc(xml, &newactions);            // Create an Array of action blocks from the new XML

// Combine New and Old action blocks

                if ((rc = combineIdamActions(kw.replace, newactions, &actions)) == 1) {    // Combine Action Blocks
                    if (kw.verbose)fprintf(stdout, "ERROR - Problem Combining XML Action Structures!\n");
                    IDL_KW_FREE;
                    PQclear(DBQuery);
                    return (IDL_GettmpLong(1));
                }

                if ((rc = createIdamActionXML(actions, xml)) == 1) {        // Create XML from combined action blocks
                    if (kw.verbose)fprintf(stdout, "ERROR - Problem Creating XML from Action Structures!\n");
                    IDL_KW_FREE;
                    PQclear(DBQuery);
                    return (IDL_GettmpLong(1));
                }

                if (kw.debug) {
                    fprintf(stdout, "Original xml[%d] = %s\n", i, signal_xml);
                    printActions(actions);
                    printActions(newactions);

                    printActions(actions);
                    fprintf(stdout, "New xml[%d] = %s\n", i, xml);
                }

                freeActions(&actions);
                freeActions(&newactions);
            }

// Update or Write a New XML Document

            sql[0] = '\0';

            if (isNew) {
                if (kw.debug) fprintf(stdout, "Saving New XML to Database\n");
                strcpy(sql, "BEGIN; INSERT INTO Meta (meta_id,xml) VALUES (nextval('meta_id_seq'),'");
                strcat(sql, xml);
                strcat(sql, "'); UPDATE Signal_Desc SET meta_id = currval('meta_id_seq') WHERE signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "; UPDATE Signal_Desc SET modified = current_date WHERE signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "; INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1) VALUES('");
                strcat(sql, clientname);
                strcat(sql, "','Meta','*','DELETE FROM Meta WHERE meta_id = key1',currval('meta_id_seq'))");
                strcat(sql, "; INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1) VALUES('");
                strcat(sql, clientname);
                strcat(sql, "','Signal_Desc','*','UPDATE Signal_Desc SET meta_id = 0 WHERE signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "',");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "); END;");
            } else {
                //if(isXML && !kw.delete){
                if (isXML) {
                    if (kw.debug) fprintf(stdout, "Updating New and Original XML to Database\n");
                    strcpy(sql, "BEGIN; UPDATE Meta SET xml = '");
                    strcat(sql, xml);
                    strcat(sql, "' WHERE meta_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "; UPDATE Meta SET modified = current_date WHERE meta_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "; INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1) VALUES('");
                    strcat(sql, clientname);
                    strcat(sql, "','Meta','xml','UPDATE Meta SET xml= \\'");
                    strcat(sql, PQgetvalue(DBQuery, i, 3));
                    strcat(sql, "\\' WHERE meta_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "',");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "); END;");
                } else {
                    //if(!isXML && kw.delete){
                    if (kw.delete) {                // Remove Original XML
                        if (kw.debug) fprintf(stdout, "Deleting Original XML from Database\n");
                        strcpy(sql, "BEGIN; UPDATE Signal_Desc SET meta_id = 0 WHERE signal_desc_id = ");
                        strcat(sql, PQgetvalue(DBQuery, i, 0));
                        strcpy(sql, "; UPDATE Signal_Desc SET modified = current_date WHERE signal_desc_id = ");
                        strcat(sql, PQgetvalue(DBQuery, i, 0));
                        strcat(sql,
                               "; INSERT INTO Change_Log (username, dbtable, dbfield, rollback_sql, key1) VALUES('");
                        strcat(sql, clientname);
                        strcat(sql, "','Signal_Desc','meta_id','UPDATE Signal_Desc SET meta_id = ");
                        strcat(sql, PQgetvalue(DBQuery, i, 1));
                        strcat(sql, " WHERE signal_desc_id = ");
                        strcat(sql, PQgetvalue(DBQuery, i, 0));
                        strcat(sql, "',");
                        strcat(sql, PQgetvalue(DBQuery, i, 0));
                        strcat(sql, "); END;");
                    } else if (kw.debug) fprintf(stdout, "Do Nothing ...\n");
                }
            }


            if (kw.debug) fprintf(stdout, "%s\n", sql);

            if (kw.commit && strlen(sql) > 0) {
                if ((rc = checkSignalAuthorisation(1, PQgetvalue(DBQuery, i, 0), clientname, kw.verbose, stdout)) ==
                    1) {
                    if ((rc = checkAdminAuthorisation(clientname, kw.verbose, stdout)) == 1) {
                        if (kw.verbose)fprintf(stdout, "ERROR - New Composite Signal NOT Created - No Authorisation\n");
                        IDL_KW_FREE;
                        PQclear(DBQuery);
                        return (IDL_GettmpLong(1));
                    }
                }
                if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                    if (kw.verbose)fprintf(stdout, "ERROR - New Composite Signal NOT Created!\n");
                    IDL_KW_FREE;
                    PQclear(DBQuery);
                    return (IDL_GettmpLong(1));
                }
            }

        }    // End of i loop
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);

    if (kw.commit || nrows > 0)
        return (returnIdamSQL(returnsql, kw.verbose, kw.debug, stdout));
    else
        return (IDL_GettmpLong(0));
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Create the IDL Return Structure

typedef struct {
    int signal_desc_id;
    int meta_id;
    IDL_STRING signal_name;
    IDL_STRING signal_alias;
    IDL_STRING generic_name;
    IDL_STRING source_alias;
    IDL_STRING signal_type;
    IDL_STRING xml;
} RSQL;

IDL_VPTR returnIdamSQL(char* sql, int verbose, int debug, FILE* fh) {
//
//
// v0.01	November 2005	D.G.Muir	Original Release
//-------------------------------------------------------------------------

    int i, nrows, ncols;

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    //IDL_STRING *pisArray;

    RSQL* rsql;            // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"SIGNAL_DESC_ID", 0,     (void*) IDL_TYP_LONG},
            {"META_ID",        0,     (void*) IDL_TYP_LONG},
            {"SIGNAL_NAME",    vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_ALIAS",   vdlen, (void*) IDL_TYP_STRING},
            {"GENERIC_NAME",   vdlen, (void*) IDL_TYP_STRING},
            {"SOURCE_ALIAS",   vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_TYPE",    vdlen, (void*) IDL_TYP_STRING},
            {"XML",            vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

//-------------------------------------------------------------
// Execute the Passed Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }
/*
   if(debug){
      for(i=0;i<nrows;i++){
         fprintf(stdout,"%d %s %s %s %s %s %s %s %s\n",i,PQgetvalue(DBQuery,i,0),PQgetvalue(DBQuery,i,1),
                 PQgetvalue(DBQuery,i,2),PQgetvalue(DBQuery,i,3),PQgetvalue(DBQuery,i,4),PQgetvalue(DBQuery,i,5),
		 PQgetvalue(DBQuery,i,6),PQgetvalue(DBQuery,i,7));
      }
   }
   PQclear(DBQuery);
   return(IDL_GettmpLong(1));
*/

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RSQL*) malloc(nrows * sizeof(RSQL))) == NULL) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].signal_desc_id = (int) atoi(PQgetvalue(DBQuery, i, 0));
        rsql[i].meta_id = (int) atoi(PQgetvalue(DBQuery, i, 1));

        if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
            IDL_StrStore(&(rsql[i].signal_name), PQgetvalue(DBQuery, i, 5));
        else
            IDL_StrStore(&(rsql[i].signal_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
            IDL_StrStore(&(rsql[i].signal_alias), PQgetvalue(DBQuery, i, 6));
        else
            IDL_StrStore(&(rsql[i].signal_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 7)) > 0)
            IDL_StrStore(&(rsql[i].generic_name), PQgetvalue(DBQuery, i, 7));
        else
            IDL_StrStore(&(rsql[i].generic_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 2)) > 0)
            IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 2));
        else
            IDL_StrStore(&(rsql[i].source_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
            IDL_StrStore(&(rsql[i].signal_type), PQgetvalue(DBQuery, i, 4));
        else
            IDL_StrStore(&(rsql[i].signal_type), "");

        if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
            IDL_StrStore(&(rsql[i].xml), PQgetvalue(DBQuery, i, 3));
        else
            IDL_StrStore(&(rsql[i].xml), "");
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    PQclear(DBQuery);
    return (ivReturn);
}


//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
// Create XML from the IDL Keyword Structure

int createIdamKWXML(char* signal, KW_RESULT kw, char* xml) {
    int i, j, ndims = 0, offset = 0;
    int dim[IDL_MAX_ARRAY_DIM];
    float* ferr_dim_params = NULL;
    double* derr_dim_params = NULL;

    const char xml_tst[] = "<?xml version=\"1.0\"?><action><signal></signal></action>";
    char xml_str[256] = "";
    char lfcr[3] = "  ";

//------------------------------------------------------------------------------------------
// Check if a Complete XML document has been passed by keyword

    if (kw.is_xml) {
        strcpy(xml, IDL_STRING_STR(&kw.xml));
        return 0;
    }

//------------------------------------------------------------------------------------------
// Build XML from Keywords


    lfcr[0] = 10;
    lfcr[1] = 13;

    strcpy(xml, "<?xml version=\"1.0\"?>");    // XML Header
    strcat(xml, lfcr);
    strcat(xml, "<action>");
    strcat(xml, lfcr);
    strcat(xml, "  <signal>");
    strcat(xml, lfcr);

//------------------------------------------------------------------------------------------
// Documentation

    if (kw.is_doc_label || kw.is_doc_units || kw.is_doc_desc || kw.is_doc_dim_id) {
        sprintf(xml_str,
                "    <documentation id=\"1\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                kw.shotrange[0], kw.shotrange[1], kw.passrange[0], kw.passrange[1]);
        strcat(xml, xml_str);
        strcat(xml, lfcr);
        if (kw.is_doc_desc && strlen(IDL_STRING_STR(&kw.doc_desc)) > 0) {
            sprintf(xml_str, "      <description>%s</description>", IDL_STRING_STR(&kw.doc_desc));
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }
        if (kw.is_doc_label && strlen(IDL_STRING_STR(&kw.doc_label)) > 0) {
            sprintf(xml_str, "      <label>%s</label>", IDL_STRING_STR(&kw.doc_label));
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }
        if (kw.is_doc_units && strlen(IDL_STRING_STR(&kw.doc_units)) > 0) {
            sprintf(xml_str, "      <units>%s</units>", IDL_STRING_STR(&kw.doc_units));
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }

        if (kw.is_doc_dim_id && (kw.is_doc_dim_label || kw.is_doc_dim_units)) {
            for (i = 0; i < kw.ndoc_dim_id; i++) {
                sprintf(xml_str, "      <dimension dimid=\"%d\">", kw.doc_dim_id[i]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                if (kw.is_doc_dim_label && i < kw.ndoc_dim_label) {
                    if (strlen(IDL_STRING_STR(&kw.doc_dim_label[i])) > 0) {
                        sprintf(xml_str, "        <label>%s</label>", IDL_STRING_STR(&kw.doc_dim_label[i]));
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                }
                if (kw.is_doc_dim_units && i < kw.ndoc_dim_units) {
                    if (strlen(IDL_STRING_STR(&kw.doc_dim_units[i])) > 0) {
                        sprintf(xml_str, "        <units>%s</units>", IDL_STRING_STR(&kw.doc_dim_units[i]));
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                }
                strcat(xml, "      </dimension>");
                strcat(xml, lfcr);
            }
        }
        strcat(xml, "    </documentation>");
        strcat(xml, lfcr);
    }

//------------------------------------------------------------------------------------------
// Time Offset

    if (kw.is_time_offset || kw.is_time_start || kw.is_time_interval) {
        if (kw.is_time_offset) {
            sprintf(xml_str, "    <time_offset id=\"1\" method=\"0\" value=\"%f\" "
                            "exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\" />",
                    kw.time_offset, kw.shotrange[0], kw.shotrange[1], kw.passrange[0], kw.passrange[1]);
        } else {
            sprintf(xml_str, "    <time_offset id=\"1\" method=\"1\" start=\"%f\" interval=\"%f\" "
                            "exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\" />",
                    kw.time_start, kw.time_interval, kw.shotrange[0], kw.shotrange[1], kw.passrange[0],
                    kw.passrange[1]);

        }
        strcat(xml, xml_str);
        strcat(xml, lfcr);
    }

//------------------------------------------------------------------------------------------
// Calibration

    if (kw.is_cal_factor || kw.is_cal_offset || kw.is_cal_units || kw.is_cal_dim_id) {
        if (kw.is_cal_target) {
            sprintf(xml_str,
                    "    <calibration id=\"1\" target=\"%s\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                    IDL_STRING_STR(&kw.cal_target), kw.shotrange[0], kw.shotrange[1], kw.passrange[0], kw.passrange[1]);
        } else {
            sprintf(xml_str,
                    "    <calibration id=\"1\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                    kw.shotrange[0], kw.shotrange[1], kw.passrange[0], kw.passrange[1]);
        }
        strcat(xml, xml_str);
        strcat(xml, lfcr);
        if (kw.is_cal_factor && kw.cal_factor != (double) 1.0) {
            sprintf(xml_str, "      <factor>%f</factor>", kw.cal_factor);
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }
        if (kw.is_cal_offset && kw.cal_offset != (double) 0.0) {
            sprintf(xml_str, "      <offset>%f</offset>", kw.cal_offset);
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }
        if (kw.is_cal_units && strlen(IDL_STRING_STR(&kw.cal_units)) > 0) {
            sprintf(xml_str, "      <units>%s</units>", IDL_STRING_STR(&kw.cal_units));
            strcat(xml, xml_str);
            strcat(xml, lfcr);
        }

        if (kw.is_cal_dim_id && (kw.is_cal_dim_factor || kw.is_cal_dim_offset || kw.is_cal_dim_units)) {
            for (i = 0; i < kw.ncal_dim_id; i++) {
                if (kw.is_cal_dim_target && i < kw.ncal_dim_target) {
                    if (strlen(IDL_STRING_STR(&kw.cal_dim_target[i])) > 0)
                        sprintf(xml_str, "      <dimension dimid=\"%d\" target=\"%s\">", kw.cal_dim_id[i],
                                IDL_STRING_STR(&kw.cal_dim_target[i]));
                    else
                        sprintf(xml_str, "      <dimension dimid=\"%d\">", kw.cal_dim_id[i]);
                } else
                    sprintf(xml_str, "      <dimension dimid=\"%d\">", kw.cal_dim_id[i]);
                strcat(xml, xml_str);
                strcat(xml, lfcr);
                if (kw.is_cal_dim_factor && i < kw.ncal_dim_factor) {
                    if (kw.cal_dim_factor[i] != (double) 1.0) {
                        sprintf(xml_str, "        <factor>%f</factor>", kw.cal_dim_factor[i]);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                }
                if (kw.is_cal_dim_offset && i < kw.ncal_dim_offset) {
                    if (kw.cal_dim_offset[i] != (double) 0.0) {
                        sprintf(xml_str, "        <offset>%f</offset>", kw.cal_dim_offset[i]);
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                }
                if (kw.is_cal_dim_units && i < kw.ncal_dim_units) {
                    if (strlen(IDL_STRING_STR(&kw.cal_dim_units[i])) > 0) {
                        sprintf(xml_str, "        <units>%s</units>", IDL_STRING_STR(&kw.cal_dim_units[i]));
                        strcat(xml, xml_str);
                        strcat(xml, lfcr);
                    }
                }
                strcat(xml, "      </dimension>");
                strcat(xml, lfcr);
            }
        }
        strcat(xml, "    </calibration>");
        strcat(xml, lfcr);
    }

//------------------------------------------------------------------------------------------
// Error Model

    if (kw.is_err_model || kw.is_err_dim_id) {
        if (kw.is_err_model) {
            sprintf(xml_str,
                    "    <errormodel id=\"1\" model=\"%d\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                    (int) kw.err_model, kw.shotrange[0], kw.shotrange[1], (int) kw.passrange[0], (int) kw.passrange[1]);
        } else {
            sprintf(xml_str,
                    "    <errormodel id=\"1\" exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                    kw.shotrange[0], kw.shotrange[1], (int) kw.passrange[0], (int) kw.passrange[1]);
        }
        strcat(xml, xml_str);
        strcat(xml, lfcr);
        if (kw.is_err_params) {
            strcat(xml, "      <params>");
            if (kw.nerr_params == 1) {
                sprintf(xml_str, "%f", kw.err_params[0]);
                strcat(xml, xml_str);
            } else {
                for (i = 0; i < kw.nerr_params; i++) {
                    if (i == kw.nerr_params - 1)
                        sprintf(xml_str, "%f", kw.err_params[i]);
                    else
                        sprintf(xml_str, "%f,", kw.err_params[i]);
                    strcat(xml, xml_str);
                }
            }
            strcat(xml, "</params>");
            strcat(xml, lfcr);
        }

// Dimensional Error Model

        if (kw.is_err_dim_id && kw.is_err_dim_model && kw.is_err_dim_params) {
            if (kw.err_dim_params->flags & IDL_V_ARR) {
                ndims = (int) kw.err_dim_params->value.arr->n_dim;                // Number of Dimensions
                for (i = 0; i < ndims; i++) dim[i] = (int) kw.err_dim_params->value.arr->dim[i];    // Dimension Lengths
            }

//fprintf(stdout,"%d %d %d\n", ndims, dim[0], dim[1]);
// dim[0] == Length of each parameter array
// dim[1] == Number of Parameter Arrays

            offset = 0;
            if (ndims == 2 && dim[1] <= kw.nerr_dim_id) {
                for (i = 0; i < dim[1]; i++) {
                    sprintf(xml_str, "      <dimension dimid=\"%d\" model=\"%d\">", kw.err_dim_id[i],
                            kw.err_dim_model[i]);
                    strcat(xml, xml_str);
                    strcat(xml, lfcr);
                    strcat(xml, "        <params>");
                    if (kw.err_dim_params->type == IDL_TYP_FLOAT) {
                        ferr_dim_params = (float*) kw.err_dim_params->value.arr->data;        // Float Array
                        if (dim[0] == 1) {
                            sprintf(xml_str, "%f", *(ferr_dim_params + offset++));
                            strcat(xml, xml_str);
                        } else {
                            for (j = 0; j < dim[0]; j++) {
                                if (j == dim[0] - 1)
                                    sprintf(xml_str, "%f", *(ferr_dim_params + offset++));
                                else
                                    sprintf(xml_str, "%f,", *(ferr_dim_params + offset++));
                                strcat(xml, xml_str);
                            }
                        }
                    } else {
                        derr_dim_params = (double*) kw.err_dim_params->value.arr->data;        // Double Array
                        if (dim[0] == 1) {
                            sprintf(xml_str, "%f", *(derr_dim_params + offset++));
                            strcat(xml, xml_str);
                        } else {
                            for (j = 0; j < dim[0]; j++) {
                                if (j == dim[0] - 1)
                                    sprintf(xml_str, "%f", *(derr_dim_params + offset++));
                                else
                                    sprintf(xml_str, "%f,", *(derr_dim_params + offset++));
                                strcat(xml, xml_str);
                            }
                        }
                    }
                    strcat(xml, "</params>");
                    strcat(xml, lfcr);
                    strcat(xml, "      </dimension>");
                    strcat(xml, lfcr);
                }
            }
        }
        strcat(xml, "    </errormodel>");
        strcat(xml, lfcr);
    }

//------------------------------------------------------------------------------------------
// Composite Signal

    if (kw.is_comp_signal && strlen(IDL_STRING_STR(&kw.comp_signal)) > 0) {
        sprintf(xml_str, "    <composite id=\"1\" data=\"%s\"", IDL_STRING_STR(&kw.comp_signal));
        strcat(xml, xml_str);
        if (kw.is_comp_error && strlen(IDL_STRING_STR(&kw.comp_error)) > 0) {
            sprintf(xml_str, " error=\"%s\"", IDL_STRING_STR(&kw.comp_error));
            strcat(xml, xml_str);
        }
        if (kw.is_comp_aserror && strlen(IDL_STRING_STR(&kw.comp_aserror)) > 0) {
            sprintf(xml_str, " aserror=\"%s\"", IDL_STRING_STR(&kw.comp_aserror));
            strcat(xml, xml_str);
        }
        sprintf(xml_str, " exp_number_start=\"%d\" exp_number_end=\"%d\" pass_start=\"%d\" pass_end=\"%d\">",
                kw.shotrange[0], kw.shotrange[1], kw.passrange[0], kw.passrange[1]);
        strcat(xml, xml_str);
        strcat(xml, lfcr);

        if (kw.is_comp_dim_id && (kw.is_comp_dim_signal || kw.is_comp_dim_error || kw.is_comp_dim_aserror)) {
            for (i = 0; i < kw.ncomp_dim_id; i++) {
                sprintf(xml_str, "      <composite_dim dimid=\"%d\"", kw.comp_dim_id[i]);
                strcat(xml, xml_str);
                if (kw.is_comp_dim_signal && i < kw.ncomp_dim_signal) {
                    if (strlen(IDL_STRING_STR(&kw.comp_dim_signal[i])) > 0) {
                        sprintf(xml_str, " dim=\"%s\"", IDL_STRING_STR(&kw.comp_dim_signal[i]));
                        strcat(xml, xml_str);
                    }
                }
                if (kw.is_comp_dim_error && i < kw.ncomp_dim_error) {
                    if (strlen(IDL_STRING_STR(&kw.comp_dim_error[i])) > 0) {
                        sprintf(xml_str, " error=\"%s\"", IDL_STRING_STR(&kw.comp_dim_error[i]));
                        strcat(xml, xml_str);
                    }
                }
                if (kw.is_comp_dim_aserror && i < kw.ncomp_dim_aserror) {
                    if (strlen(IDL_STRING_STR(&kw.comp_dim_aserror[i])) > 0) {
                        sprintf(xml_str, " aserror=\"%s\"", IDL_STRING_STR(&kw.comp_dim_aserror[i]));
                        strcat(xml, xml_str);
                    }
                }
                if (kw.is_comp_dim_to && i < kw.ncomp_dim_to) {
                    sprintf(xml_str, " to_dim=\"%d\"", kw.comp_dim_to[i]);
                    strcat(xml, xml_str);
                }
                if (kw.is_comp_dim_from && i < kw.ncomp_dim_from) {
                    sprintf(xml_str, " from_dim=\"%d\"", kw.comp_dim_from[i]);
                    strcat(xml, xml_str);
                }
                strcat(xml, " />");
                strcat(xml, lfcr);
            }
        }
        strcat(xml, "    </composite>");
        strcat(xml, lfcr);
    }

    strcat(xml, "  </signal>");                // XML Tail
    strcat(xml, lfcr);
    strcat(xml, "</action>");

    if (STR_EQUALS(xml, xml_tst)) xml[0] = '\0';            // TEST: No XML from Keywords

    return 0;
}

//=================================================================================================

//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL rosourcestatus(int argc, IDL_VPTR argv[], char* argk) {
//
//
//-------------------------------------------------------------------------

    int i, j, rc, nrows, ncols, ndims, nitems = 0, wherenum = 0, lstr;

    IDL_LONG* llist;
    IDL_INT* ilist;
    int* list = NULL;

    char sql[MAXSQL], returnsql[MAXSQL], sql_str[256];
    char whr[MAXSQL], whr_str[256];

    char old_description[MAXMETA];

    char* source, * type;
    char sql_source[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    static int startup = 1;

// Array Keyword Descriptors

    static IDL_KW_ARR_DESC_R passDesc = {IDL_KW_OFFSETOF(passrange), 1, 2, IDL_KW_OFFSETOF(npassrange)};

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DESCRIPTION", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_description),
              IDL_KW_OFFSETOF(description)},
             {"IMPACT", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_impact), IDL_KW_OFFSETOF(impact)},
             {"LATEST", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(latest)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"PRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_passrange), IDL_CHARA(passDesc)},
             {"QUERY", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_query), IDL_KW_OFFSETOF(query)},
             {"RANGE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(range)},
             {"REASON", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_reason), IDL_KW_OFFSETOF(reason)},
             {"STATUS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_status), IDL_KW_OFFSETOF(status)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_query = 0;
    kw.is_status = 0;
    kw.is_reason = 0;
    kw.is_impact = 0;
    kw.is_description = 0;
    kw.is_pass = 0;
    kw.is_passrange = 0;
    kw.is_type = 0;

    kw.npassrange = 0;
    kw.passrange[0] = -1;
    kw.passrange[1] = -1;

    kw.status = 1;
    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.commit = 0;

    source = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

// Passed Arguments: String followed by an Integer (Scalar or Array)

    if (argv[0]->type == IDL_TYP_STRING) {

        IDL_ENSURE_STRING(argv[0]);
        IDL_ENSURE_SCALAR(argv[0]);
        source = IDL_STRING_STR(&(argv[0]->value.str));                // Data Source Alias
        if (kw.debug)fprintf(stdout, "Source Alias : %s\n", source);

        if (argv[1]->type == IDL_TYP_LONG || argv[1]->type == IDL_TYP_INT) {    // Exp_number & Range
            if (argv[1]->flags & IDL_V_ARR) {
                IDL_ENSURE_ARRAY(argv[1]);
                ndims = (int) argv[1]->value.arr->n_dim;                // Range: Number of Dimensions
                nitems = (int) argv[1]->value.arr->n_elts;                // Number of Elements
                if (ndims != 1) {
                    if (kw.verbose) fprintf(stdout, "The List Array should have dimensionality 1: Check Argument #2\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if (kw.range && nitems != 2) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "The List Array is a Range but the Element Count is Incorrect: Check Argument #2\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if ((list = malloc(nitems * sizeof(int))) == NULL) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "System Problem - Unable to aquire Heap Memory - Check Resources and Try Again!\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if (argv[1]->type == IDL_TYP_INT) {
                    ilist = (IDL_INT*) argv[1]->value.arr->data;
                    for (j = 0; j < nitems; j++) list[j] = (int) (*(ilist + j));
                } else {
                    llist = (IDL_LONG*) argv[1]->value.arr->data;
                    for (j = 0; j < nitems; j++) list[j] = (int) (*(llist + j));
                }
                if (kw.debug) {
                    if (argv[1]->type == IDL_TYP_INT)
                        fprintf(stdout, "List Array is of Type: SHORT\n");
                    else
                        fprintf(stdout, "List Array is of Type: INT\n");
                    fprintf(stdout, "No List Array Items = %d\n", nitems);
                    for (j = 0; j < nitems; j++)fprintf(stdout, "Shot List:  %d\n", list[j]);
                }
            } else {
                IDL_ENSURE_SCALAR(argv[1]);
                nitems = 1;
                if ((list = malloc(nitems * sizeof(int))) == NULL) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "System Problem - Unable to aquire Heap Memory - Check Resources and Try Again!\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                list[0] = IDL_LongScalar(argv[1]);                // Single Experiment Number
                if (kw.debug) {
                    fprintf(stdout, "No List Array Items = %d\n", nitems);
                    fprintf(stdout, "Shot List   : %d\n", list[0]);
                }
            }
        } else {
            if (kw.verbose)
                fprintf(stdout,
                        "Data Source Alias Name Not Followed by an Integer Experiment Number or List Array - Check Argument #2\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    } else {

// Passed Arguments: Integer (Scalar or Array) followed by a String

        if (argv[1]->type == IDL_TYP_STRING) {

            IDL_ENSURE_STRING(argv[1]);
            IDL_ENSURE_SCALAR(argv[1]);
            source = IDL_STRING_STR(&(argv[1]->value.str));            // Data Source Alias
            if (kw.debug)fprintf(stdout, "Source Alias : %s\n", source);

            if (argv[0]->type == IDL_TYP_LONG || argv[0]->type == IDL_TYP_INT) {    // Exp_number & Range
                if (argv[0]->flags & IDL_V_ARR) {
                    IDL_ENSURE_ARRAY(argv[0]);
                    ndims = (int) argv[0]->value.arr->n_dim;                // Range: Number of Dimensions
                    nitems = (int) argv[0]->value.arr->n_elts;            // Number of Elements
                    if (ndims != 1) {
                        if (kw.verbose)
                            fprintf(stdout, "The List Array should have dimensionality 1: Check Argument #1\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if (kw.range && nitems != 2) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "The List Array is a Range but the Element Count is Incorrect: Check Argument #2\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if ((list = malloc(nitems * sizeof(int))) == NULL) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "System Problem - Unable to acquire Heap Memory - Check Resources and Try Again!\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if (argv[0]->type == IDL_TYP_INT) {
                        ilist = (IDL_INT*) argv[0]->value.arr->data;
                        for (j = 0; j < nitems; j++) list[j] = (int) (*(ilist + j));
                    } else {
                        llist = (IDL_LONG*) argv[0]->value.arr->data;
                        for (j = 0; j < nitems; j++) list[j] = (int) (*(llist + j));
                    }
                    if (kw.debug) {
                        if (argv[0]->type == IDL_TYP_INT)
                            fprintf(stdout, "List Array is of Type: SHORT\n");
                        else
                            fprintf(stdout, "List Array is of Type: INT\n");
                        fprintf(stdout, "No List Array Items = %d\n", nitems);
                        for (j = 0; j < nitems; j++)fprintf(stdout, "Shot List:  %d\n", list[j]);
                    }
                } else {
                    IDL_ENSURE_SCALAR(argv[0]);
                    nitems = 1;
                    if ((list = malloc(nitems * sizeof(int))) == NULL) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "System Problem - Unable to acquire Heap Memory - Check Resources and Try Again!\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    list[0] = IDL_LongScalar(argv[0]);                // Single Experiment Number
                    if (kw.debug) {
                        fprintf(stdout, "No List Array Items = %d\n", nitems);
                        fprintf(stdout, "Shot List   : %d\n", list[0]);
                    }
                }
            } else {
                if (kw.verbose)
                    fprintf(stdout,
                            "Data Source Alias Name Not Followed by an Integer Experiment Number or List - Check Argument #1\n");
                IDL_KW_FREE;
                return (IDL_GettmpLong(1));
            }
        }
    }

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.commit) fprintf(stdout, "Commit Keyword Passed\n");
        if (kw.is_status) fprintf(stdout, "Status Value Passed: %d\n", (int) kw.status);
        if (kw.is_reason) fprintf(stdout, "Reason Code Value Passed: %d\n", (int) kw.reason);
        if (kw.is_impact) fprintf(stdout, "Impact Code Value Passed: %d\n", (int) kw.impact);
        if (kw.is_description)
            fprintf(stdout, "Status Change Description Passed: %s\n", IDL_STRING_STR(&kw.description));
        if (kw.is_type) fprintf(stdout, "Type Value Passed  : %s\n", IDL_STRING_STR(&kw.type));
        if (kw.is_query) fprintf(stdout, "Query Value Passed : %s\n", IDL_STRING_STR(&kw.query));
        if (kw.latest) fprintf(stdout, "Latest Keyword Passed\n");
        if (kw.is_pass) fprintf(stdout, "Pass Value Passed  : %d\n", (int) kw.pass);
        if (kw.range) fprintf(stdout, "Range %d : %d\n", list[0], list[1]);
        if (kw.is_passrange) {
            fprintf(stdout, "No. Pass Range Values Passed: %d\n", (int) kw.npassrange);
            for (i = 0; i < (int) kw.npassrange; i++) fprintf(stdout, "PassRange[%d] = %d\n", i, (int) kw.passrange[i]);
        }
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Source Name

    if (strlen(source) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Data Source Name. Please check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// True Range?

    if (kw.range) {
        if (list[1] < list[0]) {
            if (kw.verbose) fprintf(stdout, "Range is Not in Ascending Order! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Valid Status Value Range

    if (kw.is_status && (kw.status < MINSTATUS || kw.status > MAXSTATUS)) {
        if (kw.verbose)
            fprintf(stdout, "Status Values are Invalid. They must be in the Range: %d - %d \n", MINSTATUS, MAXSTATUS);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Pass Values

    if (kw.is_pass && kw.pass < -1) {
        if (kw.verbose) fprintf(stdout, "The Pass Value is Invalid (>= -1). Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// True Pass Range?

    if (kw.is_passrange) {
        if (kw.npassrange != 2) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not a 2 Element Array! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.passrange[1] < kw.passrange[0]) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not in Ascending Order! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Type Values

    if (kw.is_type && (strlen(IDL_STRING_STR(&kw.type)) == 0 || strlen(IDL_STRING_STR(&kw.type)) > 1)) {
        if (kw.verbose) fprintf(stdout, "Type Value is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Query Values

    if (kw.is_query && strlen(IDL_STRING_STR(&kw.query)) == 0) {
        if (kw.verbose) fprintf(stdout, "Query Value is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Reason Code

    if (kw.is_reason && (int) kw.reason < 0) {
        if (kw.verbose) fprintf(stdout, "Reason Code is Invalid (>= 0). Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Reason/Impact Description

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) == 0 &&
        (!kw.is_reason || (kw.is_reason && (int) kw.reason != 0))) {
        if (kw.verbose) fprintf(stdout, "Description is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) > MAXMETA) {
        if (kw.verbose) fprintf(stdout, "Description is Too Long (<= %d). Please Shorten!\n", MAXMETA);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Source Name for Querying

    if (strlen(source) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Source name is too Long [%d]\n", MAXNAME - 2);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(source, sql_source);
    } else {
        lstr = strlen(source);
        if (source[0] == '*' || source[0] == '?' ||
            (source[lstr - 2] != '\\' && (source[lstr - 1] == '*' || source[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(source, sql_source);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_source, source);
            strupr(sql_source);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_source);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify Data Source Files

    whr[0] = '\0';
    wherenum = 0;

    if (strlen(source) <= 3) {        // Assume an ALIAS name form is passed
        if (kw.match) {
            strcat(whr, "source_alias ILIKE '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        } else {
            strcat(whr, "upper(source_alias) = '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        }
        wherenum++;
    } else {
        if (strlen(source) > 0) {
            if (kw.match) {
                strcat(whr, "filename ILIKE '");            // otherwise a regular name form is passed
                strcat(whr, sql_source);
                strcat(whr, "'");
            } else {
                strcat(whr, "upper(filename) = '");
                strcat(whr, sql_source);
                strcat(whr, "'");
            }
            wherenum++;
        }
    }

    if (nitems > 0) {                // Pulse List or Range
        if (wherenum > 0) strcat(whr, " AND ");
        if (kw.range) {
            sprintf(whr_str, "exp_number >= %d AND exp_number <= %d", list[0], list[1]);
            strcat(whr, whr_str);
        } else {
            if (nitems > 1) {
                sprintf(whr_str, "exp_number IN(%d", list[0]);
                strcat(whr, whr_str);
                for (i = 1; i < nitems; i++) {
                    sprintf(whr_str, ",%d", list[i]);
                    strcat(whr, whr_str);
                }
                strcat(whr, ") ");
            } else {
                sprintf(whr_str, "exp_number = %d ", list[0]);
                strcat(whr, whr_str);
            }
            free((void*) list);
        }
        wherenum++;
    }

    type = IDL_STRING_STR(&kw.type);

    if (!kw.latest) {
        if (kw.is_passrange && type[0] != 'R' && type[0] != 'r' && kw.passrange[0] >= 0) {        // Not RAW Data
            if (wherenum > 0) strcat(whr, " AND ");
            sprintf(whr_str, "pass >= %d AND pass <= %d", (int) kw.passrange[0], (int) kw.passrange[1]);
            strcat(whr, whr_str);
            wherenum++;
        } else {
            if (kw.is_pass && type[0] != 'R' && type[0] != 'r' && kw.pass >= 0) {            // Not RAW Data
                if (wherenum > 0) strcat(whr, " AND ");
                sprintf(whr_str, "pass = %d", (int) kw.pass);
                strcat(whr, whr_str);
                wherenum++;
            }
        }
    }

    if (STR_IEQUALS(type, "A")) {
        if (wherenum > 0) strcat(whr, " AND ");
        strcat(whr, "type= 'A'");
        wherenum++;
    } else {
        if (STR_IEQUALS(type, "R")) {
            if (wherenum > 0) strcat(whr, " AND ");
            strcat(whr, "type= 'R'");
            wherenum++;
        } else {
            if (STR_IEQUALS(type, "M")) {
                if (wherenum > 0) strcat(whr, " AND ");
                strcat(whr, "type= 'M'");
                wherenum++;
            }
        }
    }

    if (kw.is_query && strlen(IDL_STRING_STR(&kw.query)) > 0) {                // Additional Query
        if (wherenum > 0) strcat(whr, " AND ");
        strcat(whr, IDL_STRING_STR(&kw.query));
        wherenum++;
    }


//-----------------------------------------------------------------------------------------------------------------------------
// Check there is NO Ambiguity in the Source_Alias

    if (kw.latest || (!kw.is_passrange && kw.is_pass && (int) kw.pass == -1)) {
        strcpy(sql,
               "SELECT A.source_alias, count(*) as count FROM (SELECT source_alias, max(pass) as mpass FROM data_source WHERE ");
        strcat(sql, whr);
        strcat(sql, " GROUP BY source_alias) as A GROUP BY source_alias");
    } else {
        strcpy(sql, "SELECT source_alias, count(*) as count FROM data_source WHERE ");
        strcat(sql, whr);
        strcat(sql, " GROUP BY source_alias");
    }

    if (kw.debug)fprintf(stdout, "%s\n", sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) > 1) {
        if (kw.verbose)
            fprintf(stdout, "The Data Source File Alias Name (%s) is Ambiguous ... Please Clarify\n", source);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) == 0) {
        if (kw.verbose)
            fprintf(stdout, "No Data Source Files with the Alias Name (%s) were Found ... Please Clarify\n", source);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.debug)
        fprintf(stdout, "Number of Source Files with Alias Name (%s) Selected: %d\n", PQgetvalue(DBQuery, 0, 0), nrows);

//-----------------------------------------------------------------------------------------------------------------------------
// Validate Reason & Impact Codes

    if (kw.is_reason) {
        sprintf(sql_str, "SELECT count(*) FROM Status_Reason WHERE reason_code = %d", (int) kw.reason);
        if ((rc = countIdamTable(sql_str, kw.verbose, stdout)) == 0) {
            if (kw.verbose)
                fprintf(stdout, "The Status Change Reason Code is Invalid (%d) ... Please Correct\n", (int) kw.reason);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

    if (kw.is_impact) {
        sprintf(sql_str, "SELECT count(*) FROM Status_Impact WHERE impact_code = %d", (int) kw.impact);
        if ((rc = countIdamTable(sql_str, kw.verbose, stdout)) == 0) {
            if (kw.verbose)
                fprintf(stdout, "The Status Change Impact Code is Invalid (%d) ... Please Correct\n", (int) kw.impact);
            if (kw.debug) fprintf(stdout, "%s\n", sql_str);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Check User Authorisation (All Records should have the same Source_Alias name)

    if (kw.commit && (rc = checkSourceAuthorisation(PQgetvalue(DBQuery, 0, 0), clientname, kw.verbose, stdout)) == 1) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - You are Not Authorised to Update Status Values of Source Files %s\n",
                    PQgetvalue(DBQuery, 0, 0));
        kw.commit = 0;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query Data_Source Table for Current Status Values

    PQclear(DBQuery);

    if (kw.latest || (!kw.is_passrange && kw.is_pass && (int) kw.pass == -1)) {
        strcpy(sql, "SELECT A.source_id, A.exp_number, B.mpass, A.source_status, A.source_alias, A.filename, A.type, "
                "A.status_reason_impact_code, A.status_desc_id FROM data_source as A, "
                "(SELECT source_id, max(pass) as mpass FROM data_source WHERE ");
        strcat(sql, whr);
        strcat(sql, " GROUP BY source_id) as B WHERE A.source_id = B.source_id");
    } else {
        strcpy(sql, "SELECT source_id, exp_number, pass, source_status, source_alias, filename, type, "
                "status_reason_impact_code, status_desc_id FROM data_source WHERE ");
        strcat(sql, whr);
    }

    if (kw.debug)fprintf(stdout, "%s\n", sql);

    strcpy(returnsql, sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//---------------------------------------------------------------------------------------------------------
// Update the Status Values

    if (kw.is_status || kw.is_reason || kw.is_impact || kw.is_description) {

        for (i = 0; i < nrows; i++) {

            strcpy(sql, "BEGIN;");

// Status

            if (kw.is_status) {
                sprintf(sql_str, " UPDATE Data_Source SET source_status = %d WHERE source_id = %s",
                        (int) kw.status, PQgetvalue(DBQuery, i, 0));
                strcat(sql, sql_str);
                strcat(sql, "; UPDATE Data_Source SET modified = current_date WHERE source_id =");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql,
                       "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
                strcat(sql, clientname);
                strcat(sql, "','Data_Source',");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, ",'source_status',");
                if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
                    strcat(sql, PQgetvalue(DBQuery, i, 3));
                else
                    strcat(sql, DEFAULTSTATUSSTR);
                strcat(sql, ",'UPDATE Data_Source SET source_status = ");
                if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
                    strcat(sql, PQgetvalue(DBQuery, i, 3));
                else
                    strcat(sql, DEFAULTSTATUSSTR);
                strcat(sql, " WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "');");
            }

// Status Change Reason Code

            if (kw.is_reason || kw.is_impact) {
                int code;
                if (!kw.is_reason) {
                    if (strlen(PQgetvalue(DBQuery, i, 7)) > 0) {
                        kw.reason = (int) atoi(PQgetvalue(DBQuery, i, 7)) / 100;
                    } else {
                        kw.reason = DEFAULTREASONCODE;
                    }
                }
                if (!kw.is_impact) {
                    if (strlen(PQgetvalue(DBQuery, i, 7)) > 0) {
                        code = (int) atoi(PQgetvalue(DBQuery, i, 7));
                        kw.impact = code - 100 * (code / 100);
                    } else {
                        kw.impact = DEFAULTIMPACTCODE;
                    }
                }
                code = 100 * kw.reason + kw.impact;

                sprintf(sql_str, " UPDATE Data_Source SET status_reason_impact_code = %d WHERE source_id = %s",
                        code, PQgetvalue(DBQuery, i, 0));
                strcat(sql, sql_str);
                strcat(sql, "; UPDATE Data_Source SET modified = current_date WHERE source_id =");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql,
                       "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
                strcat(sql, clientname);
                strcat(sql, "','Data_Source',");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, ",'status_reason_impact_code',");
                if (strlen(PQgetvalue(DBQuery, i, 7)) == 0) {
                    strcat(sql, DEFAULTREASONIMPACTCODESTR);
                } else {
                    strcat(sql, PQgetvalue(DBQuery, i, 7));
                }
                strcat(sql, ",'UPDATE Data_Source SET status_reason_impact_code = ");
                if (strlen(PQgetvalue(DBQuery, i, 7)) == 0) {
                    strcat(sql, DEFAULTREASONIMPACTCODESTR);
                } else {
                    strcat(sql, PQgetvalue(DBQuery, i, 7));
                }
                strcat(sql, " WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, "');");
            }

// Status Change Description

            if (kw.is_description) {
                if (strlen(PQgetvalue(DBQuery, i, 8)) == 0) {    // No Previous Description? - Create a New Record

                    strcat(sql,
                           " INSERT INTO Status_Desc (status_desc_id,description) VALUES (nextval('status_desc_id_seq'),'");
                    strcat(sql, IDL_STRING_STR(&kw.description));
                    strcat(sql,
                           "'); UPDATE Data_Source SET status_desc_id = currval('status_desc_id_seq') WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, "; UPDATE Data_Source SET modified = current_date WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql,
                           "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
                    strcat(sql, clientname);
                    strcat(sql, "','Data_Source',");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, ",'status_desc_id',0,");
                    strcat(sql, "'UPDATE Data_Source SET status_desc_id = 0 WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, "');");

                } else {        // Read the Previous Description and Update the current Record

                    sprintf(sql_str, "SELECT description FROM Status_Desc WHERE status_desc_id = %s",
                            PQgetvalue(DBQuery, i, 8));
                    if (kw.debug)fprintf(stdout, "\n%s\n\n", sql_str);
                    if ((rc = queryIdamTable(sql_str, kw.verbose, stdout, old_description)) != 0) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "ERROR ... Unable to Read the Previous Status Change Description for Database!\n");
                        PQclear(DBQuery);
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    sprintf(sql_str, " UPDATE Status_Desc SET description = '%s' WHERE status_desc_id = %s",
                            IDL_STRING_STR(&kw.description), PQgetvalue(DBQuery, i, 8));
                    strcat(sql, sql_str);
                    strcat(sql, "; UPDATE Status_Desc SET modified = current_date WHERE status_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql,
                           "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
                    strcat(sql, clientname);
                    strcat(sql, "','Status_Desc',");
                    strcat(sql, PQgetvalue(DBQuery, i, 8));
                    strcat(sql, ",'description','");
                    strcat(sql, old_description);
                    strcat(sql, "','UPDATE Status_Desc SET description = \\'");
                    strcat(sql, old_description);
                    strcat(sql, "\\' WHERE status_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 8));
                    strcat(sql, "');");
                }
            }

            strcat(sql, " END;");

            if (kw.debug)fprintf(stdout, "\n%s\n\n", sql);

            if (kw.commit) {
                if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                    if (kw.verbose)
                        fprintf(stdout, "ERROR - Data_Source Status NOT Updated (record %s)\n",
                                PQgetvalue(DBQuery, i, 0));
                    PQclear(DBQuery);
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
            }
        }
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);

    return (returnIdamSourceSQL(returnsql, kw.verbose, kw.debug, stdout));
}


IDL_VPTR returnIdamSourceSQL(char* sql, int verbose, int debug, FILE* fh) {
//
//
// v0.01	November 2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------

    int i, nrows, ncols, rc;
    char description[MAXMETA];
    char sql_str[256];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int source_id;
        int exp_number;
        int pass;
        int status;
        int reason;
        int impact;
        IDL_STRING source_alias;
        IDL_STRING filename;
        IDL_STRING type;
        IDL_STRING description;
    } RSOURCE;

    RSOURCE* rsql;            // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"SOURCE_ID",    0,     (void*) IDL_TYP_LONG},
            {"EXP_NUMBER",   0,     (void*) IDL_TYP_LONG},
            {"PASS",         0,     (void*) IDL_TYP_LONG},
            {"STATUS",       0,     (void*) IDL_TYP_LONG},
            {"REASON",       0,     (void*) IDL_TYP_LONG},
            {"IMPACT",       0,     (void*) IDL_TYP_LONG},
            {"SOURCE_ALIAS", vdlen, (void*) IDL_TYP_STRING},
            {"FILENAME",     vdlen, (void*) IDL_TYP_STRING},
            {"TYPE",         vdlen, (void*) IDL_TYP_STRING},
            {"DESCRIPTION",  vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

//-------------------------------------------------------------
// Execute the Passed Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RSOURCE*) malloc(nrows * sizeof(RSOURCE))) == NULL) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].source_id = (int) atoi(PQgetvalue(DBQuery, i, 0));
        rsql[i].exp_number = (int) atoi(PQgetvalue(DBQuery, i, 1));
        rsql[i].pass = (int) atoi(PQgetvalue(DBQuery, i, 2));
        rsql[i].status = (int) atoi(PQgetvalue(DBQuery, i, 3));
        rsql[i].reason = (int) atoi(PQgetvalue(DBQuery, i, 7)) / 100;
        rsql[i].impact = (int) atoi(PQgetvalue(DBQuery, i, 7)) - 100 * rsql[i].reason;

        if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
            IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 4));
        else
            IDL_StrStore(&(rsql[i].source_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
            IDL_StrStore(&(rsql[i].filename), PQgetvalue(DBQuery, i, 5));
        else
            IDL_StrStore(&(rsql[i].filename), "");

        if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
            IDL_StrStore(&(rsql[i].type), PQgetvalue(DBQuery, i, 6));
        else
            IDL_StrStore(&(rsql[i].type), "");

        if (strlen(PQgetvalue(DBQuery, i, 8)) > 0) {
            sprintf(sql_str, "SELECT description FROM Status_Desc WHERE status_desc_id = %s",
                    PQgetvalue(DBQuery, i, 8));
            if ((rc = queryIdamTable(sql_str, verbose, fh, description)) == 0)
                IDL_StrStore(&(rsql[i].description), description);
            else
                IDL_StrStore(&(rsql[i].description), "");
        } else
            IDL_StrStore(&(rsql[i].description), "");
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    PQclear(DBQuery);
    return (ivReturn);
}




//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL rosignalstatus(int argc, IDL_VPTR argv[], char* argk) {
//
//
//-------------------------------------------------------------------------

    int i, j, rc, nrows, ncols, ndims, nitems, wherenum = 0, lstr;
    int msource = 0, msignal = 0;

    IDL_LONG* llist;
    IDL_INT* ilist;
    int* list = NULL;

    char sql[MAXSQL], returnsql[MAXSQL], sql_str[256];
    char whr[MAXSQL], whr_sig[MAXSQL], whr_str[256];

    char old_description[MAXMETA];

    char* signal, * source, * type;
    char sql_source[MAXNAME], sql_signal[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    static int startup = 1;

// Array Keyword Descriptors

    static IDL_KW_ARR_DESC_R passDesc = {IDL_KW_OFFSETOF(passrange), 1, 2, IDL_KW_OFFSETOF(npassrange)};

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"ALIAS", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(alias)},
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DESCRIPTION", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_description),
              IDL_KW_OFFSETOF(description)},
             {"IMPACT", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_impact), IDL_KW_OFFSETOF(impact)},
             {"LATEST", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(latest)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"PRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_passrange), IDL_CHARA(passDesc)},
             {"QUERY", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_query), IDL_KW_OFFSETOF(query)},
             {"RANGE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(range)},
             {"REASON", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_reason), IDL_KW_OFFSETOF(reason)},
             {"SOURCE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_source), IDL_KW_OFFSETOF(source)},
             {"STATUS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_status), IDL_KW_OFFSETOF(status)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_query = 0;
    kw.is_status = 0;
    kw.is_reason = 0;
    kw.is_impact = 0;
    kw.is_description = 0;
    kw.is_source = 0;
    kw.is_pass = 0;
    kw.is_passrange = 0;
    kw.range = 0;
    kw.is_type = 0;

    kw.npassrange = 0;
    kw.passrange[0] = -1;
    kw.passrange[1] = -1;

    kw.status = 1;
    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.commit = 0;
    kw.alias = 0;

    signal = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

// Passed Arguments: String followed by an Integer (Scalar or Array)

    if (argv[0]->type == IDL_TYP_STRING) {

        IDL_ENSURE_STRING(argv[0]);
        IDL_ENSURE_SCALAR(argv[0]);
        signal = IDL_STRING_STR(&(argv[0]->value.str));                // Signal Alias
        if (kw.debug)fprintf(stdout, "Signal Alias : %s\n", signal);

        if (argv[1]->type == IDL_TYP_LONG || argv[1]->type == IDL_TYP_INT) {    // Exp_number & Range
            if (argv[1]->flags & IDL_V_ARR) {
                IDL_ENSURE_ARRAY(argv[1]);
                ndims = (int) argv[1]->value.arr->n_dim;                // Range: Number of Dimensions
                nitems = (int) argv[1]->value.arr->n_elts;                // Number of Elements
                if (ndims != 1) {
                    if (kw.verbose) fprintf(stdout, "The List Array should have dimensionality 1: Check Argument #2\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if (kw.range && nitems != 2) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "The List Array is a Range but the Element Count is Incorrect: Check Argument #2\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if ((list = malloc(nitems * sizeof(int))) == NULL) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "System Problem - Unable to aquire Heap Memory - Check Resources and Try Again!\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                if (argv[1]->type == IDL_TYP_INT) {
                    ilist = (IDL_INT*) argv[1]->value.arr->data;
                    for (j = 0; j < nitems; j++) list[j] = (int) (*(ilist + j));
                } else {
                    llist = (IDL_LONG*) argv[1]->value.arr->data;
                    for (j = 0; j < nitems; j++) list[j] = (int) (*(llist + j));
                }
                if (kw.debug) {
                    if (argv[1]->type == IDL_TYP_INT)
                        fprintf(stdout, "List Array is of Type: SHORT\n");
                    else
                        fprintf(stdout, "List Array is of Type: INT\n");
                    fprintf(stdout, "No List Array Items = %d\n", nitems);
                    for (j = 0; j < nitems; j++)fprintf(stdout, "Shot List:  %d\n", list[j]);
                }
            } else {
                IDL_ENSURE_SCALAR(argv[1]);
                nitems = 1;
                if ((list = malloc(nitems * sizeof(int))) == NULL) {
                    if (kw.verbose)
                        fprintf(stdout,
                                "System Problem - Unable to acquire Heap Memory - Check Resources and Try Again!\n");
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
                list[0] = IDL_LongScalar(argv[1]);                // Single Experiment Number
                if (kw.debug) {
                    fprintf(stdout, "No List Array Items = %d\n", nitems);
                    fprintf(stdout, "Shot List   : %d\n", list[0]);
                }
            }
        } else {
            if (kw.verbose)
                fprintf(stdout,
                        "Signal Alias Name Not Followed by an Integer Experiment Number or List Array - Check Argument #2\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    } else {

// Passed Arguments: Integer (Scalar or Array) followed by a String

        if (argv[1]->type == IDL_TYP_STRING) {

            IDL_ENSURE_STRING(argv[1]);
            IDL_ENSURE_SCALAR(argv[1]);
            signal = IDL_STRING_STR(&(argv[1]->value.str));            // Signal Alias
            if (kw.debug)fprintf(stdout, "Signal Alias : %s\n", signal);

            if (argv[0]->type == IDL_TYP_LONG || argv[0]->type == IDL_TYP_INT) {    // Exp_number & Range
                if (argv[0]->flags & IDL_V_ARR) {
                    IDL_ENSURE_ARRAY(argv[0]);
                    ndims = (int) argv[0]->value.arr->n_dim;                // Range: Number of Dimensions
                    nitems = (int) argv[0]->value.arr->n_elts;                // Number of Elements
                    if (ndims != 1) {
                        if (kw.verbose)
                            fprintf(stdout, "The List Array should have dimensionality 1: Check Argument #1\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if (kw.range && nitems != 2) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "The List Array is a Range but the Element Count is Incorrect: Check Argument #2\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if ((list = malloc(nitems * sizeof(int))) == NULL) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "System Problem - Unable to aquire Heap Memory - Check Resources and Try Again!\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    if (argv[0]->type == IDL_TYP_INT) {
                        ilist = (IDL_INT*) argv[0]->value.arr->data;
                        for (j = 0; j < nitems; j++) list[j] = (int) (*(ilist + j));
                    } else {
                        llist = (IDL_LONG*) argv[0]->value.arr->data;
                        for (j = 0; j < nitems; j++) list[j] = (int) (*(llist + j));
                    }
                    if (kw.debug) {
                        if (argv[0]->type == IDL_TYP_INT)
                            fprintf(stdout, "List Array is of Type: SHORT\n");
                        else
                            fprintf(stdout, "List Array is of Type: INT\n");
                        fprintf(stdout, "No List Array Items = %d\n", nitems);
                        for (j = 0; j < nitems; j++)fprintf(stdout, "Shot List:  %d\n", list[j]);
                    }
                } else {
                    IDL_ENSURE_SCALAR(argv[0]);
                    nitems = 1;
                    if ((list = malloc(nitems * sizeof(int))) == NULL) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "System Problem - Unable to aquire Heap Memory - Check Resources and Try Again!\n");
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    list[0] = IDL_LongScalar(argv[0]);                // Single Experiment Number
                    if (kw.debug) {
                        fprintf(stdout, "No List Array Items = %d\n", nitems);
                        fprintf(stdout, "Shot List   : %d\n", list[0]);
                    }
                }
            } else {
                if (kw.verbose)
                    fprintf(stdout,
                            "Signal Alias Name Not Followed by an Integer Experiment Number or List - Check Argument #1\n");
                IDL_KW_FREE;
                return (IDL_GettmpLong(1));
            }
        }
    }

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.commit) fprintf(stdout, "Commit Keyword Passed\n");
        if (kw.is_status) fprintf(stdout, "Status Value Passed: %d\n", (int) kw.status);
        if (kw.is_reason) fprintf(stdout, "Reason Code Value Passed: %d\n", (int) kw.reason);
        if (kw.is_impact) fprintf(stdout, "Impact Code Value Passed: %d\n", (int) kw.impact);
        if (kw.is_description)
            fprintf(stdout, "Status Change Description Passed: %s\n", IDL_STRING_STR(&kw.description));
        if (kw.is_source) fprintf(stdout, "Source Value Passed: %s\n", IDL_STRING_STR(&kw.source));
        if (kw.is_type) fprintf(stdout, "Type Value Passed  : %s\n", IDL_STRING_STR(&kw.type));
        if (kw.is_query) fprintf(stdout, "Query Value Passed : %s\n", IDL_STRING_STR(&kw.query));
        if (kw.latest) fprintf(stdout, "Latest Keyword Passed\n");
        if (kw.is_pass) fprintf(stdout, "Pass Value Passed  : %d\n", (int) kw.pass);
        if (kw.range) fprintf(stdout, "Range %d : %d\n", list[0], list[1]);
        if (kw.is_passrange) {
            fprintf(stdout, "No. Pass Range Values Passed: %d\n", (int) kw.npassrange);
            for (i = 0; i < (int) kw.npassrange; i++) fprintf(stdout, "PassRange[%d] = %d\n", i, (int) kw.passrange[i]);
        }
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Signal Name

    if (strlen(signal) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Name. Please check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Source Name

    if (kw.is_source && strlen(IDL_STRING_STR(&kw.source)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Data Source Alias. Please check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// True Range?

    if (kw.range) {
        if (list[1] < list[0]) {
            if (kw.verbose) fprintf(stdout, "Range is Not in Ascending Order! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Valid Status Value Range

    if (kw.is_status && (kw.status < MINSTATUS || kw.status > MAXSTATUS)) {
        if (kw.verbose)
            fprintf(stdout, "Status Values are Invalid. They must be in the Range: %d - %d \n", MINSTATUS, MAXSTATUS);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Pass Values

    if (kw.is_pass && kw.pass < -1) {
        if (kw.verbose) fprintf(stdout, "The Pass Value is Invalid (> -1). Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// True Pass Range?

    if (kw.is_passrange) {
        if (kw.npassrange != 2) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not a 2 Element Array! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.passrange[1] < kw.passrange[0]) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not in Ascending Order! Please correct!\n");
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Type Values

    if (kw.is_type && (strlen(IDL_STRING_STR(&kw.type)) == 0 || strlen(IDL_STRING_STR(&kw.type)) > 1)) {
        if (kw.verbose) fprintf(stdout, "Type Value is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }


// Query Values

    if (kw.is_query && strlen(IDL_STRING_STR(&kw.query)) == 0) {
        if (kw.verbose) fprintf(stdout, "Query Value is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Reason Code

    if (kw.is_reason && (int) kw.reason < 0) {
        if (kw.verbose) fprintf(stdout, "Reason Code is Invalid (>= 0). Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Reason/Impact Description

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) == 0 &&
        (!kw.is_reason || (kw.is_reason && (int) kw.reason != 0))) {
        if (kw.verbose) fprintf(stdout, "Description is Invalid. Please Check!\n");
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) > MAXMETA) {
        if (kw.verbose) fprintf(stdout, "Description is Too Long (<= %d). Please Check!\n", MAXMETA);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Source Name for Querying

    if (kw.is_source) {

        source = IDL_STRING_STR(&kw.source);

        if (strlen(source) >= MAXNAME - 2) {
            if (kw.verbose) fprintf(stdout, "Source name is too Long [%d]\n", MAXNAME - 2);
            free((void*) list);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

    if (strlen(signal) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Signal name is too Long [%d]\n", MAXNAME - 2);
        free((void*) list);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(signal, sql_signal);
        if (kw.is_source) sqlIdamPatternMatch(source, sql_source);
    } else {
        if (kw.is_source) {
            lstr = strlen(source);
            if (source[0] == '*' || source[0] == '?' ||
                (source[lstr - 2] != '\\' && (source[lstr - 1] == '*' || source[lstr - 1] == '?'))) {
                sqlIdamPatternMatch(source, sql_source);
                msource = 1;                    // Flag Pattern Matching to be used
            } else {
                strcpy(sql_source, source);
                strupr(sql_source);        // If Pattern Matching is Not Specified then convert case
            }
        }

        lstr = strlen(signal);
        if (signal[0] == '*' || signal[0] == '?' ||
            (signal[lstr - 2] != '\\' && (signal[lstr - 1] == '*' || signal[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(signal, sql_signal);
            msignal = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_signal, signal);
            strupr(sql_signal);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_signal);
    if (kw.is_source) TrimString(sql_source);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify Data Source Files

    whr[0] = '\0';
    wherenum = 0;
    type = IDL_STRING_STR(&kw.type);

    if (kw.is_source) {
        if (kw.match || msource) {
            strcat(whr, "source_alias ILIKE '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        } else {
            strcat(whr, "upper(source_alias) = '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        }
        wherenum++;
    }

    if (nitems > 0) {                // Pulse List or Range
        if (wherenum > 0) strcat(whr, " AND ");
        if (kw.range) {
            sprintf(whr_str, "exp_number >= %d AND exp_number <= %d", list[0], list[1]);
            strcat(whr, whr_str);
        } else {
            if (nitems > 1) {
                sprintf(whr_str, "exp_number IN(%d", list[0]);
                strcat(whr, whr_str);
                for (i = 1; i < nitems; i++) {
                    sprintf(whr_str, ",%d", list[i]);
                    strcat(whr, whr_str);
                }
                strcat(whr, ") ");
            } else {
                sprintf(whr_str, "exp_number = %d ", list[0]);
                strcat(whr, whr_str);
            }
            free((void*) list);
        }
        wherenum++;
    }

    if (!kw.latest) {
        if (kw.is_passrange && type[0] != 'R' && type[0] != 'r' && kw.passrange[0] >= 0) {        // Not RAW Data
            if (wherenum > 0) strcat(whr, " AND ");
            sprintf(whr_str, "pass >= %d AND pass <= %d", (int) kw.passrange[0], (int) kw.passrange[1]);
            strcat(whr, whr_str);
            wherenum++;
        } else {
            if (kw.is_pass && type[0] != 'R' && type[0] != 'r' && kw.pass >= 0) {            // Not RAW Data
                if (wherenum > 0) strcat(whr, " AND ");
                sprintf(whr_str, "pass = %d", (int) kw.pass);
                strcat(whr, whr_str);
                wherenum++;
            }
        }
    }

    if (STR_IEQUALS(type, "A")) {
        if (wherenum > 0) strcat(whr, " AND ");
        strcat(whr, "type= 'A'");
        wherenum++;
    } else {
        if (STR_IEQUALS(type, "R")) {
            if (wherenum > 0) strcat(whr, " AND ");
            strcat(whr, "type= 'R'");
            wherenum++;
        } else {
            if (STR_IEQUALS(type, "M")) {
                if (wherenum > 0) strcat(whr, " AND ");
                strcat(whr, "type= 'M'");
                wherenum++;
            }
        }
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify Signals

    whr_sig[0] = '\0';
    wherenum = 0;

    if (kw.match || msignal) {                        // Use Pattern Matching
        if (kw.alias) {                            // RO has Indicated the signal name is an Alias
            strcat(whr_sig, "signal_alias ILIKE '");
            strcat(whr_sig, sql_signal);
            strcat(whr_sig, "'");
        } else {
            strcat(whr_sig, "signal_name ILIKE '");
            strcat(whr_sig, sql_signal);
            strcat(whr_sig, "'");
        }
        wherenum++;
    } else {
        if (kw.alias) {
            strcat(whr_sig, "upper(signal_alias) = '");
            strcat(whr_sig, sql_signal);
            strcat(whr_sig, "'");
        } else {
            strcat(whr_sig, "upper(signal_name) = '");
            strcat(whr_sig, sql_signal);
            strcat(whr_sig, "'");
        }
        wherenum++;
    }

    if (STR_IEQUALS(type, "A")) {
        if (wherenum > 0) strcat(whr_sig, " AND ");
        strcat(whr_sig, "type= 'A'");
        wherenum++;
    } else {
        if (STR_IEQUALS(type, "R")) {
            if (wherenum > 0) strcat(whr_sig, " AND ");
            strcat(whr_sig, "type= 'R'");
            wherenum++;
        } else {
            if (STR_IEQUALS(type, "M")) {
                if (wherenum > 0) strcat(whr_sig, " AND ");
                strcat(whr_sig, "type= 'M'");
                wherenum++;
            }
        }
    }
/*
      if(kw.is_query && strlen(IDL_STRING_STR(&kw.query)) > 0){				// Additional Query used against Signal Table
         if(wherenum > 0) strcat(whr_sig," AND ");
         strcat(whr_sig, IDL_STRING_STR(&kw.query));
	 wherenum++;
      }
*/

//-----------------------------------------------------------------------------------------------------------------------------
// Check there is NO Ambiguity in the Source_Alias if Specified

    if (kw.is_source) {

        if (kw.latest || (!kw.is_passrange && kw.is_pass && (int) kw.pass == -1)) {
            strcpy(sql,
                   "SELECT A.source_alias, count(*) as count FROM (SELECT source_alias, max(pass) as mpass FROM data_source WHERE ");
            strcat(sql, whr);
            strcat(sql, " GROUP BY source_alias) as A GROUP BY source_alias");
        } else {
            strcpy(sql, "SELECT source_alias, count(*) as count FROM data_source WHERE ");
            strcat(sql, whr);
            strcat(sql, " GROUP BY source_alias");
        }

        if (kw.debug)fprintf(stdout, "%s\n", sql);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        if ((nrows = PQntuples(DBQuery)) > 1) {
            if (kw.verbose)
                fprintf(stdout, "The Specified Data Source File Alias Name (%s) is Ambiguous ... Please Clarify\n",
                        IDL_STRING_STR(&kw.source));
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        if ((nrows = PQntuples(DBQuery)) == 0) {
            if (kw.verbose)
                fprintf(stdout,
                        "No Data Source Files with the Specified Alias Name (%s) were Found ... Please Clarify\n",
                        IDL_STRING_STR(&kw.source));
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        source = PQgetvalue(DBQuery, 0, 0);

        if (kw.debug)
            fprintf(stdout, "Number of Source Files with Specified Alias Name (%s) Selected: %d\n", source, nrows);

    }

//-----------------------------------------------------------------------------------------------------------------------------
// Check there is NO Ambiguity in the Source of Signals

    if (!kw.is_source) {

        strcpy(sql, "SELECT source_alias, count(*) as count FROM ");
        strcat(sql, "(SELECT source_alias FROM Signal, "
                "(SELECT signal_desc_id as desc_id FROM signal_desc WHERE ");
        strcat(sql, whr_sig);
        if (kw.latest) {
            strcat(sql,
                   ") as A, (SELECT source_alias, source_id as src_id, max(pass) as mpass FROM data_source WHERE ");
            strcat(sql, whr);
            strcat(sql, " GROUP BY source_alias, src_id");
        } else {
            strcat(sql, ") as A, (SELECT source_id as src_id, source_alias FROM data_source WHERE ");
            strcat(sql, whr);
        }
        strcat(sql,
               ") as B WHERE Signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C GROUP BY source_alias");

        if (kw.debug)fprintf(stdout, "%s\n", sql);

        if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        if ((nrows = PQntuples(DBQuery)) > 1) {
            if (kw.verbose)
                fprintf(stdout, "The Data Source associated with the Signal (%s) is Ambiguous ... Please Clarify\n",
                        signal);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        if ((nrows = PQntuples(DBQuery)) == 0) {
            if (kw.verbose)
                fprintf(stdout, "No Data Source Files associated with the Signal (%s) were Found ... Please Clarify\n",
                        signal);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }

        source = PQgetvalue(DBQuery, 0, 0);                // Modify the Source Where to Include the Alias Name

        if (strlen(whr) > 0 && strlen(source) > 0) {
            if (kw.match) {
                strcat(whr, " AND source_alias ILIKE '");
                strcat(whr, source);
                strcat(whr, "'");
            } else {
                strcat(whr, " AND upper(source_alias) = upper('");
                strcat(whr, source);
                strcat(whr, "')");
            }
        }

        if (kw.debug)fprintf(stdout, "Number of Source Files with the Alias Name (%s) selected: %d\n", source, nrows);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Validate Reason & Impact Codes

    if (kw.is_reason) {
        sprintf(sql_str, "SELECT count(*) FROM Status_Reason WHERE reason_code = %d", (int) kw.reason);
        if ((rc = countIdamTable(sql_str, kw.verbose, stdout)) == 0) {
            if (kw.verbose)
                fprintf(stdout, "The Status Change Reason Code is Invalid (%d) ... Please Correct\n", (int) kw.reason);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

    if (kw.is_impact) {
        sprintf(sql_str, "SELECT count(*) FROM Status_Impact WHERE impact_code = %d", (int) kw.impact);
        if ((rc = countIdamTable(sql_str, kw.verbose, stdout)) == 0) {
            if (kw.verbose)
                fprintf(stdout, "The Status Change Impact Code is Invalid (%d) ... Please Correct\n", (int) kw.impact);
            if (kw.debug) fprintf(stdout, "%s\n", sql_str);
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }
//-----------------------------------------------------------------------------------------------------------------------------
// Query Signal Table for Current Status Values

    PQclear(DBQuery);

    strcpy(sql, "SELECT * FROM (SELECT source_id, signal_desc_id, signal_status, source_alias, exp_number, pass, "
            "filename, type, signal_name, signal_alias, generic_name, status_reason_impact_code, status_desc_id FROM Signal,"
            "(SELECT signal_desc_id as desc_id, signal_name, signal_alias, generic_name FROM signal_desc WHERE ");
    strcat(sql, whr_sig);
    strcat(sql, ") as A, ");

    if (kw.latest || (!kw.is_passrange && kw.is_pass && (int) kw.pass == -1)) {
        strcat(sql, "(SELECT * FROM (SELECT source_id as src_id, source_alias, exp_number, pass, "
                "filename, type FROM data_source WHERE ");
        strcat(sql, whr);
        strcat(sql, ") as B1, (SELECT source_id as src_key, max(pass) as mpass FROM data_source WHERE ");
        strcat(sql, whr);
        strcat(sql, " GROUP BY src_key) as B2 WHERE B1.src_id = B2.src_key");
    } else {
        strcat(sql, "(SELECT source_id as src_id, source_alias, exp_number, pass, filename, type "
                "FROM data_source WHERE ");
        strcat(sql, whr);
    }
    if (kw.is_query && strlen(IDL_STRING_STR(&kw.query)) > 0) {        // Additional Query used against Signal Table
        strcat(sql, ") as B WHERE Signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C WHERE ");
        strcat(sql, IDL_STRING_STR(&kw.query));
        strcat(sql, " ORDER BY signal_name;");
    } else {
        strcat(sql,
               ") as B WHERE Signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C ORDER BY signal_name;");
    }

    if (kw.debug)fprintf(stdout, "%s\n", sql);

    strcpy(returnsql, sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//---------------------------------------------------------------------------------------------------------
// Update the Status Values

    if (kw.is_status || kw.is_reason || kw.is_impact || kw.is_description) {

        for (i = 0; i < nrows; i++) {

// Check User Authorisation: Continue Looping if Not Authorised

            if (kw.commit &&
                (rc = checkSignalAuthorisation(0, PQgetvalue(DBQuery, i, 8), clientname, kw.verbose, stdout)) == 1) {
                if (kw.verbose)
                    fprintf(stdout, "ERROR - You are Not Authorised to Update Status Values of the Signal %s\n",
                            PQgetvalue(DBQuery, i, 8));
                continue;
            }

            strcpy(sql, "BEGIN;");

// Status

            if (kw.is_status) {
                sprintf(sql_str, " UPDATE Signal SET signal_status = %d WHERE source_id = %s AND signal_desc_id = %s",
                        (int) kw.status, PQgetvalue(DBQuery, i, 0), PQgetvalue(DBQuery, i, 1));
                strcat(sql, sql_str);
                strcat(sql, "; UPDATE Signal SET modified = current_date WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, " AND signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql,
                       "; INSERT INTO Change_Log (username,dbtable,key1,key2,dbfield,prior_value,rollback_sql) VALUES ('");
                strcat(sql, clientname);
                strcat(sql, "','Signal',");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, ",");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql, ",'signal_status',");
                if (strlen(PQgetvalue(DBQuery, i, 2)) > 0)
                    strcat(sql, PQgetvalue(DBQuery, i, 2));
                else
                    strcat(sql, DEFAULTSTATUSSTR);
                strcat(sql, ",'UPDATE Signal SET source_status = ");
                if (strlen(PQgetvalue(DBQuery, i, 2)) > 0)
                    strcat(sql, PQgetvalue(DBQuery, i, 2));
                else
                    strcat(sql, DEFAULTSTATUSSTR);
                strcat(sql, " WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, " AND signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql, "');");
            }

// Status Change Reason Code

            if (kw.is_reason || kw.is_impact) {
                int code;
                if (!kw.is_reason) {
                    if (strlen(PQgetvalue(DBQuery, i, 11)) > 0) {
                        kw.reason = (int) atoi(PQgetvalue(DBQuery, i, 11)) / 100;
                    } else {
                        kw.reason = DEFAULTREASONCODE;
                    }
                }
                if (!kw.is_impact) {
                    if (strlen(PQgetvalue(DBQuery, i, 11)) > 0) {
                        code = (int) atoi(PQgetvalue(DBQuery, i, 11));
                        kw.impact = code - 100 * (code / 100);
                    } else {
                        kw.impact = DEFAULTIMPACTCODE;
                    }
                }
                code = 100 * kw.reason + kw.impact;

                sprintf(sql_str,
                        " UPDATE Signal SET status_reason_impact_code = %d WHERE source_id = %s AND signal_desc_id = %s",
                        code, PQgetvalue(DBQuery, i, 0), PQgetvalue(DBQuery, i, 1));
                strcat(sql, sql_str);
                strcat(sql, "; UPDATE Signal SET modified = current_date WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, " AND signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql,
                       "; INSERT INTO Change_Log (username,dbtable,key1,key2,dbfield,prior_value,rollback_sql) VALUES ('");
                strcat(sql, clientname);
                strcat(sql, "','Signal',");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, ",");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql, ",'status_reason_impact_code',");
                if (strlen(PQgetvalue(DBQuery, i, 11)) == 0) {
                    strcat(sql, DEFAULTREASONIMPACTCODESTR);
                } else {
                    strcat(sql, PQgetvalue(DBQuery, i, 11));
                }
                strcat(sql, ",'UPDATE Signal SET status_reason_impact_code = ");
                if (strlen(PQgetvalue(DBQuery, i, 11)) == 0) {
                    strcat(sql, DEFAULTREASONIMPACTCODESTR);
                } else {
                    strcat(sql, PQgetvalue(DBQuery, i, 11));
                }
                strcat(sql, " WHERE source_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 0));
                strcat(sql, " AND signal_desc_id = ");
                strcat(sql, PQgetvalue(DBQuery, i, 1));
                strcat(sql, "');");
            }

// Status Change Description

            if (kw.is_description) {
                if (strlen(PQgetvalue(DBQuery, i, 12)) == 0) {    // No Previous Description? - Create a New Record

                    strcat(sql,
                           " INSERT INTO Status_Desc (status_desc_id,description) VALUES (nextval('status_desc_id_seq'),'");
                    strcat(sql, IDL_STRING_STR(&kw.description));
                    strcat(sql,
                           "'); UPDATE Signal SET status_desc_id = currval('status_desc_id_seq') WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, " AND signal_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "; UPDATE Signal SET modified = current_date WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, " AND signal_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql,
                           "; INSERT INTO Change_Log (username,dbtable,key1,key2,dbfield,prior_value,rollback_sql) VALUES ('");
                    strcat(sql, clientname);
                    strcat(sql, "','Signal',");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, ",");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, ",'status_desc_id',0,");
                    strcat(sql, "'UPDATE Signal SET status_desc_id = 0 WHERE source_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 0));
                    strcat(sql, " AND signal_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 1));
                    strcat(sql, "');");
                } else {        // Read the Previous Description and Update the current Record

                    sprintf(sql_str, "SELECT description FROM Status_Desc WHERE status_desc_id = %s",
                            PQgetvalue(DBQuery, i, 12));
                    if (kw.debug)fprintf(stdout, "\n%s\n\n", sql_str);
                    if ((rc = queryIdamTable(sql_str, kw.verbose, stdout, old_description)) != 0) {
                        if (kw.verbose)
                            fprintf(stdout,
                                    "ERROR ... Unable to Read the Previous Status Change Description for Database!\n");
                        PQclear(DBQuery);
                        IDL_KW_FREE;
                        return (IDL_GettmpLong(1));
                    }
                    sprintf(sql_str, " UPDATE Status_Desc SET description = '%s' WHERE status_desc_id = %s",
                            IDL_STRING_STR(&kw.description), PQgetvalue(DBQuery, i, 12));
                    strcat(sql, sql_str);
                    strcat(sql, "; UPDATE Status_Desc SET modified = current_date WHERE status_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 12));
                    strcat(sql,
                           "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
                    strcat(sql, clientname);
                    strcat(sql, "','Status_Desc',");
                    strcat(sql, PQgetvalue(DBQuery, i, 12));
                    strcat(sql, ",'description','");
                    strcat(sql, old_description);
                    strcat(sql, "','UPDATE Status_Desc SET description = \\'");
                    strcat(sql, old_description);
                    strcat(sql, "\\' WHERE status_desc_id = ");
                    strcat(sql, PQgetvalue(DBQuery, i, 12));
                    strcat(sql, "');");
                }
            }

            strcat(sql, " END;");

            if (kw.debug)fprintf(stdout, "\n%s\n\n", sql);

            if (kw.commit) {
                if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                    if (kw.verbose)
                        fprintf(stdout, "ERROR - Signal Status NOT Updated (record %s %s)\n",
                                PQgetvalue(DBQuery, i, 0), PQgetvalue(DBQuery, i, 1));
                    PQclear(DBQuery);
                    IDL_KW_FREE;
                    return (IDL_GettmpLong(1));
                }
            }

        }        // End of i loop
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (returnIdamSignalSQL(returnsql, kw.verbose, kw.debug, stdout));
}


IDL_VPTR returnIdamSignalSQL(char* sql, int verbose, int debug, FILE* fh) {
//
//
// v0.01	November 2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------

    int i, nrows, ncols, rc;
    char description[MAXMETA];
    char sql_str[256];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int source_id;
        int signal_desc_id;
        int exp_number;
        int pass;
        int status;
        int reason;
        int impact;
        IDL_STRING source_alias;
        IDL_STRING filename;
        IDL_STRING type;
        IDL_STRING signal_name;
        IDL_STRING signal_alias;
        IDL_STRING generic_name;
        IDL_STRING description;
    } RSIGNAL;

    RSIGNAL* rsql;            // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"SOURCE_ID",      0,     (void*) IDL_TYP_LONG},
            {"SIGNAL_DESC_ID", 0,     (void*) IDL_TYP_LONG},
            {"EXP_NUMBER",     0,     (void*) IDL_TYP_LONG},
            {"PASS",           0,     (void*) IDL_TYP_LONG},
            {"STATUS",         0,     (void*) IDL_TYP_LONG},
            {"REASON",         0,     (void*) IDL_TYP_LONG},
            {"IMPACT",         0,     (void*) IDL_TYP_LONG},
            {"SOURCE_ALIAS",   vdlen, (void*) IDL_TYP_STRING},
            {"FILENAME",       vdlen, (void*) IDL_TYP_STRING},
            {"TYPE",           vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_NAME",    vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_ALIAS",   vdlen, (void*) IDL_TYP_STRING},
            {"GENERIC_NAME",   vdlen, (void*) IDL_TYP_STRING},
            {"DESCRIPTION",    vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

//-------------------------------------------------------------
// Execute the Passed Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RSIGNAL*) malloc(nrows * sizeof(RSIGNAL))) == NULL) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].source_id = (int) atoi(PQgetvalue(DBQuery, i, 0));
        rsql[i].signal_desc_id = (int) atoi(PQgetvalue(DBQuery, i, 1));
        rsql[i].exp_number = (int) atoi(PQgetvalue(DBQuery, i, 4));
        rsql[i].pass = (int) atoi(PQgetvalue(DBQuery, i, 5));
        rsql[i].status = (int) atoi(PQgetvalue(DBQuery, i, 2));
        rsql[i].reason = (int) atoi(PQgetvalue(DBQuery, i, 11)) / 100;
        rsql[i].impact = (int) atoi(PQgetvalue(DBQuery, i, 11)) - 100 * rsql[i].reason;

        if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
            IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 3));
        else
            IDL_StrStore(&(rsql[i].source_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
            IDL_StrStore(&(rsql[i].filename), PQgetvalue(DBQuery, i, 6));
        else
            IDL_StrStore(&(rsql[i].filename), "");

        if (strlen(PQgetvalue(DBQuery, i, 7)) > 0)
            IDL_StrStore(&(rsql[i].type), PQgetvalue(DBQuery, i, 7));
        else
            IDL_StrStore(&(rsql[i].type), "");

        if (strlen(PQgetvalue(DBQuery, i, 8)) > 0)
            IDL_StrStore(&(rsql[i].signal_name), PQgetvalue(DBQuery, i, 8));
        else
            IDL_StrStore(&(rsql[i].signal_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 9)) > 0)
            IDL_StrStore(&(rsql[i].signal_alias), PQgetvalue(DBQuery, i, 9));
        else
            IDL_StrStore(&(rsql[i].signal_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 10)) > 0)
            IDL_StrStore(&(rsql[i].generic_name), PQgetvalue(DBQuery, i, 10));
        else
            IDL_StrStore(&(rsql[i].generic_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 12)) > 0) {
            sprintf(sql_str, "SELECT description FROM Status_Desc WHERE status_desc_id = %s",
                    PQgetvalue(DBQuery, i, 12));
            if ((rc = queryIdamTable(sql_str, verbose, fh, description)) == 0)
                IDL_StrStore(&(rsql[i].description), description);
            else
                IDL_StrStore(&(rsql[i].description), "");
        } else
            IDL_StrStore(&(rsql[i].description), "");

    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    PQclear(DBQuery);
    return (ivReturn);
}



//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL rosignaldesc(int argc, IDL_VPTR argv[], char* argk) {
//
// Change the Signal Description in the Signal_Desc Table
//-------------------------------------------------------------------------

    int i=0, rc, nrows, wherenum = 0, lstr;

    char sql[MAXSQL], returnsql[MAXSQL];
    char whr[MAXSQL];

    char* signal;
    char sql_signal[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    static int startup = 1;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"ALIAS", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(alias)},
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DESCRIPTION", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_description),
              IDL_KW_OFFSETOF(description)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.is_description = 0;
    kw.commit = 0;
    kw.alias = 0;

    signal = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

// Passed Arguments: String followed by an Integer (Scalar or Array)

    if (argv[0]->type == IDL_TYP_STRING) {

        IDL_ENSURE_STRING(argv[0]);
        IDL_ENSURE_SCALAR(argv[0]);

        signal = IDL_STRING_STR(&(argv[0]->value.str));                // Signal Name

        if (kw.debug) fprintf(stdout, "Signal Name      : %s\n", signal);
    }

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.is_description)
            fprintf(stdout, "Status Change Description Passed: %s\n", IDL_STRING_STR(&kw.description));
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.commit) fprintf(stdout, "Commit Keyword Passed\n");
        if (kw.alias) fprintf(stdout, "Alias Keyword Passed\n");
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Signal Name

    if (strlen(signal) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Reason/Impact Description

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) == 0) {
        if (kw.verbose) fprintf(stdout, "Description is Invalid. Please Check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.is_description && strlen(IDL_STRING_STR(&kw.description)) > MAXMETA) {
        if (kw.verbose) fprintf(stdout, "Description is Too Long (<= %d). Please Check!\n", MAXMETA);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Signal Name for Querying

    if (strlen(signal) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Signal name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(signal, sql_signal);
    } else {
        lstr = strlen(signal);
        if (signal[0] == '*' || signal[0] == '?' ||
            (signal[lstr - 2] != '\\' && (signal[lstr - 1] == '*' || signal[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(signal, sql_signal);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_signal, signal);
            strupr(sql_signal);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_signal);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    if (kw.match) {                    // Use Pattern Matching
        if (kw.alias) {                    // RO has Indicated the signal name is an Alias
            strcat(whr, "signal_alias ILIKE '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        } else {
            strcat(whr, "signal_name ILIKE '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        }
        wherenum++;
    } else {
        if (kw.alias) {
            strcat(whr, "upper(signal_alias) = '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        } else {
            strcat(whr, "upper(signal_name) = '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        }
        wherenum++;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query Signal_Desc Table and Check there is NO Ambiguity in the Sgnal Name

    PQclear(DBQuery);

    strcpy(sql, "SELECT signal_desc_id, description, signal_name, signal_alias, generic_name, "
            "signal_class, signal_owner FROM Signal_Desc WHERE ");
    strcat(sql, whr);

    strcpy(returnsql, sql);

    if (kw.debug)fprintf(stdout, "%s\n", sql);

    strcpy(returnsql, sql);

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) > 1) {
        if (kw.verbose) fprintf(stdout, "The Specified Signal (%s) is Ambiguous ... Please Clarify\n", signal);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Signal (%s) was Found ... Please Clarify\n", signal);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Check User Authorisation

    if (kw.commit &&
        (rc = checkSignalAuthorisation(1, PQgetvalue(DBQuery, 0, 0), clientname, kw.verbose, stdout)) == 1) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - You are Not Authorised to Update Descriptions of Signals %s\n",
                    PQgetvalue(DBQuery, 0, 2));
        kw.commit = 0;
    }

//---------------------------------------------------------------------------------------------------------
// Update the Description

    sprintf(sql, "BEGIN; UPDATE Signal_desc SET description = '%s' WHERE signal_desc_id = %s",
            IDL_STRING_STR(&kw.description),
            PQgetvalue(DBQuery, 0, 0));
    strcat(sql, "; UPDATE Signal_desc SET modified = current_date WHERE signal_desc_id = ");
    strcat(sql, PQgetvalue(DBQuery, 0, 0));
    strcat(sql, "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
    strcat(sql, clientname);
    strcat(sql, "','Signal_Desc',");
    strcat(sql, PQgetvalue(DBQuery, 0, 0));
    strcat(sql, ",'description','");
    strcat(sql, PQgetvalue(DBQuery, 0, 1));
    strcat(sql, "','UPDATE Signal_Desc SET description = \\'");
    strcat(sql, PQgetvalue(DBQuery, 0, 1));
    strcat(sql, "\\' WHERE signal_desc_id = ");
    strcat(sql, PQgetvalue(DBQuery, 0, 0));
    strcat(sql, "'); END;");

    if (kw.debug) {
        fprintf(stdout, "\n%s\n\n", sql);
    }

    if (kw.commit) {
        if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
            if (kw.verbose)
                fprintf(stdout, "ERROR - Signal Status NOT Updated (record %s)\n", PQgetvalue(DBQuery, i, 0));
            PQclear(DBQuery);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);

    return (returnIdamSignalDescSQL(returnsql, kw.verbose, kw.debug, stdout));
}


IDL_VPTR returnIdamSignalDescSQL(char* sql, int verbose, int debug, FILE* fh) {
//
//
// v0.01	November 2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------

    int i, nrows, ncols;

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int signal_desc_id;
        IDL_STRING signal_name;
        IDL_STRING signal_alias;
        IDL_STRING generic_name;
        IDL_STRING signal_class;
        IDL_STRING signal_owner;
        IDL_STRING description;
    } RDESC;

    RDESC* rsql;            // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"SIGNAL_DESC_ID", 0,     (void*) IDL_TYP_LONG},
            {"SIGNAL_NAME",    vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_ALIAS",   vdlen, (void*) IDL_TYP_STRING},
            {"GENERIC_NAME",   vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_CLASS",   vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_OWNER",   vdlen, (void*) IDL_TYP_STRING},
            {"DESCRIPTION",    vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

//-------------------------------------------------------------
// Execute the Passed Query

    DBQuery = NULL;

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (verbose) {
            fprintf(fh, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (verbose) {
            fprintf(fh, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(fh, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RDESC*) malloc(nrows * sizeof(RDESC))) == NULL) {
        if (verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].signal_desc_id = (int) atoi(PQgetvalue(DBQuery, i, 0));

        if (strlen(PQgetvalue(DBQuery, i, 1)) > 0)
            IDL_StrStore(&(rsql[i].description), PQgetvalue(DBQuery, i, 1));
        else
            IDL_StrStore(&(rsql[i].description), "");

        if (strlen(PQgetvalue(DBQuery, i, 2)) > 0)
            IDL_StrStore(&(rsql[i].signal_name), PQgetvalue(DBQuery, i, 2));
        else
            IDL_StrStore(&(rsql[i].signal_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
            IDL_StrStore(&(rsql[i].signal_alias), PQgetvalue(DBQuery, i, 3));
        else
            IDL_StrStore(&(rsql[i].signal_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
            IDL_StrStore(&(rsql[i].generic_name), PQgetvalue(DBQuery, i, 4));
        else
            IDL_StrStore(&(rsql[i].generic_name), "");

        if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
            IDL_StrStore(&(rsql[i].signal_class), PQgetvalue(DBQuery, i, 5));
        else
            IDL_StrStore(&(rsql[i].signal_class), "");

        if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
            IDL_StrStore(&(rsql[i].signal_owner), PQgetvalue(DBQuery, i, 6));
        else
            IDL_StrStore(&(rsql[i].signal_owner), "");

    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);

    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    PQclear(DBQuery);
    return (ivReturn);
}


//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL rosignalclass(int argc, IDL_VPTR argv[], char* argk) {
//
// Change Signal Classifications and Ownership
//-------------------------------------------------------------------------

    int i=0, rc, nrows, wherenum = 0, lstr;

    char sql[MAXSQL], returnsql[MAXSQL];
    char whr[MAXSQL];

    char* signal;
    char sql_signal[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    static int startup = 1;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"ALIAS", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(alias)},
             {"CLASS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_class), IDL_KW_OFFSETOF(class)},
             {"COMMIT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(commit)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"OWNER", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_owner), IDL_KW_OFFSETOF(owner)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.commit = 0;
    kw.alias = 0;
    kw.is_class = 0;
    kw.is_owner = 0;

    signal = NULL;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

    if (kw.debug) fprintf(stdout, "Build Date: %s\n", __DATE__);

    if (argc == 0) {
        if (kw.verbose) fprintf(stdout, "No Arguments Passed: 1 Expected\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if (kw.debug) fprintf(stdout, "%d Arguments Passed\n", argc);

// Passed Arguments: String followed by an Integer (Scalar or Array)

    if (argv[0]->type == IDL_TYP_STRING) {

        IDL_ENSURE_STRING(argv[0]);
        IDL_ENSURE_SCALAR(argv[0]);

        signal = IDL_STRING_STR(&(argv[0]->value.str));                // Signal Name

        if (kw.debug) fprintf(stdout, "Signal Name      : %s\n", signal);
    }

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.commit) fprintf(stdout, "Commit Keyword Passed\n");
        if (kw.alias) fprintf(stdout, "Alias Keyword Passed\n");
        if (kw.is_class) fprintf(stdout, "Class Keyword Passed: %s\n", IDL_STRING_STR(&kw.class));
        if (kw.is_owner) fprintf(stdout, "Owner Keyword Passed: %s\n", IDL_STRING_STR(&kw.owner));
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Signal Name

    if (strlen(signal) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Class Name

    if (kw.is_class && strlen(IDL_STRING_STR(&kw.class)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Class Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Owner Name

    if (kw.is_owner && strlen(IDL_STRING_STR(&kw.owner)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Owner Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//--------------------------------------------------------------------------
// Authentication: Userid

    if (startup) {
        char* uid = NULL;
        if ((uid = getlogin()) != NULL)        // Very Slow!! ... so do once only
            strcpy(clientname, uid);
        else
            userid(clientname);            // Use an even Slower method!
        startup = 0;                // Don't call again!
        if (kw.debug) fprintf(stdout, "Authentication Userid %s\n", clientname);
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Signal Name for Querying

    if (strlen(signal) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Signal name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(signal, sql_signal);
    } else {
        lstr = strlen(signal);
        if (signal[0] == '*' || signal[0] == '?' ||
            (signal[lstr - 2] != '\\' && (signal[lstr - 1] == '*' || signal[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(signal, sql_signal);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_signal, signal);
            strupr(sql_signal);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_signal);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    if (kw.match) {                    // Use Pattern Matching
        if (kw.alias) {                    // RO has Indicated the signal name is an Alias
            strcat(whr, "signal_alias ILIKE '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        } else {
            strcat(whr, "signal_name ILIKE '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        }
        wherenum++;
    } else {
        if (kw.alias) {
            strcat(whr, "upper(signal_alias) = '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        } else {
            strcat(whr, "upper(signal_name) = '");
            strcat(whr, sql_signal);
            strcat(whr, "'");
        }
        wherenum++;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query Signal_Desc Table and Check there is NO Ambiguity in the Sgnal Name

    PQclear(DBQuery);

    strcpy(sql, "SELECT signal_desc_id, description, signal_name, signal_alias, generic_name, "
            "signal_class, signal_owner FROM Signal_Desc WHERE ");
    strcat(sql, whr);

    strcpy(returnsql, sql);

    if (kw.debug)fprintf(stdout, "%s\n", sql);

    if (!kw.is_class && !kw.is_owner) {
        IDL_KW_FREE;
        return (returnIdamSignalDescSQL(returnsql, kw.verbose, kw.debug, stdout));
    }

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) > 1) {
        if (kw.verbose) fprintf(stdout, "The Specified Signal (%s) is Ambiguous ... Please Clarify\n", signal);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    if ((nrows = PQntuples(DBQuery)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Signal (%s) was Found ... Please Clarify\n", signal);
        PQclear(DBQuery);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Check User Authorisation

    if (kw.commit && kw.is_class &&
        (rc = checkSignalAuthorisation(1, PQgetvalue(DBQuery, 0, 0), clientname, kw.verbose, stdout)) == 1) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - You are Not Authorised to Update the Classification of the Signal %s\n",
                    PQgetvalue(DBQuery, 0, 2));
        kw.is_class = 0;
    }

    if (kw.commit && kw.is_owner && (rc = checkAdminAuthorisation(clientname, kw.verbose, stdout)) == 1) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - You are Not Authorised to Update Ownership of the Signal %s\n",
                    PQgetvalue(DBQuery, 0, 2));
        kw.is_owner = 0;
    }

    if (!kw.is_class && !kw.is_owner && kw.commit)kw.commit = 0;

//---------------------------------------------------------------------------------------------------------
// Update the Description

    if (kw.is_class) {
        sprintf(sql, "BEGIN; UPDATE Signal_desc SET signal_class = '%s' WHERE signal_desc_id = %s;",
                IDL_STRING_STR(&kw.class), PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "; UPDATE Signal_desc SET modified = current_date WHERE signal_desc_id = ");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
        strcat(sql, clientname);
        strcat(sql, "','Signal_Desc',");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, ",'signal_class','");
        strcat(sql, PQgetvalue(DBQuery, 0, 5));
        strcat(sql, "','UPDATE Signal_Desc SET signal_class = \\'");
        strcat(sql, PQgetvalue(DBQuery, 0, 5));
        strcat(sql, "\\' WHERE signal_desc_id = ");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "'); END;");

        if (kw.debug)fprintf(stdout, "\n%s\n\n", sql);

        if (kw.commit) {
            if (executeIdamSQL(sql, kw.verbose, stdout) == 1) {
                if (kw.verbose)
                    fprintf(stdout, "ERROR - Signal Status NOT Updated (record %s)\n", PQgetvalue(DBQuery, i, 0));
                PQclear(DBQuery);
                IDL_KW_FREE;
                return (IDL_GettmpLong(1));
            }
        }
    }

    if (kw.is_owner) {
        sprintf(sql, "BEGIN; UPDATE Signal_desc SET signal_owner = '%s' WHERE signal_desc_id = %s;",
                IDL_STRING_STR(&kw.owner), PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "; UPDATE Signal_desc SET modified = current_date WHERE signal_desc_id = ");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "; INSERT INTO Change_Log (username,dbtable,key1,dbfield,prior_value,rollback_sql) VALUES ('");
        strcat(sql, clientname);
        strcat(sql, "','Signal_Desc',");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, ",'signal_owner','");
        strcat(sql, PQgetvalue(DBQuery, 0, 6));
        strcat(sql, "','UPDATE Signal_Desc SET signal_owner = \\'");
        strcat(sql, PQgetvalue(DBQuery, 0, 6));
        strcat(sql, "\\' WHERE signal_desc_id = ");
        strcat(sql, PQgetvalue(DBQuery, 0, 0));
        strcat(sql, "'); END;");

        if (kw.debug)fprintf(stdout, "\n%s\n\n", sql);

        if (kw.commit) {
            if ((rc = executeIdamSQL(sql, kw.verbose, stdout)) == 1) {
                if (kw.verbose)
                    fprintf(stdout, "ERROR - Signal Status NOT Updated (record %s)\n", PQgetvalue(DBQuery, i, 0));
                PQclear(DBQuery);
                IDL_KW_FREE;
                return (IDL_GettmpLong(1));
            }
        }
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);

    return (returnIdamSignalDescSQL(returnsql, kw.verbose, kw.debug, stdout));
}

//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL listsignals(int argc, IDL_VPTR argv[], char* argk) {
//
// List ALL signals available for a specific pulse/pass, class and type
//-------------------------------------------------------------------------

    int i, nrows, ncols, wherenum, lstr;

    char* type, * source;
    char sql[MAXSQL];
    char whr[MAXSQL], whr_str[256];
    char sql_source[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int exp_number;
        int pass;
        int source_status;
        int signal_status;
        IDL_STRING signal_alias;
        IDL_STRING generic_name;
        IDL_STRING type;
        IDL_STRING description;
        IDL_STRING source_alias;
        IDL_STRING signal_class;
        IDL_STRING signal_owner;
        IDL_STRING mds_name;
    } RLIST;

    typedef struct {            // Create the IDL Return Structure
        IDL_STRING signal_alias;
        IDL_STRING generic_name;
        IDL_STRING type;
        IDL_STRING description;
        IDL_STRING source_alias;
        IDL_STRING signal_class;
        IDL_STRING signal_owner;
        IDL_STRING mds_name;
    } DRLIST;

    RLIST* rsql;
    DRLIST* drsql;            // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"EXP_NUMBER",    0,     (void*) IDL_TYP_LONG},
            {"PASS",          0,     (void*) IDL_TYP_LONG},
            {"SOURCE_STATUS", 0,     (void*) IDL_TYP_LONG},
            {"SIGNAL_STATUS", 0,     (void*) IDL_TYP_LONG},
            {"SIGNAL_ALIAS",  vdlen, (void*) IDL_TYP_STRING},
            {"GENERIC_NAME",  vdlen, (void*) IDL_TYP_STRING},
            {"TYPE",          vdlen, (void*) IDL_TYP_STRING},
            {"DESCRIPTION",   vdlen, (void*) IDL_TYP_STRING},
            {"SOURCE_ALIAS",  vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_CLASS",  vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_OWNER",  vdlen, (void*) IDL_TYP_STRING},
            {"MDS_NAME",      vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

    IDL_STRUCT_TAG_DEF dpTags[] = {
            {"SIGNAL_ALIAS", vdlen, (void*) IDL_TYP_STRING},
            {"GENERIC_NAME", vdlen, (void*) IDL_TYP_STRING},
            {"TYPE",         vdlen, (void*) IDL_TYP_STRING},
            {"DESCRIPTION",  vdlen, (void*) IDL_TYP_STRING},
            {"SOURCE_ALIAS", vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_CLASS", vdlen, (void*) IDL_TYP_STRING},
            {"SIGNAL_OWNER", vdlen, (void*) IDL_TYP_STRING},
            {"MDS_NAME",     vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

// Array Keyword Descriptors

    static IDL_KW_ARR_DESC_R expDesc = {IDL_KW_OFFSETOF(shotrange), 1, 2, IDL_KW_OFFSETOF(nshotrange)};
    static IDL_KW_ARR_DESC_R passDesc = {IDL_KW_OFFSETOF(passrange), 1, 2, IDL_KW_OFFSETOF(npassrange)};

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"ALL", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(all)},
             {"CLASS", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_class), IDL_KW_OFFSETOF(class)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"DISTINCT", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(distinct)},
             {"EXP_NUMBER", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(exp_number)},
             {"HUGE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(huge)},
             {"LATEST", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(latest)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"OWNER", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_owner), IDL_KW_OFFSETOF(owner)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"PRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_passrange), IDL_CHARA(passDesc)},
             {"PULNO", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(pulno)},
             {"SEARCH", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_search), IDL_KW_OFFSETOF(search)},
             {"SHOT", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(shot)},
             {"SOURCE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_source), IDL_KW_OFFSETOF(source)},
             {"SRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_shotrange), IDL_CHARA(expDesc)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_exp_number = 0;
    kw.exp_number = 0;
    kw.pulno = 0;
    kw.shot = 0;
    kw.nshotrange = 0;
    kw.shotrange[0] = 0;
    kw.shotrange[1] = 0;
    kw.is_shotrange = 0;

    kw.is_pass = 0;
    kw.is_passrange = 0;
    kw.npassrange = 0;
    kw.passrange[0] = -1;
    kw.passrange[1] = -1;

    kw.is_search = 0;
    kw.is_source = 0;
    kw.is_class = 0;
    kw.is_owner = 0;
    kw.huge = 0;
    kw.all = 0;
    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;
    kw.distinct = 0;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.all) fprintf(stdout, "All Keyword Passed\n");
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.latest) fprintf(stdout, "Latest Keyword Passed\n");
        if (kw.is_search) fprintf(stdout, "Search Keyword Passed: %s\n", IDL_STRING_STR(&kw.search));
        if (kw.is_source) fprintf(stdout, "Source Keyword Passed: %s\n", IDL_STRING_STR(&kw.source));
        if (kw.is_type) fprintf(stdout, "Type Keyword Passed  : %s\n", IDL_STRING_STR(&kw.type));
        if (kw.is_class) fprintf(stdout, "Class Keyword Passed : %s\n", IDL_STRING_STR(&kw.class));
        if (kw.is_owner) fprintf(stdout, "Owner Keyword Passed : %s\n", IDL_STRING_STR(&kw.owner));
        if (kw.is_pass) fprintf(stdout, "Pass Value Passed: %d\n", (int) kw.pass);
        if (kw.is_exp_number) {
            fprintf(stdout, "Exp_Number Value Passed: %d\n", (int) kw.exp_number);
            fprintf(stdout, "Pulno Value Passed     : %d\n", (int) kw.pulno);
            fprintf(stdout, "Shot Value Passed      : %d\n", (int) kw.shot);
        }
        if (kw.is_shotrange) {
            fprintf(stdout, "No. Shot Range Values Passed: %d\n", (int) kw.nshotrange);
            for (i = 0; i < (int) kw.nshotrange; i++) fprintf(stdout, "Range[%d] = %d\n", i, (int) kw.shotrange[i]);
        }
        if (kw.is_passrange) {
            fprintf(stdout, "No. Pass Range Values Passed: %d\n", (int) kw.npassrange);
            for (i = 0; i < (int) kw.npassrange; i++) fprintf(stdout, "PassRange[%d] = %d\n", i, (int) kw.passrange[i]);
        }
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// True Range?

    if (kw.is_shotrange) {
        if (kw.nshotrange != 2) {
            if (kw.verbose) fprintf(stdout, "Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.shotrange[1] < kw.shotrange[0]) {
            if (kw.verbose) fprintf(stdout, "Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Range Limit

    if (kw.is_shotrange && !kw.huge) {
        if ((i = abs(kw.shotrange[1] - kw.shotrange[0])) > LISTRANGELIMIT) {
            if (kw.verbose)
                fprintf(stdout, "Range is Too Large: The limit is %d shots. Please respecify!\n", LISTRANGELIMIT);
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// True Pass Range?

    if (kw.is_passrange) {
        if (kw.npassrange != 2) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.passrange[1] < kw.passrange[0]) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Source Name

    if (kw.is_source && strlen(IDL_STRING_STR(&kw.source)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Source Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Search Text

    if (kw.is_search && strlen(IDL_STRING_STR(&kw.search)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Search Text. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Class Name

    if (kw.is_class && strlen(IDL_STRING_STR(&kw.class)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Class Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// Owner Name

    if (kw.is_owner && strlen(IDL_STRING_STR(&kw.owner)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Signal Owner Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    // Exp_Number

    if (kw.is_exp_number && !kw.is_shotrange) {
        if (kw.exp_number == 0) {
            if (kw.pulno > 0)
                kw.exp_number = kw.pulno;
            else if (kw.shot > 0) kw.exp_number = kw.shot;
        }
    }

    if (!kw.all && !kw.distinct && !kw.is_shotrange && !kw.is_exp_number) {
        if (kw.verbose)
            fprintf(stdout, "Warning - No Exp. Numbers passed and No ALL or DISTINCT Keyword Passed - Assuming ALL.\n");
        kw.all = 1;
    }

    if (!kw.all && kw.distinct && !kw.is_shotrange && !kw.is_exp_number) {
        if (kw.verbose)
            fprintf(stdout, "No Exp. Numbers passed with DISTINCT Keyword - Please specify an Exp. Number or Range \n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (kw.debug) fprintf(stdout, "DBConnect is NULL: Try to connect.\n");
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Source Name for Querying

    source = IDL_STRING_STR(&kw.source);

    if (strlen(source) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Source name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(source, sql_source);
    } else {
        lstr = strlen(source);
        if (source[0] == '*' || source[0] == '?' ||
            (source[lstr - 2] != '\\' && (source[lstr - 1] == '*' || source[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(source, sql_source);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_source, source);
            strupr(sql_source);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_source);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    type = IDL_STRING_STR(&kw.type);

    if (!kw.all) {
        if (kw.is_shotrange) {                // Start of range
            if (wherenum > 0) strcat(whr, " AND ");
            sprintf(whr_str, "exp_number >= %d AND exp_number <= %d", (int) kw.shotrange[0], (int) kw.shotrange[1]);
            strcat(whr, whr_str);
            wherenum++;
        } else {
            if (kw.is_exp_number) {
                if (wherenum > 0) strcat(whr, " AND ");
                sprintf(whr_str, "exp_number = %d", (int) kw.exp_number);
                strcat(whr, whr_str);
                wherenum++;
            }
        }

        if (!kw.latest) {
            if (kw.is_passrange && type[0] != 'R' && type[0] != 'r' && kw.passrange[0] >= 0) {        // Not RAW Data
                if (wherenum > 0) strcat(whr, " AND ");
                sprintf(whr_str, "pass >= %d AND pass <= %d", (int) kw.passrange[0], (int) kw.passrange[1]);
                strcat(whr, whr_str);
                wherenum++;
            } else {
                if (kw.is_pass && type[0] != 'R' && type[0] != 'r' && kw.pass >= 0) {            // Not RAW Data
                    if (wherenum > 0) strcat(whr, " AND ");
                    sprintf(whr_str, "pass = %d", (int) kw.pass);
                    strcat(whr, whr_str);
                    wherenum++;
                }
            }
        }
    }

    if (kw.is_source) {
        //if(kw.all) strcat(whr," WHERE ");
        if (wherenum > 0) strcat(whr, " AND ");
        if (kw.match) {
            strcat(whr, "source_alias ILIKE '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        } else {
            strcat(whr, "upper(source_alias) = '");
            strcat(whr, sql_source);
            strcat(whr, "'");
        }
        wherenum++;
    }

    if (kw.is_type) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        //else
        //   if(kw.all) strcat(whr," WHERE ");
        strcat(whr, "type='");
        strcat(whr, IDL_STRING_STR(&kw.type));
        strcat(whr, "'");
        wherenum++;
    }

    if (kw.is_search) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        //else
        //   if(kw.all) strcat(whr," WHERE ");
        strcat(whr, "(description ILIKE '%");
        strcat(whr, IDL_STRING_STR(&kw.search));
        strcat(whr, "%' OR ");
        strcat(whr, "signal_alias ILIKE '%");
        strcat(whr, IDL_STRING_STR(&kw.search));
        strcat(whr, "%')");
        wherenum++;
    }

    if (kw.is_class) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        //else
        //   if(kw.all) strcat(whr," WHERE ");
        strcat(whr, "signal_class ILIKE '");
        strcat(whr, IDL_STRING_STR(&kw.class));
        strcat(whr, "'");
        wherenum++;
    }

    if (kw.is_owner) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        //else
        //   if(kw.all) strcat(whr," WHERE ");
        strcat(whr, "signal_owner ILIKE '");
        strcat(whr, IDL_STRING_STR(&kw.owner));
        strcat(whr, "'");
        wherenum++;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query the Database

    if (!kw.all && !kw.distinct) {
        strcpy(sql,
               "SELECT DISTINCT * FROM (SELECT signal_desc_id, exp_number, pass, signal_alias, generic_name, type, description, "
                       "signal_status, source_status, source_alias, signal_class, signal_owner, mds_name FROM Signal,(SELECT signal_desc_id as desc_id, "
                       "signal_alias, generic_name, description, signal_class, signal_owner, mds_name FROM signal_desc");
        if (kw.is_type) {
            strcat(sql, " WHERE type='");
            strcat(sql, IDL_STRING_STR(&kw.type));
            strcat(sql, "'");
        }
        if (kw.latest || (!kw.is_passrange && kw.is_pass && (int) kw.pass == -1)) {
            strcat(sql, ") as A, ");
            strcat(sql,
                   "(SELECT source_id as src_key, exp_number, pass, source_status, source_alias, type FROM data_source, ");
            strcat(sql,
                   "(SELECT src_id FROM (SELECT source_id as src_id, filename as fname, pass as passno FROM Data_Source WHERE ");
            strcat(sql, whr);
            strcat(sql, ") as X, (SELECT filename, max(pass) as mpass FROM Data_Source WHERE ");
            strcat(sql, whr);
            strcat(sql,
                   " GROUP BY filename) as Y WHERE X.fname = Y.filename AND X.passno = Y.mpass) as Z WHERE Z.src_id = data_source.source_id");
            strcat(sql, ") as B WHERE signal.source_id=B.src_key AND Signal.signal_desc_id=A.desc_id) as C "
                    "ORDER BY signal_alias ASC, exp_number ASC, pass ASC");
        } else {
            strcat(sql,
                   ") as A, (SELECT source_id as src_id, exp_number, pass, type, source_status, source_alias FROM data_source WHERE ");
            strcat(sql, whr);
            strcat(sql, ") as B WHERE signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C "
                    "ORDER BY signal_alias ASC, exp_number ASC, pass ASC");
        }
    } else {
        if (kw.distinct && !kw.all) {
            strcpy(sql, "SELECT DISTINCT * FROM (SELECT signal_alias, generic_name, type, "
                    "description, source_alias, signal_class, signal_owner, mds_name FROM Signal,(SELECT signal_desc_id as desc_id, "
                    "signal_alias, generic_name, description, signal_class, signal_owner, mds_name FROM signal_desc");
            if (kw.is_type) {
                strcat(sql, " WHERE type='");
                strcat(sql, IDL_STRING_STR(&kw.type));
                strcat(sql, "'");
            }
            if (kw.latest) {
                strcat(sql, ") as A, ");
                strcat(sql,
                       "(SELECT source_id as src_key, exp_number, pass, source_status, source_alias, type FROM data_source, ");
                strcat(sql,
                       "(SELECT src_id FROM (SELECT source_id as src_id, filename as fname, pass as passno FROM Data_Source WHERE ");
                strcat(sql, whr);
                strcat(sql, ") as X, (SELECT filename, max(pass) as mpass FROM Data_Source WHERE ");
                strcat(sql, whr);
                strcat(sql,
                       " GROUP BY filename) as Y WHERE X.fname = Y.filename AND X.passno = Y.mpass) as Z WHERE Z.src_id = data_source.source_id");
                strcat(sql, ") as B WHERE signal.source_id=B.src_key AND Signal.signal_desc_id=A.desc_id) as C "
                        "ORDER BY signal_alias ASC");
            } else {
                strcat(sql,
                       ") as A, (SELECT source_id as src_id, exp_number, pass, type, source_status, source_alias FROM data_source WHERE ");
                strcat(sql, whr);
                strcat(sql, ") as B WHERE signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C "
                        "ORDER BY signal_alias ASC");
            }
        } else {
            strcpy(sql,
                   "SELECT signal_alias, generic_name, type, description, source_alias, signal_class, signal_owner, mds_name FROM signal_desc");
            if (wherenum > 0) {
                strcat(sql, " WHERE ");
                strcat(sql, whr);
            }
            strcat(sql, " ORDER BY signal_alias ASC");
        }
    }

/*
   if(!kw.all && !kw.distinct){
      strcpy(sql,"SELECT DISTINCT * FROM (SELECT signal_desc_id, exp_number, pass, signal_alias, generic_name, type, description, "
                 "signal_status, source_status, source_alias, signal_class, signal_owner FROM Signal,(SELECT signal_desc_id as desc_id, "
		 "signal_alias, generic_name, description, signal_class, signal_owner FROM signal_desc");
      if(kw.is_type){
         strcat(sql," WHERE type='");
         strcat(sql,IDL_STRING_STR(&kw.type));
         strcat(sql,"'");
      }
      if(kw.latest || (!kw.is_passrange && kw.is_pass && (int)kw.pass == -1)){
         strcat(sql,") as A, (SELECT * FROM Data_Source, (SELECT source_id as src_id, type, source_status, "
                    "source_alias FROM data_source WHERE ");
         strcat(sql,whr);
         strcat(sql,") as B1, (SELECT source_id, max(pass) as mpass FROM Data_Source WHERE ");
         strcat(sql,whr);
         strcat(sql," GROUP BY source_id) as B2 WHERE B1.src_id = B2.source_id");
      } else {
         strcat(sql,") as A, (SELECT source_id as src_id, exp_number, pass, type, source_status, source_alias FROM data_source WHERE ");
         strcat(sql,whr);
      }
      strcat(sql,") as B WHERE signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C "
                 "ORDER BY signal_alias ASC, exp_number ASC, pass ASC");
   } else {
      if(kw.distinct){
         strcpy(sql,"SELECT DISTINCT * FROM (SELECT signal_alias, generic_name, type, "
	            "description, source_alias, signal_class, signal_owner FROM Signal,(SELECT signal_desc_id as desc_id, "
		    "signal_alias, generic_name, description, signal_class, signal_owner FROM signal_desc");
         if(kw.is_type){
            strcat(sql," WHERE type='");
            strcat(sql,IDL_STRING_STR(&kw.type));
            strcat(sql,"'");
         }
         if(kw.latest){
            strcat(sql,") as A, (SELECT * FROM Data_Source, (SELECT source_id as src_id, type, source_status, "
                       "source_alias FROM data_source WHERE ");
            strcat(sql,whr);
            strcat(sql,") as B1, (SELECT source_id, max(pass) as mpass FROM Data_Source WHERE ");
            strcat(sql,whr);
            strcat(sql," GROUP BY source_id) as B2 WHERE B1.src_id = B2.source_id");
         } else {
            strcat(sql,") as A, (SELECT source_id as src_id, exp_number, pass, type, source_status, source_alias FROM data_source WHERE ");
            strcat(sql,whr);
         }
         strcat(sql,") as B WHERE signal.source_id=B.src_id AND Signal.signal_desc_id=A.desc_id) as C "
                    "ORDER BY signal_alias ASC");
      } else {
         strcpy(sql,"SELECT signal_alias, generic_name, type, description, source_alias, signal_class, signal_owner FROM signal_desc");
         if(strcat(sql,whr);
         strcat(sql," ORDER BY signal_alias ASC");
      }
   }
*/

    if (kw.debug)fprintf(stdout, "%s\n", sql);
//return(IDL_GettmpLong(1));


//-------------------------------------------------------------
// Execute the Passed Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (!kw.huge && nrows > LISTROWLIMIT) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - Too many Records (%d) Returned by the Query. The limit is %d", nrows,
                    LISTROWLIMIT);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if (!kw.all && !kw.distinct) {
        if ((rsql = (RLIST*) malloc(nrows * sizeof(RLIST))) == NULL) {
            if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }
    } else {
        if ((drsql = (DRLIST*) malloc(nrows * sizeof(DRLIST))) == NULL) {
            if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
            IDL_KW_FREE;
            PQclear(DBQuery);
            return (IDL_GettmpLong(1));
        }
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        if (!kw.all && !kw.distinct) {
            rsql[i].exp_number = (int) atoi(PQgetvalue(DBQuery, i, 1));
            rsql[i].pass = (int) atoi(PQgetvalue(DBQuery, i, 2));
            rsql[i].signal_status = (int) atoi(PQgetvalue(DBQuery, i, 7));
            rsql[i].source_status = (int) atoi(PQgetvalue(DBQuery, i, 8));

            if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
                IDL_StrStore(&(rsql[i].signal_alias), PQgetvalue(DBQuery, i, 3));
            else
                IDL_StrStore(&(rsql[i].signal_alias), "");

            if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
                IDL_StrStore(&(rsql[i].generic_name), PQgetvalue(DBQuery, i, 4));
            else
                IDL_StrStore(&(rsql[i].generic_name), "");

            if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
                IDL_StrStore(&(rsql[i].type), PQgetvalue(DBQuery, i, 5));
            else
                IDL_StrStore(&(rsql[i].type), "");

            if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
                IDL_StrStore(&(rsql[i].description), PQgetvalue(DBQuery, i, 6));
            else
                IDL_StrStore(&(rsql[i].description), "");

            if (strlen(PQgetvalue(DBQuery, i, 9)) > 0)
                IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 9));
            else
                IDL_StrStore(&(rsql[i].source_alias), "");

            if (strlen(PQgetvalue(DBQuery, i, 10)) > 0)
                IDL_StrStore(&(rsql[i].signal_class), PQgetvalue(DBQuery, i, 10));
            else
                IDL_StrStore(&(rsql[i].signal_class), "");

            if (strlen(PQgetvalue(DBQuery, i, 11)) > 0)
                IDL_StrStore(&(rsql[i].signal_owner), PQgetvalue(DBQuery, i, 11));
            else
                IDL_StrStore(&(rsql[i].signal_owner), "");

            if (strlen(PQgetvalue(DBQuery, i, 12)) > 0)
                IDL_StrStore(&(rsql[i].mds_name), PQgetvalue(DBQuery, i, 12));
            else
                IDL_StrStore(&(rsql[i].mds_name), "");

        } else {
            if (strlen(PQgetvalue(DBQuery, i, 0)) > 0)
                IDL_StrStore(&(drsql[i].signal_alias), PQgetvalue(DBQuery, i, 0));
            else
                IDL_StrStore(&(drsql[i].signal_alias), "");

            if (strlen(PQgetvalue(DBQuery, i, 1)) > 0)
                IDL_StrStore(&(drsql[i].generic_name), PQgetvalue(DBQuery, i, 1));
            else
                IDL_StrStore(&(drsql[i].generic_name), "");

            if (strlen(PQgetvalue(DBQuery, i, 2)) > 0)
                IDL_StrStore(&(drsql[i].type), PQgetvalue(DBQuery, i, 2));
            else
                IDL_StrStore(&(drsql[i].type), "");

            if (strlen(PQgetvalue(DBQuery, i, 3)) > 0)
                IDL_StrStore(&(drsql[i].description), PQgetvalue(DBQuery, i, 3));
            else
                IDL_StrStore(&(drsql[i].description), "");

            if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
                IDL_StrStore(&(drsql[i].source_alias), PQgetvalue(DBQuery, i, 4));
            else
                IDL_StrStore(&(drsql[i].source_alias), "");

            if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
                IDL_StrStore(&(drsql[i].signal_class), PQgetvalue(DBQuery, i, 5));
            else
                IDL_StrStore(&(drsql[i].signal_class), "");

            if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
                IDL_StrStore(&(drsql[i].signal_owner), PQgetvalue(DBQuery, i, 6));
            else
                IDL_StrStore(&(drsql[i].signal_owner), "");

            if (strlen(PQgetvalue(DBQuery, i, 7)) > 0)
                IDL_StrStore(&(drsql[i].mds_name), PQgetvalue(DBQuery, i, 7));
            else
                IDL_StrStore(&(drsql[i].mds_name), "");

        }
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    if (!kw.all && !kw.distinct) {
        psDef = IDL_MakeStruct(NULL, pTags);
        ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);
    } else {
        psDef = IDL_MakeStruct(NULL, dpTags);
        ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) drsql, freeMem, psDef);
    }

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (ivReturn);
}

//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL listsources(int argc, IDL_VPTR argv[], char* argk) {
//
// List ALL data source files available for a specific pulse/pass and type
//-------------------------------------------------------------------------

    int i, nrows, ncols, wherenum, lstr;

    char* type, * source;
    char sql[MAXSQL];
    char whr[MAXSQL], whr_str[256];
    char sql_source[MAXNAME];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int source_id;
        int run_id;
        int exp_number;
        int pass;
        int source_status;
        IDL_STRING type;
        IDL_STRING source_alias;
        IDL_STRING format;
        IDL_STRING path;
        IDL_STRING filename;
        IDL_STRING server;
    } RLIST;

    RLIST* rsql;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"SOURCE_ID",     0,     (void*) IDL_TYP_LONG},
            {"RUN_ID",        0,     (void*) IDL_TYP_LONG},
            {"EXP_NUMBER",    0,     (void*) IDL_TYP_LONG},
            {"PASS",          0,     (void*) IDL_TYP_LONG},
            {"SOURCE_STATUS", 0,     (void*) IDL_TYP_LONG},
            {"TYPE",          vdlen, (void*) IDL_TYP_STRING},
            {"SOURCE_ALIAS",  vdlen, (void*) IDL_TYP_STRING},
            {"FORMAT",        vdlen, (void*) IDL_TYP_STRING},
            {"PATH",          vdlen, (void*) IDL_TYP_STRING},
            {"FILENAME",      vdlen, (void*) IDL_TYP_STRING},
            {"SERVER",        vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

// Array Keyword Descriptors

    static IDL_KW_ARR_DESC_R expDesc = {IDL_KW_OFFSETOF(shotrange), 1, 2, IDL_KW_OFFSETOF(nshotrange)};
    static IDL_KW_ARR_DESC_R passDesc = {IDL_KW_OFFSETOF(passrange), 1, 2, IDL_KW_OFFSETOF(npassrange)};

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"EXP_NUMBER", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(exp_number)},
             {"LATEST", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(latest)},
             {"MATCH", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(match)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"PRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_passrange), IDL_CHARA(passDesc)},
             {"PULNO", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(pulno)},
             {"SHOT", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_exp_number), IDL_KW_OFFSETOF(shot)},
             {"SOURCE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_source), IDL_KW_OFFSETOF(source)},
             {"SRANGE", IDL_TYP_LONG, 1, IDL_KW_ARRAY, IDL_KW_OFFSETOF(is_shotrange), IDL_CHARA(expDesc)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_exp_number = 0;
    kw.exp_number = 0;
    kw.pulno = 0;
    kw.shot = 0;
    kw.nshotrange = 0;
    kw.shotrange[0] = 0;
    kw.shotrange[1] = 0;
    kw.is_shotrange = 0;

    kw.is_pass = 0;
    kw.is_passrange = 0;
    kw.npassrange = 0;
    kw.passrange[0] = -1;
    kw.passrange[1] = -1;

    kw.is_source = 0;
    kw.is_type = 0;
    kw.match = 0;
    kw.verbose = 0;
    kw.debug = 0;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, "Build Date: %s\n", __DATE__);
        if (kw.match) fprintf(stdout, "Match Keyword Passed\n");
        if (kw.latest) fprintf(stdout, "Latest Keyword Passed\n");
        if (kw.is_source) fprintf(stdout, "Source Keyword Passed: %s\n", IDL_STRING_STR(&kw.source));
        if (kw.is_type) fprintf(stdout, "Type Keyword Passed  : %s\n", IDL_STRING_STR(&kw.type));
        if (kw.is_pass) fprintf(stdout, "Pass Value Passed: %d\n", (int) kw.pass);
        if (kw.is_exp_number) {
            fprintf(stdout, "Exp_Number Value Passed: %d\n", (int) kw.exp_number);
            fprintf(stdout, "Pulno Value Passed     : %d\n", (int) kw.pulno);
            fprintf(stdout, "Shot Value Passed      : %d\n", (int) kw.shot);
        }
        if (kw.is_shotrange) {
            fprintf(stdout, "No. Shot Range Values Passed: %d\n", (int) kw.nshotrange);
            for (i = 0; i < (int) kw.nshotrange; i++) fprintf(stdout, "Range[%d] = %d\n", i, (int) kw.shotrange[i]);
        }
        if (kw.is_passrange) {
            fprintf(stdout, "No. Pass Range Values Passed: %d\n", (int) kw.npassrange);
            for (i = 0; i < (int) kw.npassrange; i++) fprintf(stdout, "PassRange[%d] = %d\n", i, (int) kw.passrange[i]);
        }
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// True Range?

    if (kw.is_shotrange) {
        if (kw.nshotrange != 2) {
            if (kw.verbose) fprintf(stdout, "Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.shotrange[1] < kw.shotrange[0]) {
            if (kw.verbose) fprintf(stdout, "Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// True Pass Range?

    if (kw.is_passrange) {
        if (kw.npassrange != 2) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not a 2 Element Array! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.passrange[1] < kw.passrange[0]) {
            if (kw.verbose) fprintf(stdout, "Pass Number Range is Not in Ascending Order! Please correct!\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
    }

// Source Name

    if (kw.is_source && strlen(IDL_STRING_STR(&kw.source)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Source Name. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

    // Exp_Number

    if (kw.is_exp_number && !kw.is_shotrange) {
        if (kw.exp_number == 0) {
            if (kw.pulno > 0)
                kw.exp_number = kw.pulno;
            else if (kw.shot > 0) kw.exp_number = kw.shot;
        }
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Pattern Matching

// Prepare Source Name for Querying

    source = IDL_STRING_STR(&kw.source);

    if (strlen(source) >= MAXNAME - 2) {
        if (kw.verbose) fprintf(stdout, "Source name is too Long [%d]\n", MAXNAME - 2);
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

// If user specifies MATCH Keyword explicitly then use ILIKE pattern match
// Otherwise check for Standard pattern matching characters ? and *. If found then enable pattern matching.

    if (kw.match) {        // escape any SQL pattern matching characters: _ and %
        sqlIdamPatternMatch(source, sql_source);
    } else {
        lstr = strlen(source);
        if (source[0] == '*' || source[0] == '?' ||
            (source[lstr - 2] != '\\' && (source[lstr - 1] == '*' || source[lstr - 1] == '?'))) {
            sqlIdamPatternMatch(source, sql_source);
            kw.match = 1;                    // Flag Pattern Matching to be used
        } else {
            strcpy(sql_source, source);
            strupr(sql_source);        // If Pattern Matching is Not Specified then convert case
        }
    }
    TrimString(sql_source);

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    type = IDL_STRING_STR(&kw.type);

    if (kw.is_shotrange) {                // Start of range
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        sprintf(whr_str, "exp_number >= %d AND exp_number <= %d", (int) kw.shotrange[0], (int) kw.shotrange[1]);
        strcat(whr, whr_str);
        wherenum++;
    } else {
        if (kw.is_exp_number) {
            if (wherenum > 0)
                strcat(whr, " AND ");
            else
                strcat(whr, " WHERE ");
            sprintf(whr_str, "exp_number = %d", (int) kw.exp_number);
            strcat(whr, whr_str);
            wherenum++;
        }
    }

    if (!kw.latest) {
        if (kw.is_passrange && type[0] != 'R' && type[0] != 'r' && kw.passrange[0] >= 0) {        // Not RAW Data
            if (wherenum > 0)
                strcat(whr, " AND ");
            else
                strcat(whr, " WHERE ");
            sprintf(whr_str, "pass >= %d AND pass <= %d", (int) kw.passrange[0], (int) kw.passrange[1]);
            strcat(whr, whr_str);
            wherenum++;
        } else {
            if (kw.is_pass && type[0] != 'R' && type[0] != 'r' && kw.pass >= 0) {            // Not RAW Data
                if (wherenum > 0)
                    strcat(whr, " AND ");
                else
                    strcat(whr, " WHERE ");
                sprintf(whr_str, "pass = %d", (int) kw.pass);
                strcat(whr, whr_str);
                wherenum++;
            }
        }
    }

    if (kw.is_source) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        if (strlen(source) <= 3) {        // Assume an ALIAS name form is passed
            if (kw.match) {
                strcat(whr, "source_alias ILIKE '");
                strcat(whr, sql_source);
                strcat(whr, "'");
            } else {
                strcat(whr, "upper(source_alias) = '");
                strcat(whr, sql_source);
                strcat(whr, "'");
            }
            wherenum++;
        } else {
            if (strlen(sql_source) > 0) {
                if (kw.match) {
                    strcat(whr, "filename ILIKE '");        // otherwise a regular name form is passed
                    strcat(whr, sql_source);
                    strcat(whr, "'");
                } else {
                    strcat(whr, "upper(filename) = '");
                    strcat(whr, sql_source);
                    strcat(whr, "'");
                }
                wherenum++;
            }
        }
    }

    if (kw.is_type) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        strcat(whr, "type='");
        strcat(whr, IDL_STRING_STR(&kw.type));
        strcat(whr, "'");
        wherenum++;
    }


//-----------------------------------------------------------------------------------------------------------------------------
// Check there is some selection criteria in play

    if (strlen(whr) == 0) {
        if (kw.verbose) fprintf(stdout, "No Selection Criteria have been given. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query the Database

    strcpy(sql,
           "SELECT source_id as src_key, exp_number, pass, source_status, source_alias, type, format, path, filename, "
                   "server, run_id FROM data_source ");

    if (kw.latest) {
        strcat(sql,
               ",(SELECT src_id FROM (SELECT source_id as src_id, exp_number as enumber, pass as passno FROM Data_Source ");
        strcat(sql, whr);
        strcat(sql, ") as X, (SELECT exp_number, max(pass) as mpass FROM Data_Source ");
        strcat(sql, whr);
        strcat(sql,
               " GROUP BY exp_number) as Y WHERE X.enumber = Y.exp_number AND X.passno = Y.mpass) as Z WHERE Z.src_id = data_source.source_id");
    } else
        strcat(sql, whr);

    strcat(sql, " ORDER BY filename, pass ASC");

    if (kw.debug)fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Passed Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (nrows > LISTROWLIMIT) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - Too many Records (%d) Returned by the Query. The limit is %d", nrows,
                    LISTROWLIMIT);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RLIST*) malloc(nrows * sizeof(RLIST))) == NULL) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }


// Populate the Structure Array

    for (i = 0; i < nrows; i++) {

        rsql[i].source_id = (int) atoi(PQgetvalue(DBQuery, i, 0));
        rsql[i].exp_number = (int) atoi(PQgetvalue(DBQuery, i, 1));
        rsql[i].pass = (int) atoi(PQgetvalue(DBQuery, i, 2));
        rsql[i].source_status = (int) atoi(PQgetvalue(DBQuery, i, 3));

        if (strlen(PQgetvalue(DBQuery, i, 4)) > 0)
            IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 4));
        else
            IDL_StrStore(&(rsql[i].source_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 5)) > 0)
            IDL_StrStore(&(rsql[i].type), PQgetvalue(DBQuery, i, 5));
        else
            IDL_StrStore(&(rsql[i].type), "");

        if (strlen(PQgetvalue(DBQuery, i, 6)) > 0)
            IDL_StrStore(&(rsql[i].format), PQgetvalue(DBQuery, i, 6));
        else
            IDL_StrStore(&(rsql[i].format), "");

        if (strlen(PQgetvalue(DBQuery, i, 7)) > 0)
            IDL_StrStore(&(rsql[i].path), PQgetvalue(DBQuery, i, 7));
        else
            IDL_StrStore(&(rsql[i].path), "");

        if (strlen(PQgetvalue(DBQuery, i, 8)) > 0)
            IDL_StrStore(&(rsql[i].filename), PQgetvalue(DBQuery, i, 8));
        else
            IDL_StrStore(&(rsql[i].filename), "");

        if (strlen(PQgetvalue(DBQuery, i, 9)) > 0)
            IDL_StrStore(&(rsql[i].server), PQgetvalue(DBQuery, i, 9));
        else
            IDL_StrStore(&(rsql[i].server), "");

        if (strlen(PQgetvalue(DBQuery, i, 10)) > 0)
            rsql[i].run_id = (int) atoi(PQgetvalue(DBQuery, i, 10));
        else
            rsql[i].run_id = 0;
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);
    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (ivReturn);
}

//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL listallsources(int argc, IDL_VPTR argv[], char* argk) {
//
// List ALL data source files available for a specific type
//-------------------------------------------------------------------------

    int i, nrows;

    char sql[MAXSQL];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        IDL_STRING type;
        IDL_STRING source_alias;
    } RLIST;

    RLIST* rsql;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"TYPE",         vdlen, (void*) IDL_TYP_STRING},
            {"SOURCE_ALIAS", vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.verbose = 0;
    kw.debug = 0;
    kw.is_type = 0;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        fprintf(stdout, __DATE__);
        if (kw.is_type) fprintf(stdout, "Type Keyword Passed  : %s\n", IDL_STRING_STR(&kw.type));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL

    if (kw.is_type) {
        sprintf(sql, "SELECT DISTINCT source_alias, type FROM data_source WHERE type='%s' ORDER BY type, source_alias;",
                IDL_STRING_STR(&kw.type));
    } else {
        sprintf(sql, "SELECT DISTINCT source_alias, type FROM data_source ORDER BY type, source_alias;");
    }

    if (kw.debug)fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Passed Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows

    if (nrows == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (nrows > LISTROWLIMIT) {
        if (kw.verbose)
            fprintf(stdout, "ERROR - Too many Records (%d) Returned by the Query. The limit is %d", nrows,
                    LISTROWLIMIT);
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RLIST*) malloc(nrows * sizeof(RLIST))) == NULL) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {

        if (strlen(PQgetvalue(DBQuery, i, 0)) > 0)
            IDL_StrStore(&(rsql[i].source_alias), PQgetvalue(DBQuery, i, 0));
        else
            IDL_StrStore(&(rsql[i].source_alias), "");

        if (strlen(PQgetvalue(DBQuery, i, 1)) > 0)
            IDL_StrStore(&(rsql[i].type), PQgetvalue(DBQuery, i, 1));
        else
            IDL_StrStore(&(rsql[i].type), "");
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);
    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (ivReturn);
}



//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL listreasons(int argc, IDL_VPTR argv[], char* argk) {
//
// List Status Change Reasons
//-------------------------------------------------------------------------

    int i, nrows, ncols, wherenum;

    char sql[MAXSQL];
    char whr[MAXSQL];
    char whr_str[56];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int code;
        IDL_STRING description;
    } RLIST;

    RLIST* rsql;                // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"CODE",        0,     (void*) IDL_TYP_LONG},
            {"DESCRIPTION", vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"CODE", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_code), IDL_KW_OFFSETOF(code)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"SEARCH", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_search), IDL_KW_OFFSETOF(search)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_search = 0;
    kw.is_code = 0;
    kw.verbose = 0;
    kw.debug = 0;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

    if (kw.debug) fprintf(stdout, "Build Date: %s\n", __DATE__);

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        if (kw.is_search) fprintf(stdout, "Search Keyword Passed: %s\n", IDL_STRING_STR(&kw.search));
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Search Text

    if (kw.is_search && strlen(IDL_STRING_STR(&kw.search)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Search Text. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    if (kw.is_search) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        strcat(whr, "description ILIKE '%");
        strcat(whr, IDL_STRING_STR(&kw.search));
        strcat(whr, "%'");
        wherenum++;
    }

    if (kw.is_code) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        sprintf(whr_str, "reason_code = %d", (int) kw.code);
        strcat(whr, whr_str);
        wherenum++;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query the Database

    strcpy(sql, "SELECT reason_code, description FROM status_reason");
    strcat(sql, whr);

    if (kw.debug)fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Passed Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (kw.is_search && nrows > 1) {
        if (kw.verbose)
            IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Too Many Reason Codes Found. Please Clarify");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RLIST*) malloc(nrows * sizeof(RLIST))) == NULL) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].code = (int) atoi(PQgetvalue(DBQuery, i, 0));
        if (strlen(PQgetvalue(DBQuery, i, 1)) > 0)
            IDL_StrStore(&(rsql[i].description), PQgetvalue(DBQuery, i, 1));
        else
            IDL_StrStore(&(rsql[i].description), "");
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);
    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (ivReturn);
}

//=================================================================================================
//----------------------------------------------------------------------------

IDL_VPTR IDL_CDECL listimpacts(int argc, IDL_VPTR argv[], char* argk) {
//
// List Status Change Impact Codes
//-------------------------------------------------------------------------

    int i, nrows, ncols, wherenum;

    char sql[MAXSQL];
    char whr[MAXSQL];
    char whr_str[56];

    PGresult* DBQuery = NULL;
    ExecStatusType DBQueryStatus;

    typedef struct {            // Create the IDL Return Structure
        int code;
        IDL_STRING description;
    } RLIST;

    RLIST* rsql;                // Pointer to the Returned IDL/C Structure Array;
    IDL_VPTR ivReturn = NULL;

    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //static IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    IDL_MEMINT vdlen[] = {1, 1};        // Scalar Value Array
    //static IDL_LONG vdlen[] = {1,1};		// Scalar Value Array

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"CODE",        0,     (void*) IDL_TYP_LONG},
            {"DESCRIPTION", vdlen, (void*) IDL_TYP_STRING},
            {0}
    };

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"CODE", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_code), IDL_KW_OFFSETOF(code)},
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"SEARCH", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_search), IDL_KW_OFFSETOF(search)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.is_search = 0;
    kw.is_code = 0;
    kw.verbose = 0;
    kw.debug = 0;

// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//---------------------------------------------------------------------------
// All Arguments & keywords
//---------------------------------------------------------------------------
// Identify Arguments

    if (kw.debug) fprintf(stdout, "Build Date: %s\n", __DATE__);

//-----------------------------------------------------------------------
// Debug Dump of Keywords

    if (kw.debug) {
        if (kw.is_search) fprintf(stdout, "Search Keyword Passed: %s\n", IDL_STRING_STR(&kw.search));
    }

//--------------------------------------------------------------------------
// Test Passed Parameters

// Search Text

    if (kw.is_search && strlen(IDL_STRING_STR(&kw.search)) == 0) {
        if (kw.verbose) fprintf(stdout, "No Passed Search Text. Please check!\n");
        IDL_KW_FREE;
        return (IDL_GettmpLong(1));
    }

//--------------------------------------------------------------------------
// Assign IDAM Server Library Globals

    dbgout = stdout;
    errout = stdout;
    logout = stdout;

//--------------------------------------------------------------------------
// Connect to Database

    if (DBConnect == NULL) {
        if (!(DBConnect = (PGconn*) startSQL())) {
            if (kw.verbose) fprintf(stdout, "IDAM Database Server Connect Error\n");
            IDL_KW_FREE;
            return (IDL_GettmpLong(1));
        }
        if (kw.debug) fprintf(stdout, "IDAM Database Connection Made\n");
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Build SQL WHERE Clause to Identify the Signal

    whr[0] = '\0';
    wherenum = 0;

    if (kw.is_search) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        strcat(whr, "description ILIKE '%");
        strcat(whr, IDL_STRING_STR(&kw.search));
        strcat(whr, "%'");
        wherenum++;
    }

    if (kw.is_code) {
        if (wherenum > 0)
            strcat(whr, " AND ");
        else
            strcat(whr, " WHERE ");
        sprintf(whr_str, "impact_code = %d", (int) kw.code);
        strcat(whr, whr_str);
        wherenum++;
    }

//-----------------------------------------------------------------------------------------------------------------------------
// Query the Database

    strcpy(sql, "SELECT impact_code, description FROM status_impact");
    strcat(sql, whr);

    if (kw.debug)fprintf(stdout, "%s\n", sql);

//-------------------------------------------------------------
// Execute the Passed Query

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Failure to Execute the Query: %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    DBQueryStatus = PQresultStatus(DBQuery);

    if (DBQueryStatus != PGRES_TUPLES_OK) {
        if (kw.verbose) {
            fprintf(stdout, "ERROR - Query Incorrectly Processed %s\n", sql);
            fprintf(stdout, "ERROR - %s\n", PQresultErrorMessage(DBQuery));
        }
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

//-------------------------------------------------------------
// Extract the Resultset

    nrows = PQntuples(DBQuery);        // Number of Rows
    ncols = PQnfields(DBQuery);        // Number of Columns

    if (nrows == 0 || ncols == 0) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - No Results Returned by the Query");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

    if (kw.is_search && nrows > 1) {
        if (kw.verbose)
            IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Too Many Impact Codes Found. Please Clarify");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Allocate the Appropriate Return Structure Array

    if ((rsql = (RLIST*) malloc(nrows * sizeof(RLIST))) == NULL) {
        if (kw.verbose)IDL_Message(IDL_M_NAMED_GENERIC, IDL_MSG_INFO, "ERROR - Unable to Allocate Heap!");
        IDL_KW_FREE;
        PQclear(DBQuery);
        return (IDL_GettmpLong(1));
    }

// Populate the Structure Array

    for (i = 0; i < nrows; i++) {
        rsql[i].code = (int) atoi(PQgetvalue(DBQuery, i, 0));
        if (strlen(PQgetvalue(DBQuery, i, 1)) > 0)
            IDL_StrStore(&(rsql[i].description), PQgetvalue(DBQuery, i, 1));
        else
            IDL_StrStore(&(rsql[i].description), "");
    }

//--------------------------------------------------------------------------
// Create the IDL Structure

    ilDims[0] = nrows;        // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);
    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) rsql, freeMem, psDef);

//--------------------------------------------------------------------------
// Cleanup Keywords & PG Heap

    IDL_KW_FREE;
    PQclear(DBQuery);
    return (ivReturn);
}

IDL_VPTR IDL_CDECL roclose(int argc, IDL_VPTR argv[], char* argk) {
//
// IDL DLM Function to Close the Socket Connection to the Database Server
//
// No Arguments:
//
// If the IDL Exit Handler Routine was called as expected (Not the case!), this
// function would Not be Needed.
//
// v0.01	31Jan2006	D.G.Muir	Original Release
//-------------------------------------------------------------------------
    if (DBConnect != NULL) PQfinish(DBConnect);
    DBConnect = NULL;
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamlastshot(int argc, IDL_VPTR argv[], char* argk) {

    int nrows, pulno;

//-----------------------------------------------------------------------
// Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
    } KW_RESULT;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.verbose = 0;
    kw.debug = 0;

    dbglog = stdout;        // Debug Log
    errlog = stderr;        // Error Log
    stdlog = stdout;        // Standard Log Output


//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//-----------------------------------------------------------------------
// Connect to the Database Server

    if (DBConnect == NULL) {
        if (!(DBConnect = startSQL())) {
            if (kw.verbose) fprintf(stderr, "getidamlastshot:SQL Server Connect Error \n");
            PQfinish(DBConnect);
            DBConnect = NULL;
            return (IDL_GettmpLong(0));
        }
    }

//-----------------------------------------------------------------------
// Query the IDAM Database

    nrows = getLastShot(DBConnect, &pulno, kw.verbose, kw.debug);

    IDL_KW_FREE;

    if (nrows != 1) return (IDL_GettmpLong(0));

    return (IDL_GettmpLong(pulno));
}


IDL_VPTR IDL_CDECL getidamshotdatetime(int argc, IDL_VPTR argv[], char* argk) {

    int nrows, pulno;
    char date[MAXDATE], time[MAXDATE];

    IDL_VPTR ivReturn = NULL;
    void* psDef = NULL;

    IDL_MEMINT ilDims[IDL_MAX_ARRAY_DIM];
    //IDL_LONG ilDims[IDL_MAX_ARRAY_DIM];

    typedef struct {
        IDL_LONG exp_number;
        IDL_STRING date;
        IDL_STRING time;
    } SOUT;

    SOUT* sout = NULL;


// IDL tags structure

    IDL_STRUCT_TAG_DEF pTags[] = {
            {"EXP_NUMBER", 0, (void*) IDL_TYP_LONG},
            {"DATE",       0, (void*) IDL_TYP_STRING},
            {"TIME",       0, (void*) IDL_TYP_STRING},
            {0}
    };

//-----------------------------------------------------------------------
// Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
    } KW_RESULT;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.verbose = 0;
    kw.debug = 0;

    dbglog = stdout;        // Debug Log
    errlog = stderr;        // Error Log
    stdlog = stdout;        // Standard Log Output


//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//-----------------------------------------------------------------------
// Passed Shot Number

    IDL_ENSURE_SCALAR(argv[0]);
    pulno = IDL_LongScalar(argv[0]);

//-----------------------------------------------------------------------
// Connect to the Database Server

    if (DBConnect == NULL) {
        if (!(DBConnect = startSQL())) {
            if (kw.verbose) fprintf(stderr, "idamshotdatetime:SQL Server Connect Error \n");
            PQfinish(DBConnect);
            DBConnect = NULL;
            return (IDL_GettmpLong(0));
        }
    }

//-----------------------------------------------------------------------
// Query the IDAM Database

    nrows = getExpDateTime(DBConnect, pulno, date, time, kw.verbose);

    IDL_KW_FREE;

    if (nrows != 1) return (IDL_GettmpLong(0));

    if (kw.debug) {
        fprintf(stdout, "Shot %d\n", pulno);
        fprintf(stdout, "Date %s\n", date);
        fprintf(stdout, "Time %s\n", time);
    }

//--------------------------------------------------------------------------
// Assign the Return Structure

    sout = (SOUT*) malloc(sizeof(SOUT));

    sout->exp_number = pulno;
    IDL_StrStore(&(sout->date), date);
    IDL_StrStore(&(sout->time), time);

//--------------------------------------------------------------------------
// Create the Return IDL Structure

    ilDims[0] = 1;    // Number of Structure Array Elements

    psDef = IDL_MakeStruct(NULL, pTags);
    ivReturn = IDL_ImportArray(1, ilDims, IDL_TYP_STRUCT, (UCHAR*) sout, freeMem, psDef);

    return (ivReturn);
}


IDL_VPTR IDL_CDECL getidamlatestsourcepass(int argc, IDL_VPTR argv[], char* argk) {

    int nrows, pulno, pass;
    char* source;

//-----------------------------------------------------------------------
// Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
    } KW_RESULT;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.verbose = 0;
    kw.debug = 0;

    dbglog = stdout;        // Debug Log
    errlog = stderr;        // Error Log
    stdlog = stdout;        // Standard Log Output


//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//-----------------------------------------------------------------------
// Connect to the Database Server

    if (DBConnect == NULL) {
        if (!(DBConnect = startSQL())) {
            if (kw.verbose) fprintf(stderr, "getidamlastshot:SQL Server Connect Error \n");
            PQfinish(DBConnect);
            DBConnect = NULL;
            return (IDL_GettmpLong(-1));
        }
    }

//-----------------------------------------------------------------------
// Passed Shot Number and Source Alias

    IDL_ENSURE_SCALAR(argv[0]);
    pulno = IDL_LongScalar(argv[0]);

    IDL_ENSURE_STRING(argv[1]);
    source = (char*) IDL_STRING_STR(&(argv[1]->value.str));

//-----------------------------------------------------------------------
// Query the IDAM Database

    nrows = getLatestSourcePass(DBConnect, pulno, source, kw.verbose, &pass);

    IDL_KW_FREE;

    if (nrows != 1) return (IDL_GettmpLong(-1));

    return IDL_GettmpLong(pass);
}


IDL_VPTR IDL_CDECL getidampath(int argc, IDL_VPTR argv[], char* argk) {

    int nrows, pulno, iscase = 1;
    char* source;
    char path[MAXPATH];
    char type[2] = " ";
    char* ptype;

//-----------------------------------------------------------------------
// Keyword Structure

    typedef struct {
        IDL_KW_RESULT_FIRST_FIELD;
        IDL_LONG verbose;
        IDL_LONG debug;
        IDL_LONG help;
        IDL_LONG is_pass;
        IDL_LONG pass;
        IDL_LONG is_type;
        IDL_STRING type;
    } KW_RESULT;

// Maintain Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] =
            {IDL_KW_FAST_SCAN,
             {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
             {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
             {"PASS", IDL_TYP_LONG, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_pass), IDL_KW_OFFSETOF(pass)},
             {"TYPE", IDL_TYP_STRING, 1, IDL_KW_ZERO, IDL_KW_OFFSETOF(is_type), IDL_KW_OFFSETOF(type)},
             {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
             {NULL}};

    KW_RESULT kw;

    kw.verbose = 0;
    kw.debug = 0;
    kw.help = 0;
    kw.pass = -1;
    kw.is_pass = 0;
    kw.is_type = 0;

    dbglog = stdout;        // Debug Log
    errlog = stderr;        // Error Log
    stdlog = stdout;        // Standard Log Output

//-----------------------------------------------------------------------
// Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

//-----------------------------------------------------------------------
// Help Requested?

    if (kw.help) {
        fprintf(stdout, "\ngetidampath: Return the file path to a Data Source\n\n");
        fprintf(stdout, "syntax: (string)path = getidampath( (long)shotno, (string)alias [,pass=(long)pass]"
                "[,type=(string)type] [,verbose=verbose])\n");
        fprintf(stdout, "The pass number is only relevant for Analysed sources (type 'A'). "
                "If not passed, the Latest pass is assumed.\n");
        fprintf(stdout, "Other types: R => Raw sources, I => Image sources. By default, Raw type is assumed.\n\n");
        return (IDL_GettmpLong(0));
    }

//-----------------------------------------------------------------------
// Connect to the Database Server

    if (DBConnect == NULL) {
        if (!(DBConnect = startSQL())) {
            if (kw.verbose) fprintf(stderr, "getidampath:SQL Server Connect Error \n");
            PQfinish(DBConnect);
            DBConnect = NULL;
            return IDL_GettmpLong(0);
        }
    }

//-----------------------------------------------------------------------
// Passed Shot Number and Source Alias

    IDL_ENSURE_SCALAR(argv[0]);
    pulno = IDL_LongScalar(argv[0]);

    IDL_ENSURE_STRING(argv[1]);
    source = (char*) IDL_STRING_STR(&(argv[1]->value.str));

    if (kw.is_type) {
        ptype = IDL_STRING_STR(&kw.type);
        type[0] = ptype[0];
    } else {
        type[0] = '\0';
    }

    if (!kw.is_pass) kw.pass = -1;    // Not set to -1 above as expected - IDL_KW_ZERO !!!

    if (kw.debug) {
        fprintf(stdout, "getidampath: Shot %d, Source %s\n", pulno, source);
        fprintf(stdout, "isPass %ld\n", (long)kw.is_pass);
        fprintf(stdout, "Pass %ld\n", (long)kw.pass);
        fprintf(stdout, "Type %s\n", type);
    }

//-----------------------------------------------------------------------
// Query the IDAM Database

    nrows = getSourcePath(DBConnect, pulno, kw.pass, source, type, iscase, kw.verbose, path);

    IDL_KW_FREE;

    if (nrows != 1) {
        fprintf(stdout, "getidampath: Zero or Multiple paths found! [%d]\n", nrows);
        return (IDL_GettmpLong(0));
    }

//-----------------------------------------------------------------------
// Return the Path

    return IDL_StrToSTRING(path);        // Create and Return an IDL string
}
