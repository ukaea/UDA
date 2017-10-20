/*---------------------------------------------------------------
* IDAM Plugin data Reader to Access Data from SQL Tables
*
* Input Arguments:	REQUEST_BLOCK request_block
*			DATA_SOURCE data_source
*
* Returns:		readSQL		0 if read was successful
*					otherwise an Error Code is returned
*			DATA_BLOCK	Structure with Data from the target File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
**
* ToDo:
*
*-----------------------------------------------------------------------------*/
#include "readSQL.h"

#include <stdlib.h>
#include <memory.h>
#include <strings.h>

#include <clientserver/errorLog.h>
#include <server/sqllib.h>
#include <clientserver/udaTypes.h>
#include <clientserver/initStructs.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaErrors.h>
#include <structures/struct.h>

#ifdef USEREADSOAP
#include "soapStub.h"
int readSOAP(char *filename, struct _ns1__device *device);
void freeSOAP();
#endif

#ifndef NOTGENERICENABLED

#ifdef TESTCODEX
PGconn *gDBConnect = NULL;
#endif

// Continuously Measured Data

int readCMDSQL(PGconn* DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK* data_block,
               USERDEFINEDTYPELIST* userdefinedtypelist)
{

    int i, ltpass, err = 0;

    double* dp = NULL;
    char* token = NULL;

    char sql[MAXSQL];
    char work[STRING_LENGTH];
    char dtime1[STRING_LENGTH / 2];
    char dtime2[STRING_LENGTH / 2];

    PGresult* DBQuery = NULL;

//-------------------------------------------------------------
// Parse Request for Time Range: model = epoch(dt1, dt2)

    if ((ltpass = strlen(request_block.tpass)) == 0) {
        err = 998;
        addIdamError(CODEERRORTYPE, "readSQL", err, "No Source to Parse for the SQL Data");
        return err;
    }

    strncpy(work, request_block.tpass + 6, ltpass - 7);
    work[ltpass - 3] = '\0';

    if ((token = strtok(work, ",")) != NULL) {
        strcpy(dtime1, token);
    } else {
        err = 998;
        addIdamError(CODEERRORTYPE, "readSQL", err, "Unable to Parse Source for the SQL Data");
        return err;
    }

    if ((token = strtok(NULL, ",")) != NULL) {
        strcpy(dtime2, token);
    }

//-------------------------------------------------------------
// Open SQL Connection

    if ((DBConnect = gDBConnect) == NULL) {        // No connection to IDAM SQL Database
        if (!(DBConnect = startSQL())) {
            if (DBConnect != NULL) PQfinish(DBConnect);
            err = 777;
            addIdamError(CODEERRORTYPE, "readSQL", err, "SQL Database Server Connect Error");
            return err;
        }
        gDBConnect = DBConnect;                // Pass Connection Back
    }

//-------------------------------------------------------------
// Build SQL

    strcpy(sql, "SELECT EXTRACT(EPOCH FROM CMData.dtime) as epoch, value FROM CMData WHERE name = '");
    strcat(sql, request_block.signal);
    strcat(sql, "' AND dtime >= '");
    strcat(sql, dtime1);
    if (strlen(dtime2) > 0) {
        strcat(sql, "' AND dtime <= '");
        strcat(sql, dtime2);
    }
    strcat(sql, "';");

//fprintf(stdout,"%s\n",sql);

//-------------------------------------------------------------
// Execute SQL

    if ((DBQuery = PQexec(DBConnect, sql)) == NULL) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readSQL", err, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        return err;
    }

//-------------------------------------------------------------
// Retrieve Data

// Error Trap Loop

    err = 0;

    do {

// Allocate Heap

        data_block->data_n = (int)PQntuples(DBQuery);        // Number of Rows
        data_block->rank = 1;
        data_block->order = 0;
        data_block->data_type = UDA_TYPE_DOUBLE;
        strcpy(data_block->data_desc, request_block.signal);
        strcpy(data_block->data_label, request_block.signal);
        strcpy(data_block->data_units, "");

        if ((dp = (double*)malloc(sizeof(double) * data_block->data_n)) == NULL) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err,
                         "Unable to Allocate Heap Memory for the SQL Data");
            break;
        }

        data_block->data = (char*)dp;

        for (i = 0; i < data_block->data_n; i++) dp[i] = strtod(PQgetvalue(DBQuery, i, 1), NULL);

