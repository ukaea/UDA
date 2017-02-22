/*
*  geometry.c
*
*  Created on: 24 Feb 2016
*      Author: lkogan
*
*  Reads in geometry db netcdf files.
*
*  Methods that are implemented:
*  help: display help for this plugin
*
*  get : retrieve netcdf file
*        Arguments:
*         - signal : The signal to be extracted from the netcdf file.
*         - file : The file name, for testing with local files. Normally filename is extracted from db.
*                  If using this argument, don't need any of the following arguments.
*         - Config : If argument is present, then the configuration file will be returned.
*         - cal : If argument is present, then the calibration file will be returned.
*         - version_config : Version number for Config file. If not set then the latest will be returned.
*         - version_cal : Version number for calibration file. If not set then the latest will be returned.
*         - three_d : If argument is present, then the 3D geometry is returned, otherwise the 2D geometry is returned.
*         - tor_angle : If returning the 2D geometry, and the component is toroidal angle - dependent, then the user must
*                       give a toroidal angle at which to slice the model.
*
*  getSignalFile : read netcdf signal file (containing information about signal - geometry component mappings)
*        Arguments:
*         - signal : Signal or group to extract from file of the data signal
*         - version : file version number.
*
*  getSignalFilename : retrieve name of filename containing information about data signals associated with a particular geomsignal group or element.
*        Arguments:
*         - geomsignal : Group or signal for which to retrieve corresponding signal filename with signal info
*         - version : file version number. If not set then the latest will be returned.
*/
#include "geometry.h"

#include <math.h>
#include <stdlib.h>
#include <strings.h>

#include <server/sqllib.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <server/modules/netcdf4/readCDF4.h>

////////////////////
// Add error message & send same message to the logs & set error code
////////////////////
void idamErrorAndLog(char* message, int err, int* err_set)
{
    *err_set = err;
    idamLog(LOG_ERROR, message);
    addIdamError(&idamerrorstack, CODEERRORTYPE, "geom", err, message);

    return;
}

/////////
// Check which signal id s are available for this exp number,
// and flag those in the is_available element of the data structure.
// Requires connecting to main IDAM db (rather than geom db)
/////////	
int checkAvailableSignals(int shot, int n_all, int** signal_ids, int** is_available)
{

    idamLog(LOG_DEBUG, "geom: Checking for signal ids in IDAM\n");

    int n_signals_available = 0;

    PGconn* DBConnect = startSQL();
    PGresult* DBQuery_IDAM = NULL;

    if (DBConnect == NULL) {
        idamLog(LOG_ERROR, "geom: Connection to IDAM database failed.\n");
        return 0;
    }

    int i;
    for (i = 0; i < n_all; i++) {
        char query_idam[MAXSQL];

        sprintf(query_idam,
                "SELECT ds.exp_number FROM data_source ds, signal s, signal_desc sd WHERE ds.exp_number=%d AND ds.source_id=s.source_id AND s.signal_desc_id=sd.signal_desc_id AND sd.signal_desc_id=%d;",
                shot, (*signal_ids)[i]);

        if ((DBQuery_IDAM = PQexec(DBConnect, query_idam)) == NULL) {
            idamLog(LOG_ERROR, "geom: IDAM database query failed.\n");
            continue;
        }

        if (PQresultStatus(DBQuery_IDAM) != PGRES_TUPLES_OK && PQresultStatus(DBQuery_IDAM) != PGRES_COMMAND_OK) {
            PQclear(DBQuery_IDAM);
            idamLog(LOG_ERROR, "geom: Database query failed.\n");
            continue;
        }

        int nRows_idam = PQntuples(DBQuery_IDAM);

        if (nRows_idam == 0) {
            PQclear(DBQuery_IDAM);
            continue;
        }

        n_signals_available = n_signals_available + 1;
        (*is_available)[i] = 1;

        PQclear(DBQuery_IDAM);
    }

    PQfinish(DBConnect);

    if (n_signals_available == 0) {
        idamLog(LOG_DEBUG, "geom: None of the signals for this geometry component are available for this shot\n");
    }

    return n_signals_available;

}

////////////////////
// Function to find which geomgroup (and therefore which file) a signal is in
// and then to call a script to create the appropriate geometry file from the CAD.
// This is a test function at the moment, the script called doesn't actually do anything!
// To be integrated with Ivan's python script that extracts info from step files.
//
//   Arguments
//    - signal : The signal for which the file that needs making needs identifying
//    - tor_angle : The toroidal angle at which to extract the information. 
//    - DBConnect : The connection to the geom database
//    - DBQuery : The query for the geom database
////////////////////
int generateGeom(char* signal, float tor_angle, PGconn* DBConnect, PGresult* DBQuery)
{
    char* pythonpath = getenv("PYTHONPATH");
    idamLog(LOG_DEBUG, "geom: python path: %s\n", pythonpath);

    char* path = getenv("PATH");
    idamLog(LOG_DEBUG, "geom: path: %s\n", path);

    idamLog(LOG_DEBUG, "geom: Trying to generate geometry.\n");

    int err = 0;

    // Check if the signal exists
    char query_signal[MAXSQL];
    sprintf(query_signal,
            "SELECT g.name FROM geomgroup g, geomgroup_geomsignal_map ggm WHERE lower(ggm.geomsignal_alias)=lower('%s') AND g.geomgroup_id=ggm.geomgroup_id;",
            signal);

    idamLog(LOG_DEBUG, "geom: Query is %s.\n", query_signal);

    if ((DBQuery = PQexec(DBConnect, query_signal)) == NULL) {
        idamErrorAndLog("geom: Database query failed.\n", 999, &err);
        return err;
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        idamErrorAndLog("geom: Database query failed.\n", 999, &err);
        return err;
    }

    int nRows_signal = PQntuples(DBQuery);

    if (nRows_signal == 0) {
        idamErrorAndLog("geom: No rows were found in database matching query\n", 999, &err);
        return err;
    }

    // Retrieve the geomgroup that we need to retrieve geometry for
    int s_group = PQfnumber(DBQuery, "name");

    if (!PQgetisnull(DBQuery, 0, s_group)) {
        char* geom_group = PQgetvalue(DBQuery, 0, s_group);

        // Run python script to slice model for this component.
        char system_call[MAXSQL];
        sprintf(system_call, "run_make_file_from_cad.sh %s %f", geom_group, tor_angle);

        idamLog(LOG_DEBUG, "geom: Run command %s\n", system_call);

        int ret = system(system_call);

        idamLog(LOG_DEBUG, "geom: Return code from geometry making code %d\n", ret);

        if (ret != 0) return 999;
    } else { return 999; }

    return 0;
}

