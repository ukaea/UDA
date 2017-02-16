/*
 *  xpadtree.c
 *
 *  Created on: 24 Feb 2016
 *      Author: lkogan
 *  
 *  Reads signal tag and tree information for use in eg. XPAD.
 *                    
 *  Methods that are implemented
 */

#include <libpq-fe.h>
#include <strings.h>
#include <stdlib.h>

#include <clientserver/errorLog.h>
#include <server/sqllib.h>
#include <structures/struct.h>
#include <clientserver/initStructs.h>
#include <logging/logging.h>
#include <structures/accessors.h>

#include "xpadtree.h"

int idamXpadTree(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(LOG_DEBUG);

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    int err = 0;

    do {

        //Help function
        if (!strcmp(request_block->function, "help")) {
            char* help = "xpadtree::gettree() - Read in tree.\n";

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            initDimBlock(&data_block->dims[0]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "help = description of this plugin");

            data_block->data = (char*) malloc(sizeof(char) * (strlen(help) + 1));
            strcpy(data_block->data, help);

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = (int)strlen(help) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");
            break;
        } else if (!strcmp(request_block->function, "getsignals")) {

            // Return the following information about all signals available in IDAM:
            //  - signal alias
            //  - signal description
            //  - signal category
            //  - signal id
            //
            // This information will come directly from IDAM.
            // At the moment, it is in idam3.
            //
            // Arguments:
            //  - signal_type : ie. 'R' for raw data, 'A' for analysed, 'M' for modelling. If not given, return all.

            //////////////////////////////
            // Read in signal type
            //////////////////////////////
            char* signalType = NULL;
            int foundType = 0;

            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i_arg].name, "signaltype")) {
                    signalType = request_block->nameValueList.nameValue[i_arg].value;
                    IDAM_LOGF(LOG_DEBUG, "Retrieving signals of type %s\n", signalType);
                    foundType = 1;
                }
            }

            //////////////////////////////
            // Open the connection
            // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
            //////////////////////////////
            IDAM_LOG(LOG_DEBUG, "trying to get connection\n");
            PGconn* DBConnect = openDatabase("idam1.mast.ccfe.ac.uk", 56567, "idam", "readonly");
            PGresult* DBQuery = NULL;

            if (DBConnect == NULL) {
                IDAM_LOG(LOG_DEBUG, "Connection to idam1 database failed. %s\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Error connecting to idam3 db!");
                break;
            }

            char query[MAXSQL];

            if (foundType == 1) {
                char* signaltype_for_query = (char*) malloc((2 * strlen(signalType) + 1) * sizeof(char));
                PQescapeStringConn(DBConnect, signaltype_for_query, signalType, strlen(signalType), &err);
                sprintf(query,
                        "SELECT signal_desc_id, signal_alias, description, category FROM signal_desc WHERE type='%s';",
                        signaltype_for_query);
                free(signaltype_for_query);
            } else {
                sprintf(query, "SELECT signal_desc_id, signal_alias, description, category FROM signal_desc;");
            }

            IDAM_LOGF(LOG_DEBUG, "escape query %s\n", query);

            if (err != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Could not escape string");
                PQfinish(DBConnect);
                break;
            }

            // Get result
            if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "SELECT failed. %s\n", query);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "SELECT failed. \n");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                IDAM_LOG(LOG_DEBUG, "Database query failed.\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Database Query Failed!");
                break;
            }

            //Get number of rows that were returned, and allocate memory to arrays
            int nRows = PQntuples(DBQuery);

            IDAM_LOGF(LOG_DEBUG, "query returned %d\n", nRows);

            if (nRows == 0) {
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "No signals with type: %s were found\n", signalType);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "No signals were found\n");
                PQclear(DBQuery);
                PQfinish(DBConnect);
                break;
            }

            IDAM_LOG(LOG_DEBUG, "setup datastruct\n");

            // Struct to store results in
            struct DATASTRUCT {
                char** signal_alias;
                char** signal_description;
                char** signal_category;
                int* signal_id;
            };
            typedef struct DATASTRUCT DATASTRUCT;

            DATASTRUCT* data;
            data = (DATASTRUCT*) malloc(sizeof(DATASTRUCT));
            addMalloc((void*) data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

            data->signal_alias = (char**) malloc((nRows) * sizeof(char*));
            addMalloc((void*) data->signal_alias, nRows, sizeof(char*), "STRING *");
            data->signal_description = (char**) malloc((nRows) * sizeof(char*));
            addMalloc((void*) data->signal_description, nRows, sizeof(char*), "STRING *");
            data->signal_category = (char**) malloc((nRows) * sizeof(char*));
            addMalloc((void*) data->signal_category, nRows, sizeof(char*), "STRING *");
            data->signal_id = (int*) malloc((nRows) * sizeof(int));
            addMalloc((void*) data->signal_id, nRows, sizeof(int), "int");

            //Get data from result
            int s_name_fnum = PQfnumber(DBQuery, "signal_alias");
            int s_desc_fnum = PQfnumber(DBQuery, "description");
            int s_cat_fnum = PQfnumber(DBQuery, "category");
            int s_id_fnum = PQfnumber(DBQuery, "signal_desc_id");

            IDAM_LOG(LOG_DEBUG, "retrieve results\n");
            int i;
            int stringLength;
            for (i = 0; i < nRows; i++) {
                // Retrieve signal alias
                if (!PQgetisnull(DBQuery, i, s_name_fnum)) {
                    stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_name_fnum)) + 1;
                    data->signal_alias[i] = (char*) malloc(stringLength * sizeof(char));
                    strcpy(data->signal_alias[i], PQgetvalue(DBQuery, i, s_name_fnum));
                    addMalloc((void*) data->signal_alias[i], stringLength, sizeof(char), "char");
                } else {
                    data->signal_alias[i] = (char*) malloc(8 * sizeof(char));
                    strcpy(data->signal_alias[i], "unknown");
                    addMalloc((void*) data->signal_alias[i], 8, sizeof(char), "char");
                }
                // Retrieve description, if NULL then set to an empty string
                if (!PQgetisnull(DBQuery, i, s_desc_fnum)) {
                    stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_desc_fnum)) + 1;
                    data->signal_description[i] = (char*) malloc(stringLength * sizeof(char));
                    strcpy(data->signal_description[i], PQgetvalue(DBQuery, i, s_desc_fnum));
                    addMalloc((void*) data->signal_description[i], stringLength, sizeof(char), "char");
                } else {
                    data->signal_description[i] = (char*) malloc(sizeof(char));
                    strcpy(data->signal_description[i], "");
                    addMalloc((void*) data->signal_description[i], 1, sizeof(char), "char");
                }
                // Retrieve category (S or P, for secondary or primary), if NULL then set to S
                if (!PQgetisnull(DBQuery, i, s_cat_fnum)) {
                    stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_cat_fnum)) + 1;
                    data->signal_category[i] = (char*) malloc(stringLength * sizeof(char));
                    strcpy(data->signal_category[i], PQgetvalue(DBQuery, i, s_cat_fnum));
                    addMalloc((void*) data->signal_category[i], stringLength, sizeof(char), "char");
                } else {
                    data->signal_category[i] = (char*) malloc(2 * sizeof(char));
                    strcpy(data->signal_category[i], "S");
                    addMalloc((void*) data->signal_category[i], 2, sizeof(char), "char");
                }
                // Retrieve signal id
                if (!PQgetisnull(DBQuery, i, s_id_fnum)) {
                    data->signal_id[i] = atoi(PQgetvalue(DBQuery, i, s_id_fnum));
                } else {
                    data->signal_id[i] = -1;
                }
            }

            IDAM_LOG(LOG_DEBUG, "close connection\n");

            // Close db connection
            PQclear(DBQuery);
            PQfinish(DBConnect);

            IDAM_LOG(LOG_DEBUG, "define structure for datablock\n");

            // Output data block
            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATASTRUCT");
            strcpy(parentTree.source, "idam3");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATASTRUCT);

            //Compound fields for datastruct
            initCompoundField(&field);
            strcpy(field.name, "signal_alias");
            defineField(&field, "signal_alias", "signal_alias", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "signal_description");
            defineField(&field, "signal_description", "signal_description", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "signal_category");
            defineField(&field, "signal_category", "signal_category", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "signal_id");
            defineField(&field, "signal_id", "signal_id", &offset, ARRAYINT);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            // Put data struct into data block for return
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "signals");
            strcpy(data_block->data_label, "signals");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("DATASTRUCT", 0);

            IDAM_LOG(LOG_DEBUG, "everything done\n");

            break;
        } else if (!strcmp(request_block->function, "getsignaltags")) {

            // Return the mapping information between the tree tags and the signals
            //  - tag id
            //  - signal alias
            //  - signal id
            //
            // Arguments:
            //  - tree_name : tree name from which to obtain mapping

            char* treename = NULL;
            int foundTreename = 0;

            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i_arg].name, "treename")) {
                    treename = request_block->nameValueList.nameValue[i_arg].value;
                    IDAM_LOGF(LOG_DEBUG, "xpadtree : retrieving tags for tree %s\n", treename);
                    foundTreename = 1;
                }
            }

            if (foundTreename != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Treename must be provided.");
                break;
            }

            //////////////////////////////
            // Open the connection
            // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
            //////////////////////////////
            IDAM_LOG(LOG_DEBUG, "trying to get connection\n");
            PGconn* DBConnect = openDatabase("idam1.mast.ccfe.ac.uk", 56567, "xpad", "xpadowner");
            PGresult* DBQuery = NULL;

            if (DBConnect == NULL) {
                IDAM_LOG(LOG_DEBUG, "Connection to xpad database failed.");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Error connecting to xpad db!");
                break;
            }

            // Get signal-tag mapping
            char* treename_for_query = (char*) malloc((2 * strlen(treename) + 1) * sizeof(char));
            PQescapeStringConn(DBConnect, treename_for_query, treename, strlen(treename), &err);

            char query[MAXSQL];
            sprintf(query,
                    "SELECT t.tag_id, ts.signal_desc_id FROM tag t, signal_tags ts WHERE tree_id=(SELECT tree_id FROM tag_tree WHERE tree_name='%s') AND t.tag_id=ts.tag_id;",
                    treename_for_query);
            free(treename_for_query);

            IDAM_LOGF(LOG_DEBUG, "QUERY IS %s\n", query);

            if (err != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Could not escape string");
                PQfinish(DBConnect);
                break;
            }

            // Get result
            if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "SELECT failed. %s\n", query);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "SELECT failed. \n");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                IDAM_LOG(LOG_DEBUG, "Database query failed.\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Database Query Failed!");
                break;
            }

            //Get number of rows that were returned, and allocate memory to arrays
            int nRows = PQntuples(DBQuery);

            IDAM_LOGF(LOG_DEBUG, "query returned %d\n", nRows);

            if (nRows == 0) {
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "No tags for tree: %s were found\n", treename);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "No signals were found\n");
                PQclear(DBQuery);
                PQfinish(DBConnect);
                break;
            }

            IDAM_LOG(LOG_DEBUG, "setup datastruct\n");

            // Struct to store results in
            struct DATASTRUCT {
                int* tag_id;
                int* signal_id;
            };
            typedef struct DATASTRUCT DATASTRUCT;

            DATASTRUCT* data;
            data = (DATASTRUCT*) malloc(sizeof(DATASTRUCT));
            addMalloc((void*) data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

            data->tag_id = (int*) malloc((nRows) * sizeof(int));
            addMalloc((void*) data->tag_id, nRows, sizeof(int), "int");
            data->signal_id = (int*) malloc((nRows) * sizeof(int));
            addMalloc((void*) data->signal_id, nRows, sizeof(int), "int");

            // Get data from result
            int s_tag_id_fnum = PQfnumber(DBQuery, "tag_id");
            int s_signal_id_fnum = PQfnumber(DBQuery, "signal_desc_id");

            IDAM_LOG(LOG_DEBUG, "retrieve results\n");
            int i;
            for (i = 0; i < nRows; i++) {
                if (!PQgetisnull(DBQuery, i, s_tag_id_fnum)) {
                    data->tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_tag_id_fnum));
                } else {
                    data->tag_id[i] = -1;
                }
                // Retrieve signal id
                if (!PQgetisnull(DBQuery, i, s_signal_id_fnum)) {
                    data->signal_id[i] = atoi(PQgetvalue(DBQuery, i, s_signal_id_fnum));
                } else {
                    data->signal_id[i] = -1;
                }
            }

            // Close db connection
            PQclear(DBQuery);
            PQfinish(DBConnect);

            IDAM_LOG(LOG_DEBUG, "define structure for datablock\n");

            // Output data block
            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATASTRUCT");
            strcpy(parentTree.source, "idam3");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATASTRUCT);


            //Compound fields for datastruct
            initCompoundField(&field);
            strcpy(field.name, "tag_id");
            defineField(&field, "tag_id", "tag_id", &offset, ARRAYINT);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "signal_id");
            defineField(&field, "signal_id", "signal_id", &offset, ARRAYINT);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            // Put data struct into data block for return
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "signaltag_map");
            strcpy(data_block->data_label, "signaltag_map");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("DATASTRUCT", 0);

            IDAM_LOG(LOG_DEBUG, "everything done\n");

            break;

        } else if (!strcmp(request_block->function, "gettree")) {

            // Return the tree structure for a tree of a given name
            //  - tag id
            //  - parent tag id
            //  - tag name
            //
            // Also return the maximum tag id for all trees
            //
            // Arguments:
            //  - tree_name : tree name for which to obtain structure
            char* treename = NULL;
            int foundTreename = 0;

            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            IDAM_LOG(LOG_DEBUG, "TEST SIGTREE");

            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (!strcasecmp(request_block->nameValueList.nameValue[i_arg].name, "treename")) {
                    treename = request_block->nameValueList.nameValue[i_arg].value;
                    IDAM_LOGF(LOG_DEBUG, "xpadtree : retrieving tags for tree %s\n", treename);
                    foundTreename = 1;
                }
            }

            if (foundTreename != 1) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Treename must be provided.");
                break;
            }

            //////////////////////////////
            // Open the connection
            // CURRENTLY HARDCODED, NOT SURE YET WHERE THE TAG TABLES WILL BE : WILL NEED REPLACING EVENTUALLY
            //////////////////////////////
            IDAM_LOG(LOG_DEBUG, "trying to get connection\n");
            PGconn* DBConnect = openDatabase("idam1.mast.ccfe.ac.uk", 56567, "xpad", "xpadowner");
            PGresult* DBQuery = NULL;

            if (DBConnect == NULL) {
                IDAM_LOG(LOG_DEBUG, "Connection to xpad database failed.");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Error connecting to xpad db!");
                break;
            }

            // Retrieve maximum tag id
            char* query_max = "SELECT max(tag_id) FROM tag;";

            // Get result
            if ((DBQuery = PQexec(DBConnect, query_max)) == NULL) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "SELECT failed. %s\n", query_max);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "SELECT failed. \n");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                IDAM_LOG(LOG_DEBUG, "Database query failed.\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Database Query Failed!");
                break;
            }

            //Get number of rows that were returned, and allocate memory to arrays
            int nRows_max = PQntuples(DBQuery);

            if (nRows_max == 0) {
                PQclear(DBQuery);
                IDAM_LOG(LOG_DEBUG, "Database query failed to get max id.\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Database Query Failed to get max id!");
                break;
            }

            int max_tag_id = 0;
            int s_max_id_fnum = PQfnumber(DBQuery, "max");
            int i;

            for (i = 0; i < PQntuples(DBQuery); i++) {
                if (!PQgetisnull(DBQuery, i, s_max_id_fnum)) {
                    max_tag_id = atoi(PQgetvalue(DBQuery, i, s_max_id_fnum));
                }
            }

            PQclear(DBQuery);

            // Main query to get tags
            char* treename_for_query = (char*) malloc((2 * strlen(treename) + 1) * sizeof(char));
            PQescapeStringConn(DBConnect, treename_for_query, treename, strlen(treename), &err);

            char query[MAXSQL];
            sprintf(query,
                    "SELECT tag_id, tag_name, parent_tag_id FROM tag WHERE tree_id=(SELECT tree_id FROM tag_tree WHERE tree_name='%s')",
                    treename_for_query);
            free(treename_for_query);

            if (err != 0) {
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Could not escape string");
                PQfinish(DBConnect);
                break;
            }

            // Get result
            if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "SELECT failed. %s\n", query);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "SELECT failed. \n");
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                IDAM_LOG(LOG_DEBUG, "Database query failed.\n");
                err = 999;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Database Query Failed!");
                break;
            }

            //Get number of rows that were returned, and allocate memory to arrays
            int nRows = PQntuples(DBQuery);

            IDAM_LOGF(LOG_DEBUG, "query returned %d\n", nRows);

            if (nRows == 0) {
                err = 999;
                IDAM_LOGF(LOG_DEBUG, "No tags for tree: %s were found\n", treename);
                addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "No signals were found\n");
                PQclear(DBQuery);
                PQfinish(DBConnect);
                break;
            }

            IDAM_LOG(LOG_DEBUG, "setup datastruct\n");

            // Struct to store results in
            struct DATASTRUCT {
                char** tag_name;
                int* tag_id;
                int* parent_tag_id;
                int tag_id_max;
            };
            typedef struct DATASTRUCT DATASTRUCT;

            DATASTRUCT* data;
            data = (DATASTRUCT*) malloc(sizeof(DATASTRUCT));
            addMalloc((void*) data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

            data->tag_name = (char**) malloc((nRows) * sizeof(char*));
            addMalloc((void*) data->tag_name, nRows, sizeof(char*), "STRING *");
            data->tag_id = (int*) malloc((nRows) * sizeof(int));
            addMalloc((void*) data->tag_id, nRows, sizeof(int), "int");
            data->parent_tag_id = (int*) malloc((nRows) * sizeof(int));
            addMalloc((void*) data->parent_tag_id, nRows, sizeof(int), "int");

            data->tag_id_max = max_tag_id;

            // Get data from result
            int s_name_fnum = PQfnumber(DBQuery, "tag_name");
            int s_id_fnum = PQfnumber(DBQuery, "tag_id");
            int s_parent_fnum = PQfnumber(DBQuery, "parent_tag_id");

            int stringLength;

            for (i = 0; i < nRows; i++) {
                if (!PQgetisnull(DBQuery, i, s_name_fnum)) {
                    stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_name_fnum)) + 1;
                    data->tag_name[i] = (char*) malloc(stringLength * sizeof(char));
                    strcpy(data->tag_name[i], PQgetvalue(DBQuery, i, s_name_fnum));
                    addMalloc((void*) data->tag_name[i], stringLength, sizeof(char), "char");
                    IDAM_LOGF(LOG_DEBUG, "%d tag %s\n", i, data->tag_name[i]);
                }
                if (!PQgetisnull(DBQuery, i, s_id_fnum)) {
                    data->tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_id_fnum));
                } else {
                    data->tag_id[i] = -1;
                }
                if (!PQgetisnull(DBQuery, i, s_parent_fnum)) {
                    data->parent_tag_id[i] = atoi(PQgetvalue(DBQuery, i, s_parent_fnum));
                } else {
                    data->parent_tag_id[i] = -1;
                }
            }

            // Close db connection
            PQclear(DBQuery);
            PQfinish(DBConnect);

            IDAM_LOG(LOG_DEBUG, "define structure for datablock\n");

            // Output data block
            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATASTRUCT");
            strcpy(parentTree.source, "idam3");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATASTRUCT);

            //Compound fields for datastruct
            initCompoundField(&field);
            strcpy(field.name, "tag_name");
            defineField(&field, "tag_name", "tag_name", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "tag_id");
            defineField(&field, "tag_id", "tag_id", &offset, ARRAYINT);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "parent_tag_id");
            defineField(&field, "parent_tag_id", "parent_tag_id", &offset, ARRAYINT);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "tag_id_max");
            defineField(&field, "tag_id_max", "tag_id_max", &offset, SCALARINT);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            // Put data struct into data block for return
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*) data;

            strcpy(data_block->data_desc, "tags");
            strcpy(data_block->data_label, "tags");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*) findUserDefinedType("DATASTRUCT", 0);

            IDAM_LOG(LOG_DEBUG, "everything done\n");

            break;

        } else {
            //======================================================================================
            // Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "xpadtree", err, "Unknown function requested!");
            break;
        }

    } while (0);

    return err;

}