//----------------------------------------------------------------------
// Allocate & Initialise Dimensional Structures

        if ((data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS))) == NULL) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err, "Problem Allocating Dimension Heap Memory");
            break;
        }

        for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

// Dimension Labels and Units

        data_block->dims[0].dim_n = data_block->data_n;
        data_block->dims[0].data_type = UDA_TYPE_DOUBLE;
        strcpy(data_block->dims[0].dim_label, "Epoch Time");
        strcpy(data_block->dims[0].dim_units, "sec");

        if ((data_block->dims[0].dim = (char*)malloc(sizeof(double) * data_block->dims[0].dim_n)) == NULL) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err, "Problem Allocating Dimensional Heap Memory");
            break;
        }

        dp = (double*)data_block->dims[0].dim;

        for (i = 0; i < data_block->dims[0].dim_n; i++) dp[i] = strtod(PQgetvalue(DBQuery, i, 0), NULL);

    } while (0);

//----------------------------------------------------------------------
// Housekeeping

    if (err != 0) freeDataBlock(data_block);

    PQclear(DBQuery);

    return err;
}

//================================================================================================================
// General SQL and standard functions

// Data Discovery: what data do you have?

// For the default device:	list of data archives.
// For each archive:		list of data sources; list of data items.
// External links:		list of known external devices and how to connect to them (web, idam, mds+).