int idamGeom(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    int err = 0;

    do {

        //Help function
        if (STR_EQUALS(request_block->function, "help")) {
            char* help = "geom::get() - Read in geometry file.\n";

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
            initDimBlock(&data_block->dims[0]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "geom: help = description of this plugin");

            data_block->data = (char*)malloc(sizeof(char) * (strlen(help) + 1));
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

        } else if (STR_EQUALS(request_block->function, "get")) {

            //Get function to return data from configuration file and corresponding calibration file.
            // Arguments:
            // file : Location of file. If not given, then appropriate file name will be retrieved from the db
            //        for the given signal.
            // signal : Signal/Group to be retrieved from the file.
            // Config : If argument is present, then the configuration file will be returned.
            // cal : If argument is present, then the calibration file will be returned.
            // version_config : Version number for Config file. If not set then the latest will be returned.
            // version_cal : Version number for calibration file. If not set then the latest will be returned.
            // three_d : If argument is present, then the 3D geometry is returned, otherwise the 2D geometry is returned.
            // tor_angle : If returning the 2D geometry, and the component is toroidal angle - dependent, then the user must
            //             give a toroidal angle at which to slice the model.

            idamLog(LOG_DEBUG, "geom: get file and signal names.\n");

            ////////////////////////////
            // Get parameters passed by user.
            char* file = NULL;
            char* signal = NULL;

            // Flags to determine what to retrieve
            int isConfig = 0;
            int isCal = 0;
            int isFile = 0;

            // User can specify which versions to bring back
            int versionCal = -1;
            int versionConfig = -1;

            // Toroidal angle dependence
            int three_d = 0;
            float tor_angle = -1;

            int shot = request_block->exp_number;
            idamLog(LOG_DEBUG, "geom: Exp number %d\n", shot);

            // User must specify which shot number they are interested in
            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "signal")) {
                    signal = request_block->nameValueList.nameValue[i_arg].value;
                    idamLog(LOG_DEBUG, "configPlugin: Using signal name: %s\n", signal);
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "file")) {
                    file = request_block->nameValueList.nameValue[i_arg].value;
                    isFile = 1;
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "Config")) {
                    isConfig = 1;
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "cal")) {
                    isCal = 1;
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "version_cal")) {
                    if (IsNumber(request_block->nameValueList.nameValue[i_arg].value)) {
                        versionCal = atoi(request_block->nameValueList.nameValue[i_arg].value);
                    }
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "version_config")) {
                    if (IsNumber(request_block->nameValueList.nameValue[i_arg].value)) {
                        versionConfig = atoi(request_block->nameValueList.nameValue[i_arg].value);
                    }
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "three_d")) {
                    three_d = 1;
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "tor_angle")) {
                    idamLog(LOG_DEBUG, "geom : Found tor_angle %s\n",
                            request_block->nameValueList.nameValue[i_arg].value);
                    if (IsFloat(request_block->nameValueList.nameValue[i_arg].value)) {
                        tor_angle = (float)atof(request_block->nameValueList.nameValue[i_arg].value);
                        if (tor_angle < 0 || tor_angle > 2 * M_PI) {
                            idamLog(LOG_DEBUG,
                                    "geom plugin: If providing a toroidal angle, it must be in the range 0 - 2 pi. Angle given will be ignored.\n");
                            tor_angle = -1;
                        }
                        idamLog(LOG_DEBUG, "geom : Tor_angle is set to %f\n", tor_angle);
                    }
                }
            }

            idamLog(LOG_DEBUG, "Config? %d or cal %d \n", isConfig, isCal);

            if (isFile == 0 && isCal == 0 && isConfig == 0) {
                idamErrorAndLog("Filename wasn't given and didn't request configuration or calibration data.\n", 999,
                                &err);
                break;
            }

            ////////////////////////////
            // If Config or cal arguments were given,
            // connect to database to retrieve filenames
            char* signal_type = NULL;
            if (isConfig == 1 || isCal == 1) {

                //////////////////////////////
                // Open the connection
                // CURRENTLY HARDCODED IN WHILE I'M TESTING
                // .... Once this is actually in the new MAST-U db, will need to use idam functions as in readMeta to open connection.
                idamLog(LOG_DEBUG, "geom: trying to get connection\n");
                //	  PGconn* DBConnect = PQconnectdb("dbname=mastgeom user=mastgeom password=mastgeom"); // Local db for testing
                PGconn* DBConnect = PQconnectdb(
                        "dbname=idam user=idam password=idam@idam3 host=idam3.mast.ccfe.ac.uk port=60000"); // Idam3 for testing
                PGresult* DBQuery = NULL;

                if (PQstatus(DBConnect) != CONNECTION_OK) {
                    PQfinish(DBConnect);
                    idamErrorAndLog("geom: Connection to mastgeom database failed. %s\n", 999, &err);
                    break;
                }

                char* signal_for_query = (char*)malloc((2 * strlen(signal) + 1) * sizeof(char));
                PQescapeStringConn(DBConnect, signal_for_query, signal, strlen(signal), &err);

                ///////////////////
                // Construct query to extract filename of the file that needs to be read in
                char query[MAXSQL];

                if (isConfig == 1) {
                    // configuration files
                    sprintf(query,
                            "SELECT cds.file_name, ggm.geomsignal_alias, cds.version"
                                    " FROM config_data_source cds, geomgroup_geomsignal_map ggm"
                                    " WHERE cds.config_data_source_id=ggm.config_data_source_id"
                                    "   AND lower(ggm.geomsignal_alias) LIKE lower('%s/%%')"
                                    "   AND cds.start_shot<=%d AND cds.end_shot>%d",
                            signal, shot, shot);

                    // Add check for toroidal angle
                    if (three_d == 1) {
                        strcat(query, " AND cds.tor_angle=NULL;");
                    } else if (tor_angle >= 0) {
                        char tor_angle_statement[MAXSQL];
                        sprintf(tor_angle_statement,
                                " AND (CASE WHEN cds.tor_angle_dependent=TRUE THEN round(cds.tor_angle::numeric, 4)=round(%f, 4) ELSE TRUE END)",
                                tor_angle);
                        strcat(query, tor_angle_statement);
                    } else {
                        strcat(query, " AND cds.tor_angle_dependent=FALSE ");
                    }

                } else {
                    // calibration files
                    sprintf(query,
                            "SELECT cds.file_name, ggm.geomsignal_alias, cds.version"
                                    " FROM cal_data_source cds"
                                    " INNER JOIN config_data_source cods"
                                    "   ON cods.config_data_source_id=cds.config_data_source_id"
                                    " INNER JOIN geomgroup_geomsignal_map ggm"
                                    "   ON ggm.config_data_source_id=cods.config_data_source_id"
                                    " WHERE lower(ggm.geomsignal_alias) LIKE lower('%s/%%')"
                                    "   AND cds.start_shot<=%d AND cds.end_shot>%d",
                            signal, shot, shot);

                    // Add check for toroidal angle
                    if (three_d == 1) {
                        strcat(query, " AND cds.tor_angle=NULL;");
                    } else if (tor_angle >= 0) {
                        char tor_angle_statement[MAXSQL];
                        sprintf(tor_angle_statement,
                                " AND (CASE WHEN cods.tor_angle_dependent=TRUE THEN round(cds.tor_angle::numeric, 4)=round(%f, 4) ELSE TRUE END)",
                                tor_angle);
                        strcat(query, tor_angle_statement);
                    } else {
                        strcat(query, " AND cods.tor_angle_dependent=FALSE ");
                    }
                }

                // Add version check
                if (isConfig == 1 && versionConfig >= 0) {
                    char ver_str[2];
                    sprintf(ver_str, "%d", versionConfig);
                    strcat(query, " AND cds.version=%d;");
                } else if (isCal == 1 && versionCal >= 0) {
                    char ver_str[2];
                    sprintf(ver_str, "%d", versionCal);
                    strcat(query, " AND cds.version=");
                    strcat(query, ver_str);
                    strcat(query, ";");
                } else {
                    strcat(query, " ORDER BY cds.version DESC LIMIT 1;");
                }

                idamLog(LOG_DEBUG, "geom: query is %s\n", query);

                /////////////////////////////
                // Query database
                if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                    idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                    break;
                }

                if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                    PQclear(DBQuery);
                    PQfinish(DBConnect);
                    idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                    break;
                }

                // Retrieve number of rows found in query
                int nRows = PQntuples(DBQuery);

                /////////////////////////////
                // No rows found?
                // If they've asked for a specific toroidal angle then try to generate geom from the CAD.
                if (nRows == 0) {
                    idamLog(LOG_DEBUG, "geom: no rows. isConfig %d, three_d %d, tor_angle %f\n", isConfig, three_d,
                            tor_angle);
                    // If 2D configuration was asked for, and a toroidal angle was specified, then try to generate the geometry
                    // file for this toroidal angle from the CAD (this is just a dummy script for now).
                    if (isConfig == 1 && three_d != 1 && tor_angle >= 0) {
                        PQclear(DBQuery);
                        int ret = generateGeom(signal, tor_angle, DBConnect, DBQuery);

                        if (ret != 0) {
                            PQclear(DBQuery);
                            PQfinish(DBConnect);
                            break;
                        }

                        // For the moment this doesn't actually make anything... need to query again
                        // to check signals & file are now in db
                        if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                            idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                            break;
                        }

                        if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                            PQclear(DBQuery);
                            PQfinish(DBConnect);
                            idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                            break;
                        }

                        int nRows_check = PQntuples(DBQuery);

                        if (nRows_check == 0) {
                            PQclear(DBQuery);
                            PQfinish(DBConnect);
                            idamErrorAndLog("geom: No rows were found in database matching query\n", 999, &err);
                            break;
                        }

                        PQfinish(DBConnect);
                        break;
                    } else {
                        PQclear(DBQuery);
                        PQfinish(DBConnect);
                        idamErrorAndLog("geom: No rows were found in database matching query\n", 999, &err);
                        break;
                    }
                }

                /////////////////////////////
                // We have found a matching file, extract the filename
                int s_file = PQfnumber(DBQuery, "file_name");
                int s_alias = PQfnumber(DBQuery, "geomsignal_alias");

                char* file_db;
                if (!PQgetisnull(DBQuery, 0, s_file)) {
                    file_db = PQgetvalue(DBQuery, 0, s_file);

                    idamLog(LOG_DEBUG, "geom: file from db %s\n", file_db);

                    //Add on the location
                    char* file_path = getenv("MAST_GEOM_DATA");

                    if (file_path == NULL) {
                        idamErrorAndLog("geom: Could not retrieve MAST_GEOM_DATA environment variable\n", 999, &err);
                        break;
                    }

                    if (isFile == 0) {
                        file = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 2));
                        strcpy(file, file_path);
                        strcat(file, "/");
                        strcat(file, file_db);
                    }

                    idamLog(LOG_DEBUG, "geom: file_path %s\n", file);
                }

                // If the signal matches that asked for by the user, then this means that this
                // is the signal itself. If it does not match, then we are at the group level
                // This info is useful for the wrappers
                if (!PQgetisnull(DBQuery, 0, s_alias)) {
                    char* sigAlias = PQgetvalue(DBQuery, 0, s_alias);

                    // Need to ignore last character of sigAlias, which will be a /
                    if (!strncmp(sigAlias, signal_for_query, strlen(sigAlias) - 1)) {
                        signal_type = (char*)malloc(sizeof(char) * 7 + 1);
                        strcpy(signal_type, "element");
                    } else {
                        signal_type = (char*)malloc(sizeof(char) * 5 + 1);
                        strcpy(signal_type, "group");
                    }
                }

                idamLog(LOG_DEBUG, "geom: Close connection\n");

                //Close db connection
                PQclear(DBQuery);
                PQfinish(DBConnect);
                free(signal_for_query);
            } else {
                signal_type = "a";
            }

            idamLog(LOG_DEBUG, "geom: signal_type %s\n", signal_type);

            ///////////////////////////
            // Setup signal and file path in signal_desc
            strcpy(signal_desc->signal_name, signal);
            strcpy(data_source->path, file);

            /////////////////////////////
            // Read in the file
            idamLog(LOG_DEBUG, "geom: Calling readCDF to retrieve file %s with signal %s\n", file, signal);
            DATA_BLOCK data_block_file;
            initDataBlock(&data_block_file);

            int errConfig = readCDF(*data_source, *signal_desc, *request_block, &data_block_file);

            idamLog(LOG_DEBUG, "geom: Read in file\n");

            if (errConfig != 0) {
                idamErrorAndLog("Error reading geometry data!", 999, &err);
                break;
            }

            if (data_block_file.data_type != TYPE_COMPOUND) {
                idamErrorAndLog("Non-structured type returned from data reader!", 999, &err);
                break;
            }

            /////////////////////////////
            // Combine datablock and signal_type into one structure
            // to be returned
            USERDEFINEDTYPE* udt = data_block_file.opaque_block;

            struct DATAPLUSTYPE {
                void* data;
                char* signal_type;
            };
            typedef struct DATAPLUSTYPE DATAPLUSTYPE;

            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATAPLUSTYPE");
            strcpy(parentTree.source, "netcdf");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATAPLUSTYPE);

            //Compound field for calibration file
            initCompoundField(&field);
            strcpy(field.name, "data");
            field.atomictype = TYPE_UNKNOWN;
            strcpy(field.type, udt->name);
            strcpy(field.desc, "data");

            field.pointer = 1;
            field.count = 1;
            field.rank = 0;
            field.shape = NULL;            // Needed when rank >= 1

            field.size = field.count * sizeof(void*);
            field.offset = newoffset(offset, field.type);
            field.offpad = padding(offset, field.type);
            field.alignment = getalignmentof(field.type);
            offset = field.offset + field.size;    // Next Offset
            addCompoundField(&parentTree, field);

            //For signal_type
            initCompoundField(&field);
            defineField(&field, "signal_type", "signal_type", &offset, SCALARSTRING);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            DATAPLUSTYPE* data;
            size_t stringLength = strlen(signal_type) + 1;
            data = (DATAPLUSTYPE*)malloc(sizeof(DATAPLUSTYPE));
            data->signal_type = (char*)malloc(stringLength * sizeof(char));
            data->data = data_block_file.data;
            strcpy(data->signal_type, signal_type);

            addMalloc((void*)data, 1, sizeof(DATAPLUSTYPE), "DATAPLUSTYPE");
            addMalloc((void*)data->signal_type, 1, stringLength * sizeof(char), "char");

            //Return data
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "Data plus type");
            strcpy(data_block->data_label, "Data plus type");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType("DATAPLUSTYPE", 0);

            // Free heap data associated with the two DATA_BLOCKS
            // Nothing to free?
            data_block_file.data = NULL;

            break;
        } else if (STR_EQUALS(request_block->function, "getSignalFile")) {

            ////////////////
            // Retrieve user inputs
            char* signal = NULL;
            char* file = NULL;

            int version = -1;
            int shot = request_block->exp_number;
            int keep_all = 0;

            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "signal")) {
                    signal = request_block->nameValueList.nameValue[i_arg].value;
                    idamLog(LOG_DEBUG, "configPlugin: Using signal name: %s\n", signal);
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "version")) {
                    if (IsNumber(request_block->nameValueList.nameValue[i_arg].value)) {
                        version = atoi(request_block->nameValueList.nameValue[i_arg].value);
                    }
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "keep_all")) {
                    keep_all = 1;
                }
            }

            //////////////////////////////
            // Open the connection
            // CURRENTLY HARDCODED IN WHILE I'M TESTING
            // .... Once this is actually in the new MAST-U db, will need to use idam functions as in readMeta to open connection.
            idamLog(LOG_DEBUG, "geom: trying to get connection\n");
            //	PGconn* DBConnect = PQconnectdb("dbname=mastgeom user=mastgeom password=mastgeom"); // local repo for testing
            PGconn* DBConnect = PQconnectdb(
                    "dbname=idam user=idam password=idam@idam3 host=idam3.mast.ccfe.ac.uk port=60000"); // idam3 for testing/dev version
            PGresult* DBQuery = NULL;

            if (PQstatus(DBConnect) != CONNECTION_OK) {
                PQfinish(DBConnect);
                idamErrorAndLog("geom: Connection to mastgeom database failed. %s\n", 999, &err);
                break;
            }

            char* signal_for_query = (char*)malloc((2 * strlen(signal) + 1) * sizeof(char));
            PQescapeStringConn(DBConnect, signal_for_query, signal, strlen(signal), &err);

            idamLog(LOG_DEBUG, "signal_for_query %s\n", signal_for_query);

            /////////////////////
            // Query to find filename containing the data signal asked for
            char query[MAXSQL];

            if (version < 0) {
                sprintf(query,
                        "SELECT dgs.file_name, dgs.version, dgsm.signal_alias, dgsm.signal_desc_id FROM datasignal_geomdata_source dgs, datasignal_geomdata_source_map dgsm WHERE dgsm.signal_alias LIKE lower('%s/%%') OR dgsm.var_name LIKE lower('%s/%%') AND dgs.start_shot<=%d AND dgs.end_shot>%d AND dgsm.datasignal_geomdata_source_id=dgs.datasignal_geomdata_source_id AND dgs.version=(SELECT max(datasignal_geomdata_source.version) FROM datasignal_geomdata_source, datasignal_geomdata_source_map WHERE datasignal_geomdata_source_map.signal_alias LIKE lower('%s/%%') OR datasignal_geomdata_source_map.var_name LIKE lower('%s/%%') AND datasignal_geomdata_source.start_shot<=%d AND datasignal_geomdata_source.end_shot>%d AND datasignal_geomdata_source.datasignal_geomdata_source_id=datasignal_geomdata_source_map.datasignal_geomdata_source_id);",
                        signal_for_query, signal_for_query, shot, shot, signal_for_query, signal_for_query, shot, shot);
            } else {
                sprintf(query,
                        "SELECT dgs.file_name, dgs.version, dgsm.signal_alias, dgsm.signal_desc_id FROM datasignal_geomdata_source dgs, datasignal_geomdata_source_map dgsm WHERE dgsm.signal_alias LIKE lower('%s/%%') OR dgsm.var_name LIKE lower('%s/%%') AND dgs.start_shot<=%d AND dgs.end_shot>%d AND dgsm.datasignal_geomdata_source_id=dgs.datasignal_geomdata_source_id AND dgs.version=%d;",
                        signal_for_query, signal_for_query, shot, shot, version);
            }

            idamLog(LOG_DEBUG, "geom: query is %s\n", query);

            if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                break;
            }

            int nRows = PQntuples(DBQuery);
            idamLog(LOG_DEBUG, "geom: nRows returned from db : %d\n", nRows);

            if (nRows == 0) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                idamErrorAndLog("geom: No rows were found in database matching query\n", 999, &err);
                break;
            }

            char** all_sig_alias = (char**)malloc((nRows) * sizeof(char*));
            int* all_sig_id = (int*)malloc((nRows) * sizeof(int));
            int* is_available = (int*)malloc((nRows) * sizeof(int));

            int s_file = PQfnumber(DBQuery, "file_name");
            int s_sig_alias = PQfnumber(DBQuery, "signal_alias");
            int s_sig_id = PQfnumber(DBQuery, "signal_desc_id");

            // From the first row: get the filename, and check what type we are dealing with
            // (ie. is this the signal itself? or a group?)
            // For all rows, retrieve signal aliases & ids
            char* file_path = getenv("MAST_GEOM_DATA");
            char* file_db;
            char* signal_type = NULL;

            int i = 0;

            for (i = 0; i < nRows; i++) {
                // filename (1st row only)
                if (i == 0) {
                    if (!PQgetisnull(DBQuery, i, s_file)) {
                        file_db = PQgetvalue(DBQuery, 0, s_file);
                        file = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 1));
                        strcpy(file, file_path);
                        strcat(file, file_db);
                    }
                }

                // sig ids
                if (!PQgetisnull(DBQuery, i, s_sig_id)) {
                    all_sig_id[i] = atoi(PQgetvalue(DBQuery, i, s_sig_id));
                } else {
                    all_sig_id[i] = -1;
                }
                is_available[i] = 0;

                // signal alias
                if (!PQgetisnull(DBQuery, i, s_sig_alias)) {
                    char* sigAlias = PQgetvalue(DBQuery, i, s_sig_alias);
                    all_sig_alias[i] = (char*)malloc(sizeof(char) * (strlen(sigAlias) + 1));
                    strcpy(all_sig_alias[i], sigAlias);

                    // signal type (1st row only)
                    if (i == 0) {
                        if (!strncmp(sigAlias, signal_for_query, strlen(sigAlias) - 1)) {
                            signal_type = (char*)malloc(sizeof(char) * 7 + 1);
                            strcpy(signal_type, "element");
                        } else {
                            signal_type = (char*)malloc(sizeof(char) * 5 + 1);
                            strcpy(signal_type, "group");
                        }
                    }
                }
            }

            //Close db connection
            PQclear(DBQuery);
            PQfinish(DBConnect);
            free(signal_for_query);

            /////////////////////////////////
            // Check which signal aliases are actually available for this shot, in the IDAM db
            int n_signals_available = checkAvailableSignals(shot, nRows, &all_sig_id, &is_available);

            idamLog(LOG_DEBUG, "geom: n sig available %d\n", n_signals_available);

            if (n_signals_available == 0 && keep_all == 0) {
                idamErrorAndLog("geom: None of the signals in this file are available for this shot.\n", 999, &err);
                break;
            }

            /////////////////////////////////
            // Read in the file
            strcpy(signal_desc->signal_name, signal);
            strcpy(data_source->path, file);

            DATA_BLOCK data_block_file;
            initDataBlock(&data_block_file);

            err = readCDF(*data_source, *signal_desc, *request_block, &data_block_file);

            idamLog(LOG_DEBUG, "geom: Read in file signal %s\n", signal_desc->signal_name);

            if (err != 0) {
                idamErrorAndLog("Error reading geometry data!", 999, &err);
                break;
            }

            if (data_block_file.data_type != TYPE_COMPOUND) {
                idamErrorAndLog("Non-structured type returned from data reader!", 999, &err);
                break;
            }

            /////////////////////////////
            // Combine datablock and signal_type into one structure
            // to be returned
            USERDEFINEDTYPE* udt = data_block_file.opaque_block;

            struct DATAPLUSTYPE {
                void* data;
                char* signal_type;
                char** signal_alias_available;
            };
            typedef struct DATAPLUSTYPE DATAPLUSTYPE;

            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATAPLUSTYPE");
            strcpy(parentTree.source, "netcdf");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATAPLUSTYPE);

            //Compound field for calibration file
            initCompoundField(&field);
            strcpy(field.name, "data");
            field.atomictype = TYPE_UNKNOWN;
            strcpy(field.type, udt->name);
            strcpy(field.desc, "data");

            field.pointer = 1;
            field.count = 1;
            field.rank = 0;
            field.shape = NULL;            // Needed when rank >= 1

            field.size = field.count * sizeof(void*);
            field.offset = newoffset(offset, field.type);
            field.offpad = padding(offset, field.type);
            field.alignment = getalignmentof(field.type);
            offset = field.offset + field.size;    // Next Offset
            addCompoundField(&parentTree, field);

            // For signal_type
            initCompoundField(&field);
            defineField(&field, "signal_type", "signal_type", &offset, SCALARSTRING);
            addCompoundField(&parentTree, field);

            // Available signal aliases
            initCompoundField(&field);
            defineField(&field, "signal_alias_available", "signal_alias_available", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            DATAPLUSTYPE* data;

            size_t stringLength = strlen(signal_type) + 1;
            data = (DATAPLUSTYPE*)malloc(sizeof(DATAPLUSTYPE));
            data->signal_type = (char*)malloc(stringLength * sizeof(char));
            data->data = data_block_file.data;
            strcpy(data->signal_type, signal_type);

            addMalloc((void*)data, 1, sizeof(DATAPLUSTYPE), "DATAPLUSTYPE");
            addMalloc((void*)data->signal_type, 1, stringLength * sizeof(char), "char");

            if (n_signals_available > 0) {
                data->signal_alias_available = (char**)malloc(n_signals_available * sizeof(char*));
                int i_avail = 0;
                for (i = 0; i < nRows; i++) {
                    if (is_available[i] == 1) {
                        size_t strLength = strlen(all_sig_alias[i]) + 1;
                        data->signal_alias_available[i_avail] = (char*)malloc(strLength * sizeof(char));
                        addMalloc(data->signal_alias_available[i_avail], (int)strLength, sizeof(char), "char");
                        strcpy(data->signal_alias_available[i_avail], all_sig_alias[i]);
                        i_avail = i_avail + 1;
                    }
                    if (i_avail > n_signals_available - 1) {
                        break;
                    }
                }
                addMalloc((void*)data->signal_alias_available, n_signals_available, sizeof(char*), "STRING *");
            } else {
                data->signal_alias_available = (char**)malloc(sizeof(char*));
                data->signal_alias_available[0] = (char*)malloc(5 * sizeof(char));
                strcpy(data->signal_alias_available[0], "None");
                addMalloc(data->signal_alias_available[0], 5, sizeof(char), "char");
                addMalloc((void*)data->signal_alias_available, 1, sizeof(char*), "STRING *");
            }

            //Return data
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*)data;

            strcpy(data_block->data_desc, "Data plus type");
            strcpy(data_block->data_label, "Data plus type");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType("DATAPLUSTYPE", 0);

            // Free heap data associated with the two DATA_BLOCKS
            // Nothing to free?
            data_block_file.data = NULL;

            break;

        } else if (STR_EQUALS(request_block->function, "getSignalFilename")) {

            ///////////////////
            // getSignalFilename : retrieve name of filename containing information about
            //                    data signals associated with a particular geomsignal group or element.
            //     Arguments:
            //         - geomsignal : Group or signal for which to retrieve corresponding signal filename with signal info
            //         - version : file version number. If not set then the latest will be returned.
            ///////////////////

            /////////////
            // Retrieve user inputs
            char* signal = NULL;

            int shot = request_block->exp_number;

            int versionDependent = -1;

            int n_args = request_block->nameValueList.pairCount;
            int i_arg;

            // Retrive arguements provided by the user
            for (i_arg = 0; i_arg < n_args; i_arg++) {
                if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "geomsignal")) {
                    signal = request_block->nameValueList.nameValue[i_arg].value;
                    idamLog(LOG_DEBUG, "configPlugin: Using signal name: %s\n", signal);
                } else if (STR_IEQUALS(request_block->nameValueList.nameValue[i_arg].name, "version")) {
                    if (IsNumber(request_block->nameValueList.nameValue[i_arg].value)) {
                        versionDependent = i_arg;
                    }
                }
            }

            // Struct to put geom aliases and signal aliases, signal ids into
            struct DATASTRUCTFULL {
                char** file_name;
                char** geom_alias;
                char** signal_alias;
                char** var_name;
                int* signal_id;
                int* is_available;
            };
            typedef struct DATASTRUCTFULL DATASTRUCTFULL;

            DATASTRUCTFULL* data;
            data = (DATASTRUCTFULL*)malloc(sizeof(DATASTRUCTFULL));

            char* file_path = getenv("MAST_GEOM_DATA");

            ////////////////////
            // Query to find data signals and filename associated with given geom group
            idamLog(LOG_DEBUG, "geom: trying to get connection\n");
            PGconn* DBConnect = PQconnectdb(
                    "dbname=idam user=idam password=idam@idam3 host=idam3.mast.ccfe.ac.uk port=60000");
            //	PGconn* DBConnect = PQconnectdb("dbname=mastgeom user=mastgeom password=mastgeom");
            PGresult* DBQuery = NULL;

            if (PQstatus(DBConnect) != CONNECTION_OK) {
                idamLog(LOG_DEBUG, "geom: Connection to mastgeom database failed. %s\n", PQerrorMessage(DBConnect));
                PQfinish(DBConnect);
                idamErrorAndLog("Error connecting to geomtry db!", 999, &err);
                break;
            }

            char* signal_for_query = (char*)malloc((2 * strlen(signal) + 1) * sizeof(char));
            PQescapeStringConn(DBConnect, signal_for_query, signal, strlen(signal), &err);

            idamLog(LOG_DEBUG, "signal_for_query %s\n", signal_for_query);

            char query[MAXSQL];

            sprintf(query,
                    "SELECT geomgroup_geomsignal_map.geomsignal_alias, datasignal_geomdata_source.file_name, datasignal_geomdata_source_map.signal_alias, datasignal_geomdata_source_map.var_name, datasignal_geomdata_source_map.signal_desc_id, datasignal_geomdata_source.version FROM datasignal_geomdata_source INNER JOIN datasignal_geomdata_source_map ON datasignal_geomdata_source.datasignal_geomdata_source_id=datasignal_geomdata_source_map.datasignal_geomdata_source_id INNER JOIN datasignal_geomsignal_map ON datasignal_geomsignal_map.signal_desc_id=datasignal_geomdata_source_map.signal_desc_id INNER JOIN geomgroup_geomsignal_map ON geomgroup_geomsignal_map.geomgroup_geomsignal_map_id=datasignal_geomsignal_map.geomgroup_geomsignal_map_id INNER JOIN config_data_source ON config_data_source.config_data_source_id=geomgroup_geomsignal_map.config_data_source_id WHERE config_data_source.start_shot<=%d AND config_data_source.end_shot>%d AND datasignal_geomdata_source.start_shot<=%d AND datasignal_geomdata_source.end_shot>%d AND geomgroup_geomsignal_map.geomsignal_alias LIKE lower('%s/%%') AND config_data_source.version=(SELECT max(config_data_source.version) FROM config_data_source WHERE config_data_source.start_shot<=%d AND config_data_source.end_shot>%d)",
                    shot, shot, shot, shot, signal_for_query, shot, shot);

            if (versionDependent >= 0) {
                strcat(query, " AND dgs.version=");
                strcat(query, request_block->nameValueList.nameValue[versionDependent].value);
            } else {
                char version_statement[MAXSQL];
                sprintf(version_statement,
                        " AND datasignal_geomdata_source.version=(SELECT max(datasignal_geomdata_source.version) FROM datasignal_geomdata_source WHERE datasignal_geomdata_source.start_shot<=%d AND datasignal_geomdata_source.end_shot>%d)",
                        shot, shot);
                strcat(query, version_statement);
            }
            strcat(query, ";");

            idamLog(LOG_DEBUG, "geom: query is %s\n", query);

            if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
                idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                break;
            }

            if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                idamErrorAndLog("geom: Database query failed.\n", 999, &err);
                break;
            }

            int nRows = PQntuples(DBQuery);

            if (nRows == 0) {
                PQclear(DBQuery);
                PQfinish(DBConnect);
                idamErrorAndLog("geom: No rows were found in database matching query\n", 999, &err);
                break;
            }

            //Setup geom_alias and signal_alias fields in the data struct
            data->file_name = (char**)malloc((nRows) * sizeof(char*));
            data->geom_alias = (char**)malloc((nRows) * sizeof(char*));
            data->signal_alias = (char**)malloc((nRows) * sizeof(char*));
            data->var_name = (char**)malloc((nRows) * sizeof(char*));
            data->signal_id = (int*)malloc((nRows) * sizeof(int));
            data->is_available = (int*)malloc((nRows) * sizeof(int));

            int s_id = PQfnumber(DBQuery, "signal_desc_id");
            int s_geom_alias = PQfnumber(DBQuery, "geomsignal_alias");
            int s_sig_alias = PQfnumber(DBQuery, "signal_alias");
            int s_var_name = PQfnumber(DBQuery, "var_name");
            int s_file = PQfnumber(DBQuery, "file_name");

            int i = 0;

            for (i = 0; i < nRows; i++) {
                data->is_available[i] = 0;

                // Signal id
                if (!PQgetisnull(DBQuery, i, s_id)) {
                    data->signal_id[i] = atoi(PQgetvalue(DBQuery, i, s_id));
                } else {
                    data->signal_id[i] = -1;
                }

                // Geom alias
                if (!PQgetisnull(DBQuery, i, s_geom_alias)) {
                    size_t stringLength = strlen(PQgetvalue(DBQuery, i, s_geom_alias)) + 1;
                    data->geom_alias[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->geom_alias[i], PQgetvalue(DBQuery, i, s_geom_alias));
                }

                // Signal alias
                if (!PQgetisnull(DBQuery, i, s_sig_alias)) {
                    size_t stringLength = strlen(PQgetvalue(DBQuery, i, s_sig_alias)) + 1;
                    data->signal_alias[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->signal_alias[i], PQgetvalue(DBQuery, i, s_sig_alias));
                }

                // Varname
                if (!PQgetisnull(DBQuery, i, s_sig_alias)) {
                    size_t stringLength = strlen(PQgetvalue(DBQuery, i, s_var_name)) + 1;
                    data->var_name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->var_name[i], PQgetvalue(DBQuery, i, s_var_name));
                }

                // File name
                if (!PQgetisnull(DBQuery, i, s_file)) {
                    size_t stringLength = strlen(PQgetvalue(DBQuery, i, s_file)) + strlen(file_path) + 1;
                    data->file_name[i] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data->file_name[i], file_path);
                    strcat(data->file_name[i], PQgetvalue(DBQuery, i, s_file));
                }
            }

            //Close db connection
            PQclear(DBQuery);
            PQfinish(DBConnect);
            free(signal_for_query);

            /////////
            // Check which signal id s are available for this exp number,
            /////////
            int n_signals_available = checkAvailableSignals(shot, nRows, &data->signal_id, &data->is_available);

            if (n_signals_available == 0) {
                idamErrorAndLog("geom: None of the signals for this geometry component are available for this shot\n",
                                999, &err);
                break;
            }

            /////////////////////
            // Only keep filenames and other info for those data signals that are available for the requested shot.
            // Copy info into struct that will be returned to user
            struct DATASTRUCT {
                char** filenames;
                char** geom_alias;
                char** signal_alias;
                char** var_name;
            };
            typedef struct DATASTRUCT DATASTRUCT;

            DATASTRUCT* data_out;
            data_out = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
            addMalloc((void*)data_out, 1, sizeof(DATASTRUCT), "DATASTRUCT");

            data_out->filenames = (char**)malloc((n_signals_available) * sizeof(char*));
            addMalloc((void*)data_out->filenames, n_signals_available, sizeof(char*), "STRING *");
            data_out->geom_alias = (char**)malloc((n_signals_available) * sizeof(char*));
            addMalloc((void*)data_out->geom_alias, n_signals_available, sizeof(char*), "STRING *");
            data_out->signal_alias = (char**)malloc((n_signals_available) * sizeof(char*));
            addMalloc((void*)data_out->signal_alias, n_signals_available, sizeof(char*), "STRING *");
            data_out->var_name = (char**)malloc((n_signals_available) * sizeof(char*));
            addMalloc((void*)data_out->var_name, n_signals_available, sizeof(char*), "STRING *");

            // Transfer data to data_out
            int out_index = 0;
            for (i = 0; i < nRows; i++) {
                if (data->is_available[i] > 0) {
                    size_t stringLength = strlen(data->file_name[i]) + 1;
                    data_out->filenames[out_index] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data_out->filenames[out_index], data->file_name[i]);
                    addMalloc((void*)data_out->filenames[out_index], (int)stringLength, sizeof(char), "char");

                    stringLength = strlen(data->geom_alias[i]) + 1;
                    data_out->geom_alias[out_index] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data_out->geom_alias[out_index], data->geom_alias[i]);
                    addMalloc((void*)data_out->geom_alias[out_index], (int)stringLength, sizeof(char), "char");

                    stringLength = strlen(data->signal_alias[i]) + 1;
                    data_out->signal_alias[out_index] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data_out->signal_alias[out_index], data->signal_alias[i]);
                    addMalloc((void*)data_out->signal_alias[out_index], (int)stringLength, sizeof(char), "char");

                    stringLength = strlen(data->var_name[i]) + 1;
                    data_out->var_name[out_index] = (char*)malloc(stringLength * sizeof(char));
                    strcpy(data_out->var_name[out_index], data->var_name[i]);
                    addMalloc((void*)data_out->var_name[out_index], (int)stringLength, sizeof(char), "char");

                    out_index = out_index + 1;
                }
            }

            //////////////////////////////
            /// Need to free memory in temporary data struct
            //////////////////////////////
            free(data->geom_alias);
            free(data->signal_alias);
            free(data->var_name);
            free(data->signal_id);
            free(data->is_available);
            free(data);

            USERDEFINEDTYPE parentTree;
            COMPOUNDFIELD field;
            int offset = 0;

            //////////////////
            //User defined type to describe data structure
            initUserDefinedType(&parentTree);
            parentTree.idamclass = TYPE_COMPOUND;
            strcpy(parentTree.name, "DATASTRUCT");
            strcpy(parentTree.source, "netcdf");
            parentTree.ref_id = 0;
            parentTree.imagecount = 0;
            parentTree.image = NULL;
            parentTree.size = sizeof(DATASTRUCT);

            //Compound field for calibration file
            initCompoundField(&field);
            strcpy(field.name, "filenames");
            defineField(&field, "filenames", "filenames", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "geom_alias");
            defineField(&field, "geom_alias", "geom_alias", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "signal_alias");
            defineField(&field, "signal_alias", "signal_alias", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            initCompoundField(&field);
            strcpy(field.name, "var_name");
            defineField(&field, "var_name", "var_name", &offset, ARRAYSTRING);
            addCompoundField(&parentTree, field);

            addUserDefinedType(userdefinedtypelist, parentTree);

            // Put file name into signal block, to return to user.
            initDataBlock(data_block);

            data_block->data_type = TYPE_COMPOUND;
            data_block->rank = 0;            // Scalar structure (don't need a DIM array)
            data_block->data_n = 1;
            data_block->data = (char*)data_out;

            strcpy(data_block->data_desc, "Filenames");
            strcpy(data_block->data_label, "Filenames");
            strcpy(data_block->data_units, "");

            data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
            data_block->opaque_count = 1;
            data_block->opaque_block = (void*)findUserDefinedType("DATASTRUCT", 0);

        } else {
            //======================================================================================
            // Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "geom", err, "Unknown function requested!");
            break;
        }

    } while (0);

    //--------------------------------------------------------------------------------------
    // Housekeeping

    return err;
}
	 