int readSQL(PGconn* DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK* data_block,
            USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int err = 0;

    char sql[MAXSQL];

    PGconn* DBConnect2 = NULL;        // Private connection: Different to the IDAM database connection
    PGresult* DBQuery = NULL;

    unsigned short queryType;
    enum queryType {
        SQLUNKNOWN,
        SQLLASTSHOT,
        SQLDATETIME,
        SQLSOURCES,
        SQLSIGNALS,
        SQLSOAPTEST,
        SQLEFITTEST,
        SQLTYPETEST,
        SQLDEVELOP
    };

//-------------------------------------------------------------
// Is this a request for Continuously Measured Data?

    if (STR_IEQUALS(request_block.archive, "CMD")) {
        return (readCMDSQL(DBConnect, request_block, data_source, data_block, userdefinedtypelist));
    }

//-------------------------------------------------------------
// Standard functions

    sql[0] = '\0';
    queryType = SQLUNKNOWN;

    if (STR_IEQUALS(request_block.signal, "getidamlastshot()")) {
        strcpy(sql, "SELECT max(exp_number) FROM ExpDateTime;");
        queryType = SQLLASTSHOT;
    }

    char work[MAX_STRING_LENGTH];
    char udtname[MAX_STRING_LENGTH];

    short shotDependent = 0;
    short sourceDependent = 0;
    short typeDependent = 0;
    short passDependent = 0;

    int pass = 0;
    int exp_number = 0;

    if (queryType == SQLUNKNOWN && !strncasecmp(request_block.signal, "getidamshotdatetime(", 20)) {
        char* p = strchr(request_block.signal, '(');
        strcpy(work, &p[1]);
        if ((p = strchr(work, ')')) != NULL) {    // check there is an enclosing parenthesis
            p[0] = '\0';
            LeftTrimString(work);
            TrimString(work);

// Name value pairs?

            if (request_block.nameValueList.pairCount == 0) {

// Test the argument is an integer only

                if (work[0] != '\0' && IsNumber(work)) {
                    sprintf(sql, "SELECT exp_date, exp_time FROM ExpDateTime WHERE exp_number=%d", atoi(work));
                }

            } else {
                if (request_block.nameValueList.pairCount > 0) {

// allowed names and value types

// exp_number, shot, pulno	single scalar integer
                    exp_number = -1;

                    int ip;
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "exp_number") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "shot") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "pulno")) {
                            if (IsNumber(request_block.nameValueList.nameValue[ip].value)) {
                                exp_number = atoi(request_block.nameValueList.nameValue[ip].value);
                                shotDependent = 1;
                                break;
                            }
                        }
                    }

                    sprintf(sql, "SELECT exp_date, exp_time FROM ExpDateTime WHERE exp_number=%d", exp_number);
                }
            }
        }
        if (sql[0] != '\0') {
            strcpy(udtname, "getidamshotdatetime");
            queryType = SQLDATETIME;
        }
    }

    if (queryType == SQLUNKNOWN && !strncasecmp(request_block.signal, "getidamsignals(", 15)) {
        char* p = strchr(request_block.signal, '(');
        strcpy(work, &p[1]);
        if ((p = strchr(work, ')')) != NULL) {
            p[0] = '\0';
            LeftTrimString(work);
            TrimString(work);

// Name value pairs

// *** For security reasons, the original form where a WHERE clause is passed has been removed. Name value pairs allow for
// greater flexibility with security.

            if (request_block.nameValueList.pairCount == 0 && work[0] != '\0') {
                // sprintf(sql, "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc WHERE %s", work);
            } else {
                if (request_block.nameValueList.pairCount > 0) {

// allowed names and value types

// exp_number, shot, pulno	single scalar integer
// pass				single scalar integer
// source, source_alias		string  (3 characters)
// type				single character

                    short ip;
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "exp_number") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "shot") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "pulno")) {
                            if (IsNumber(request_block.nameValueList.nameValue[ip].value)) {
                                exp_number = atoi(request_block.nameValueList.nameValue[ip].value);
                                shotDependent = 1;
                                break;
                            }
                        }
                    }
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "pass")) {
                            if (IsNumber(request_block.nameValueList.nameValue[ip].value)) {
                                pass = atoi(request_block.nameValueList.nameValue[ip].value);
                                passDependent = 1;
                                break;
                            }
                        }
                    }
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "type")) {
                            if (strlen(request_block.nameValueList.nameValue[ip].value) == 1) typeDependent = ip;
                        }
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "source") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "source_alias")) {
                            if (strlen(request_block.nameValueList.nameValue[ip].value) == 3) sourceDependent = ip;
                        }
                    }

                    work[0] = '\0';
                    if (sourceDependent >= 0 && typeDependent >= 0) {
                        sprintf(work, " source_alias = '%s' AND type = '%s' ",
                                strlwr(request_block.nameValueList.nameValue[sourceDependent].value),
                                strupr(request_block.nameValueList.nameValue[typeDependent].value));
                    } else {
                        if (sourceDependent >= 0) {
                            sprintf(work, " source_alias = '%s' ",
                                    strlwr(request_block.nameValueList.nameValue[sourceDependent].value));
                        }
                        if (typeDependent >= 0) {
                            sprintf(work, " type = '%s' ",
                                    strupr(request_block.nameValueList.nameValue[typeDependent].value));
                        }
                    }

                    if (shotDependent) {
                        if (passDependent) {
                            if (sourceDependent >= 0 || typeDependent >= 0) {
                                sprintf(sql,
                                        "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc as D, "
                                                "(SELECT DISTINCT signal_desc_id from Signal as A, (SELECT source_id FROM Data_Source WHERE "
                                                "exp_number = %d AND pass = %d AND %s) as B WHERE A.source_id = B.source_id) as C WHERE "
                                                "D.signal_desc_id = C.signal_desc_id ORDER BY signal_alias ASC",
                                        exp_number, pass, work);
                            } else {
                                sprintf(sql,
                                        "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc as D, "
                                                "(SELECT DISTINCT signal_desc_id from Signal as A, (SELECT source_id FROM Data_Source WHERE "
                                                "exp_number = %d AND pass = %d) as B WHERE A.source_id = B.source_id) as C WHERE "
                                                "D.signal_desc_id = C.signal_desc_id ORDER BY signal_alias ASC",
                                        exp_number, pass);
                            }
                        } else {
                            if (sourceDependent >= 0 || typeDependent >= 0) {
                                sprintf(sql,
                                        "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc as D, "
                                                "(SELECT DISTINCT signal_desc_id from Signal as A, (SELECT source_id FROM Data_Source WHERE "
                                                "exp_number = %d AND %s) as B WHERE A.source_id = B.source_id) as C WHERE "
                                                "D.signal_desc_id = C.signal_desc_id ORDER BY signal_alias ASC",
                                        exp_number, work);
                            } else {
                                sprintf(sql,
                                        "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc as D, "
                                                "(SELECT DISTINCT signal_desc_id from Signal as A, (SELECT source_id FROM Data_Source WHERE "
                                                "exp_number = %d) as B WHERE A.source_id = B.source_id) as C WHERE "
                                                "D.signal_desc_id = C.signal_desc_id ORDER BY signal_alias ASC",
                                        exp_number);
                            }
                        }
                    } else {
                        if (sourceDependent >= 0 || typeDependent >= 0) {
                            sprintf(sql,
                                    "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc WHERE "
                                            "%s ORDER BY signal_alias ASC", work);
                        } else {
                            sprintf(sql,
                                    "SELECT signal_alias, generic_name, source_alias, type, description FROM Signal_Desc "
                                            "ORDER BY signal_alias ASC");
                        }
                    }
                }
            }
        }
        if (sql[0] != '\0') {
            strcpy(udtname, "getidamsignals");
            queryType = SQLSIGNALS;
        }
    }

    if (queryType == SQLUNKNOWN && !strncasecmp(request_block.signal, "getidamsources(", 15)) {
        char* p = strchr(request_block.signal, '(');
        strcpy(work, &p[1]);
        if ((p = strchr(work, ')')) != NULL) {
            p[0] = '\0';
            LeftTrimString(work);
            TrimString(work);

// Test for Name value pairs

            if (request_block.nameValueList.pairCount == 0) {

// Test the argument is an integer only

                if (work[0] != '\0' && IsNumber(work)) {
                    sprintf(sql,
                            "SELECT source_alias, pass, source_status, format, filename, type FROM Data_Source WHERE "
                                    "exp_number=%d ORDER BY source_alias ASC, pass DESC", atoi(work));
                }
            } else {
                if (request_block.nameValueList.pairCount > 0) {

// allowed names and value types

// exp_number, shot, pulno	single scalar integer
// pass				single scalar integer
// source, source_alias		string (3 characters)
// type				single character

                    short ip;
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "exp_number") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "shot") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "pulno")) {
                            if (IsNumber(request_block.nameValueList.nameValue[ip].value)) {
                                exp_number = atoi(request_block.nameValueList.nameValue[ip].value);
                                shotDependent = 1;
                                break;
                            }
                        }
                    }
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "pass")) {
                            if (IsNumber(request_block.nameValueList.nameValue[ip].value)) {
                                pass = atoi(request_block.nameValueList.nameValue[ip].value);
                                passDependent = 1;
                                break;
                            }
                        }
                    }
                    for (ip = 0; ip < request_block.nameValueList.pairCount; ip++) {
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "type")) {
                            if (strlen(request_block.nameValueList.nameValue[ip].value) == 1) typeDependent = ip;
                        }
                        if (STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "source") ||
                            STR_IEQUALS(request_block.nameValueList.nameValue[ip].name, "source_alias")) {
                            if (strlen(request_block.nameValueList.nameValue[ip].value) == 3) sourceDependent = ip;
                        }
                    }

                    work[0] = '\0';
                    if (sourceDependent >= 0 && typeDependent >= 0) {
                        sprintf(work, " source_alias = '%s' AND type = '%s' ",
                                strlwr(request_block.nameValueList.nameValue[sourceDependent].value),
                                strupr(request_block.nameValueList.nameValue[typeDependent].value));
                    } else {
                        if (sourceDependent >= 0) {
                            sprintf(work, " source_alias = '%s' ",
                                    strlwr(request_block.nameValueList.nameValue[sourceDependent].value));
                        }
                        if (typeDependent >= 0) {
                            sprintf(work, " type = '%s' ",
                                    strupr(request_block.nameValueList.nameValue[typeDependent].value));
                        }
                    }

                    if (shotDependent) {
                        if (passDependent) {
                            if (sourceDependent >= 0 || typeDependent >= 0) {
                                sprintf(sql,
                                        "SELECT source_alias, pass, source_status, format, filename, type FROM Data_Source WHERE "
                                                " exp_number=%d AND pass=%d AND %s  ORDER BY source_alias ASC, pass DESC",
                                        exp_number, pass, work);
                            } else {
                                sprintf(sql,
                                        "SELECT source_alias, pass, source_status, format, filename, type FROM Data_Source WHERE "
                                                " exp_number=%d AND pass=%d ORDER BY source_alias ASC, pass DESC",
                                        exp_number, pass);
                            }
                        } else {
                            if (sourceDependent >= 0 || typeDependent >= 0) {
                                sprintf(sql,
                                        "SELECT source_alias, pass, source_status, format, filename, type FROM Data_Source WHERE "
                                                " exp_number=%d AND %s ORDER BY source_alias ASC, pass DESC",
                                        exp_number, work);
                            } else {
                                sprintf(sql,
                                        "SELECT source_alias, pass, source_status, format, filename, type FROM Data_Source WHERE "
                                                " exp_number=%d ORDER BY source_alias ASC, pass DESC",
                                        exp_number);
                            }
                        }
                    }
                }
            }
        }
        if (sql[0] != '\0') {
            strcpy(udtname, "getidamsources");
            queryType = SQLSOURCES;
        }
    }

    if (queryType == SQLUNKNOWN && !strncasecmp(request_block.signal, "efitdatatest", 12)) {
        strcpy(udtname, "efitdatatest");
        queryType = SQLEFITTEST;
    }

    if (queryType == SQLUNKNOWN && !strncasecmp(request_block.signal, "soapdatatest", 12)) {
        strcpy(udtname, "soapdatatest");
        queryType = SQLSOAPTEST;
    }

    if (queryType == SQLUNKNOWN) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readSQL", err,
                     "Requested Query not known or a Syntax Error is present!");
        if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return err;
    }

//-------------------------------------------------------------
// Open SQL Connection

    DBConnect2 = DBConnect;        // Use IDAM database connection if passed

    if (DBConnect2 == NULL) {
        if (!(DBConnect2 = (PGconn*)startSQL())) {
            if (DBConnect2 != NULL) PQfinish(DBConnect2);
            err = 777;
            addIdamError(CODEERRORTYPE, "readSQL", err, "SQL Database Server Connect Error");
            return err;
        }
    }

//-------------------------------------------------------------
// Check the Server Details and connect if necessary to a different server

//-------------------------------------------------------------
// Execute SQL

    if (strlen(sql) > 0 && (DBQuery = PQexec(DBConnect2, sql)) == NULL) {
        err = 999;
        addIdamError(CODEERRORTYPE, "readSQL", err, PQresultErrorMessage(DBQuery));
        PQclear(DBQuery);
        if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return err;
    }

//-------------------------------------------------------------
//-------------------------------------------------------------
// Retrieve Data

// Latest Shot Number: getidamlastshot()	Not Hierarchical as only an integer value is returned

    if (queryType == SQLLASTSHOT && PQntuples(DBQuery) == 1) {
        int* data = (int*)malloc(sizeof(int));
        data_block->data_type = UDA_TYPE_INT;
        data_block->rank = 0;
        data_block->data_n = 1;
        data[0] = atoi(PQgetvalue(DBQuery, 0, 0));
        data_block->data = (void*)data;
        strcpy(data_block->data_desc, request_block.signal);
        strcpy(data_block->data_label, "Latest Shot Number");
        strcpy(data_block->data_units, "");
        PQclear(DBQuery);
        if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return 0;
    }

//-------------------------------------------------------------
// List Data: Increase Rank of string arrays and reduce rank of passed structure for improved performance
//
// struct{
//    char **field1;		// String Arrays: Use Arbitrary Length String Array Recipe
//    char **field2;
//    ....
// }

    if (queryType == SQLDATETIME || queryType == SQLSOURCES || queryType == SQLSIGNALS) {

        int j, len, offset = 0, rank;
        int* shape;
        USERDEFINEDTYPE* udt, usertype;
        COMPOUNDFIELD field;
        char*** data;        // Array of String Arrays
        char* p;

        LOGMALLOCLIST* logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));        // Create a MALLOC Log List
        initLogMallocList(logmalloclist);

        copyUserDefinedTypeList(&userdefinedtypelist); // Allocate and Copy the Master User Defined Type List

        int nrows = PQntuples(DBQuery);
        int ncols = PQnfields(DBQuery);

        if (nrows == 0) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err, "No Data Rows Returned from the Database");
            addIdamError(CODEERRORTYPE, "readSQL", err, sql);
            PQclear(DBQuery);
            if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
            return err;
        }

        if (ncols == 0) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err, "No Data Columns Returned from the Database");
            PQclear(DBQuery);
            if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
            return err;
        }

// The packed data structure is identical to an array of string arrays

        data = (char***)malloc(
                ncols * sizeof(char**));            // Single Structure with multiple (ncols) Arrays of Strings
        addMalloc(logmalloclist, (void*)data, 1, ncols * sizeof(char**), udtname);

// Contiguous data block with multiple string arrays

        for (j = 0; j < ncols; j++) {
            char** field = (char**)malloc(nrows * sizeof(char*));        // Table fields: each array of size nrows
            rank = 1;
            shape = NULL;
            addMalloc2(logmalloclist, (void*)field, nrows, sizeof(char*), "STRING", rank, shape);
            int i;
            for (i = 0; i < nrows; i++) {
                len = strlen(PQgetvalue(DBQuery, i, j)) + 1;
                field[i] = (char*)malloc(len * sizeof(char));
                addMalloc(logmalloclist, (void*)field[i], len, sizeof(char), "STRING");    // Individual scalar strings
                strcpy(field[i], PQgetvalue(DBQuery, i, j));
            }
            data[j] = field;
        }

        initUserDefinedType(&usertype);                        // New structure definition

        strcpy(usertype.name, udtname);
        strcpy(usertype.source, "IDAM SQL::");
        usertype.ref_id = 0;
        usertype.imagecount = 0;                // No Structure Image data
        usertype.image = NULL;
        usertype.size = ncols * sizeof(char**);    // Structure size
        usertype.idamclass = UDA_TYPE_COMPOUND;

        strcpy(work, &sql[7]);        // Step over 'SELECT '

        for (j = 0; j < ncols; j++) {
            initCompoundField(&field);

            if ((p = strchr(work, ',')) == NULL) {
                if ((p = strstr(work, " FROM")) == NULL) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "readSQL", err,
                                 "Select Query: Column names incorrectly formatted!");
                    PQclear(DBQuery);
                    if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
                    return err;
                }
            }

            p[0] = '\0';
            strcpy(field.name, work);
            LeftTrimString(field.name);
            strcpy(sql, &p[1]);
            strcpy(work, sql);
            field.atomictype = UDA_TYPE_STRING;

            field.rank = 0;
            field.count = 1;
            field.shape = NULL;
            // field is a string
            strcpy(field.type, "STRING");            // convert atomic type to a string label
            field.pointer = 1;                // field is a string
            strcpy(field.desc, "");
            field.size = sizeof(char*);
            field.offset = offset;            // Compact array/data structure: no padding (will/maybe expanded on client)
            offset = field.offset + field.size;    // Next Offset
            field.offpad = 0;
            field.alignment = 0;

            addCompoundField(&usertype, field);
        }

        addUserDefinedType(userdefinedtypelist, usertype);

        printUserDefinedType(usertype);
        printUserDefinedTypeListTable(*userdefinedtypelist);

        if ((udt = findUserDefinedType(userdefinedtypelist, udtname, 0)) == NULL) {
            err = 999;
            addIdamError(CODEERRORTYPE, "readSQL", err,
                         "Unable to Locate the User Defined Structure Definition");
            PQclear(DBQuery);
            if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
            return err;
        }

        data_block->data_type = UDA_TYPE_COMPOUND;
        data_block->rank = 0;
        data_block->data_n = 1;
        data_block->data = (char*)data;
        strcpy(data_block->data_desc, request_block.signal);
        strcpy(data_block->data_label, "Database Query Result Set");
        strcpy(data_block->data_units, "");

        data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void*)udt;

        PQclear(DBQuery);
        if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return 0;
    }

//-------------------------------------------------------------
// EFIT XML Data

#ifdef HIERARCHICAL_DATA
#ifndef NOXMLPARSER

    if(queryType == SQLEFITTEST) {

        EFIT *ef = (EFIT *)malloc(sizeof(EFIT));

        logmalloclist = (LOGMALLOCLIST *)malloc(sizeof(LOGMALLOCLIST));		// Create a MALLOC Log List
        initLogMallocList(logmalloclist);

        copyUserDefinedTypeList(&userdefinedtypelist);				// Allocate and Copy the Master User Defined Type List

        rc = parseIncludeFile("/home/dgm/IDAM/source/include/parseEfitXMLPublicString.h");	// Add EFIT structures to list

        initEfit(ef);
        parseItmXML("/home/dgm/IDAM/test/data/idamMAST.xml", ef);					// Parse XML for the Data

        addNonMalloc((void *)ef, 1, sizeof(EFIT), "EFIT");			// Heap Housekeeping separate from regular mallocs

// Retrofit malloc log with heap data

        addNonMalloc((void *)ef, 1, sizeof(EFIT), "EFIT");
        addNonMalloc((void *)ef->pfcoils,    ef->npfcoils,    sizeof(PFCOILS),    "PFCOILS");
        addNonMalloc((void *)ef->pfpassive,  ef->npfpassive,  sizeof(PFPASSIVE),  "PFPASSIVE");
        addNonMalloc((void *)ef->pfsupplies, ef->npfsupplies, sizeof(PFSUPPLIES), "PFSUPPLIES");
        addNonMalloc((void *)ef->fluxloop,   ef->nfluxloops,  sizeof(FLUXLOOP),   "FLUXLOOP");
        addNonMalloc((void *)ef->magprobe,   ef->nmagprobes,  sizeof(MAGPROBE),   "MAGPROBE");
        addNonMalloc((void *)ef->pfcircuit,  ef->npfcircuits, sizeof(PFCIRCUIT),  "PFCIRCUIT");

        addNonMalloc((void *)ef->plasmacurrent,  ef->nplasmacurrent,  sizeof(PLASMACURRENT),  "PLASMACURRENT");
        addNonMalloc((void *)ef->diamagnetic,    ef->ndiamagnetic,    sizeof(DIAMAGNETIC),    "DIAMAGNETIC");
        addNonMalloc((void *)ef->toroidalfield,  ef->ntoroidalfield,  sizeof(TOROIDALFIELD),  "TOROIDALFIELD");
        addNonMalloc((void *)ef->limiter,        ef->nlimiter,        sizeof(LIMITER),        "LIMITER");
        addNonMalloc((void *)ef->polarimetry,    ef->npolarimetry,    sizeof(POLARIMETRY),    "POLARIMETRY");
        addNonMalloc((void *)ef->interferometry, ef->ninterferometry, sizeof(INTERFEROMETRY), "INTERFEROMETRY");

        for(i=0; i<ef->npfcircuits; i++) {
            addNonMalloc((void *)ef->pfcircuit[i].coil, ef->pfcircuit[i].nco, sizeof(int),  "int");
        }
        for(i=0; i<ef->nfluxloops; i++) {
            addNonMalloc((void *)ef->fluxloop[i].r,    ef->fluxloop[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->fluxloop[i].z,    ef->fluxloop[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->fluxloop[i].dphi, ef->fluxloop[i].nco, sizeof(float),  "float");
        }
        for(i=0; i<ef->npfpassive; i++) {
            addNonMalloc((void *)ef->pfpassive[i].r,    ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].z,    ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].dr,   ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].dz,   ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].ang1, ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].ang2, ef->pfpassive[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfpassive[i].res,  ef->pfpassive[i].nco, sizeof(float),  "float");
        }
        for(i=0; i<ef->npfcoils; i++) {
            addNonMalloc((void *)ef->pfcoils[i].r,  ef->pfcoils[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfcoils[i].z,  ef->pfcoils[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfcoils[i].dr, ef->pfcoils[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->pfcoils[i].dz, ef->pfcoils[i].nco, sizeof(float),  "float");
        }
        for(i=0; i<ef->nlimiter; i++) {
            addNonMalloc((void *)ef->limiter[i].r,  ef->limiter[i].nco, sizeof(float),  "float");
            addNonMalloc((void *)ef->limiter[i].z,  ef->limiter[i].nco, sizeof(float),  "float");
        }

        if(debugon) printUserDefinedTypeListTable(dbgout, *userdefinedtypelist);

        data_block->data_type = UDA_TYPE_COMPOUND;
        data_block->rank      = 0;
        data_block->data_n    = 1;
        data_block->data      = (char *)ef;
        strcpy(data_block->data_desc, request_block.signal);
        strcpy(data_block->data_label,"EFIT XML Data Test");
        strcpy(data_block->data_units,"");

        data_block->opaque_type  = UDA_OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void *)findUserDefinedType("EFIT", 0);			// The EFIT structure definition

        if(DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return 0;
    }
#endif

//-------------------------------------------------------------
// SOAP XML Data

#ifdef USEREADSOAP
    if(queryType == SQLSOAPTEST) {

        struct _ns1__device *dev = (struct _ns1__device *)malloc(sizeof(struct _ns1__device));

        logmalloclist = (LOGMALLOCLIST *)malloc(sizeof(LOGMALLOCLIST));		// Create a MALLOC Log List
        initLogMallocList(logmalloclist);

        copyUserDefinedTypeList(&userdefinedtypelist);				// Allocate and Copy the Master User Defined Type List

        rc = parseIncludeFile("/home/dgm/IDAM/source/include/soapStub.h");	// Add SOAP structures to list

        readSOAP("/home/dgm/IDAM/test/data/idamMAST.xml", dev); 			// Parse XML

        addNonMalloc((void *)dev, 1, sizeof(struct _ns1__device), "_ns1__device");	// Heap Housekeeping separate from regular mallocs

        if(debugon) printUserDefinedTypeListTable(dbgout, *userdefinedtypelist);

        malloc_source = MALLOCSOURCESOAP;

        data_block->data_type = UDA_TYPE_COMPOUND;
        data_block->rank      = 0;
        data_block->data_n    = 1;
        data_block->data      = (char *)dev;
        strcpy(data_block->data_desc, request_block.signal);
        strcpy(data_block->data_label,"SOAP XML Data Test");
        strcpy(data_block->data_units,"");

        data_block->opaque_type  = UDA_OPAQUE_TYPE_STRUCTURES;
        data_block->opaque_count = 1;
        data_block->opaque_block = (void *)findUserDefinedType("_ns1__device", 0);		// The SOAP structure definition

        if(DBConnect2 != DBConnect) PQfinish(DBConnect2);
        return 0;
    }
#endif

//-------------------------------------------------------------


#else
    err = 999;
    addIdamError(CODEERRORTYPE, "readSQL", err, "No results returned by SQL Query!");
    PQclear(DBQuery);
    if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
    return err;
#endif

//----------------------------------------------------------------------
// Housekeeping

    if (err != 0) freeDataBlock(data_block);

    PQclear(DBQuery);
    if (DBConnect2 != DBConnect) PQfinish(DBConnect2);
    return err;

}

#ifdef USEREADSOAP
int enable_malloc_log = 1;
#include "readSOAP.c"
#endif

#else

int readSQL(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source, DATA_BLOCK *data_block, USERDEFINEDTYPELIST* userdefinedtypelist) {
    int err = 999;
    addIdamError(CODEERRORTYPE, "readCDF", err, "SQL PLUGIN NOT ENABLED");
    return err;
}

int readCMDSQL(PGconn *DBConnect, REQUEST_BLOCK request_block, DATA_SOURCE data_source,
               DATA_BLOCK *data_block, USERDEFINEDTYPELIST* userdefinedtypelist) {
    int err = 999;
    addIdamError(CODEERRORTYPE, "readCDF", err, "CMD PLUGIN NOT ENABLED");
    return err;
}

#endif
