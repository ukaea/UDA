#include "geomConfig.h"

#include <unistd.h>
#include <tgmath.h>

#include <clientserver/initStructs.h>
#include <plugins/pluginUtils.h>
#include <structures/accessors.h>
#include <structures/struct.h>

int do_meta(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect)
{

    const char* file = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, file);

    const char* geomgroup = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, geomgroup);

    int isConfig = findValue(&idam_plugin_interface->request_block->nameValueList, "config");
    int isCal = findValue(&idam_plugin_interface->request_block->nameValueList, "cal");

    float version = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, version);

    int ver = -1;
    int rev = -1;
    if (version >= 0) {
        ver = (int)floor(version);
        rev = (int)floor(version * 10);
        UDA_LOG(UDA_LOG_DEBUG, "Version %d, Revision %d", ver, rev);
    }

    int shot = idam_plugin_interface->request_block->exp_number;
    UDA_LOG(UDA_LOG_DEBUG, "Exp number %d\n", shot);

    if (file == NULL && !isCal && !isConfig) {
        RAISE_PLUGIN_ERROR("Filename wasn't given and didn't request configuration or calibration data.\n");
    }

    if (file == NULL && (geomgroup == NULL || shot == 0)) {
        RAISE_PLUGIN_ERROR("You must either specify a file OR signal name and shot number.\n")
    }

    // Find filename if not given
    if (file == NULL) {
        PGresult* DBQuery = NULL;

        if (PQstatus(DBConnect) != CONNECTION_OK) {
            PQfinish(DBConnect);
            RAISE_PLUGIN_ERROR("Connection to mastgeom database failed.\n");
        }

        char* group_for_query = (char*)malloc((2 * strlen(geomgroup) + 1) * sizeof(char));
        int err = 0;
        PQescapeStringConn(DBConnect, group_for_query, geomgroup, strlen(geomgroup), &err);

        ///////////////////
        // Construct query to extract filename of the file that needs to be read in
        char query[MAXSQL];

        // Configuration files
        // For calibration, we want to check what the configuration file is, that it contains
        // the signal requested & then find the corresponding calibration file later
        sprintf(query,
                "SELECT cds.file_name, cds.config_data_source_id"
                        " FROM config_data_source cds"
                        " WHERE lower(cds.geomgroup)=lower('%s')"
                        "   AND cds.start_shot<=%d AND cds.end_shot>%d",
                group_for_query, shot, shot);

        // Add version check
        if (ver >= 0) {
            char ver_str[2];
            sprintf(ver_str, "%d", ver);
            char rev_str[2];
            sprintf(rev_str, "%d", rev);

            strcat(query, " AND cds.version=");
            strcat(query, ver_str);
            strcat(query, " AND cds.revision=");
            strcat(query, rev_str);
        } else {
            strcat(query, " ORDER BY cds.version DESC, cds.revision DESC LIMIT 1;");
        }

        UDA_LOG(UDA_LOG_DEBUG, "query is %s\n", query);

        /////////////////////////////
        // Query database
        if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
            RAISE_PLUGIN_ERROR("Database query failed.\n");
        }

        if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
            PQclear(DBQuery);
            RAISE_PLUGIN_ERROR("Database query failed.\n");
        }

        // Retrieve number of rows found in query
        int nRows = PQntuples(DBQuery);

        /////////////////////////////
        // No rows found?
        // For the future: if they've asked for a specific toroidal angle then try to generate geom from the CAD.
        //                 see commit 91ea8223480cf28228caf30cb43d05fa1c9947db for prototype
        if (nRows == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "no rows. isConfig %d\n", isConfig);

            PQclear(DBQuery);
            RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
        }

        /////////////////////////////
        // We have found a matching file, extract the filename and signal names
        int s_file = PQfnumber(DBQuery, "file_name");
        int s_id = PQfnumber(DBQuery, "config_data_source_id");

        int file_id = -1;
        if (!PQgetisnull(DBQuery, 0, s_id)) {
            file_id = atoi(PQgetvalue(DBQuery, 0, s_id));
        }

        char* file_db;

        // Only retrieve filename if config data was asked for.
        // Otherwise, we will want to retrieve cal filename corresponding to the config later
        if (!PQgetisnull(DBQuery, 0, s_file) && isConfig) {
            file_db = PQgetvalue(DBQuery, 0, s_file);

            UDA_LOG(UDA_LOG_DEBUG, "file from db %s\n", file_db);

            //Add on the location
            char* file_path = getenv("MAST_GEOM_DATA");

            if (file_path == NULL) {
                RAISE_PLUGIN_ERROR("Could not retrieve MAST_GEOM_DATA environment variable\n");
            }

            if (file == NULL) {
                char* filename = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 2));
                strcpy(filename, file_path);
                strcat(filename, "/");
                strcat(filename, file_db);
                file = filename;
            }

            UDA_LOG(UDA_LOG_DEBUG, "file_path %s\n", file);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Clear db query\n");

        //Close db connection
        PQclear(DBQuery);

        //////////////////////////
        // If this is calibration, get cal filename
        if (isCal) {
            float version_cal = -1;
            FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, version_cal);

            int ver_cal = -1;
            int rev_cal = -1;
            if (version >= 0) {
                ver_cal = (int)floor(version_cal);
                rev_cal = (int)floor(version_cal * 10);
                UDA_LOG(UDA_LOG_DEBUG, "Cal Version %d, Revision %d", ver, rev);
            }

            PGresult* DBQuery_cal = NULL;
            char query_cal[MAXSQL];

            sprintf(query_cal,
                    "SELECT file_name FROM cal_data_source "
                            " WHERE config_data_source_id=%d "
                            "AND start_shot <= %d "
                            "AND end_shot > %d",
                    file_id, shot, shot);

            // Add version check
            if (ver_cal >= 0) {
                char ver_str[2];
                sprintf(ver_str, "%d", ver_cal);
                char rev_str[2];
                sprintf(rev_str, "%d", rev_cal);

                strcat(query, " AND version=");
                strcat(query, ver_str);
                strcat(query, " AND revision=");
                strcat(query, rev_str);
            } else {
                strcat(query, " ORDER BY version DESC, revision DESC LIMIT 1;");
            }

            UDA_LOG(UDA_LOG_DEBUG, "Cal file query %s\n", query_cal);

            if ((DBQuery_cal = PQexec(DBConnect, query_cal)) == NULL) {
                RAISE_PLUGIN_ERROR("Database query failed.\n");
            }

            if (PQresultStatus(DBQuery_cal) != PGRES_TUPLES_OK && PQresultStatus(DBQuery_cal) != PGRES_COMMAND_OK) {
                PQclear(DBQuery_cal);
                RAISE_PLUGIN_ERROR("Database query for version failed.\n");
            }

            // Retrieve number of rows found in query
            nRows = PQntuples(DBQuery_cal);

            /////////////////////////////
            // No rows found?
            if (nRows == 0) {
                UDA_LOG(UDA_LOG_DEBUG, "no rows for query\n");

                PQclear(DBQuery_cal);
                RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
            }

            int s_file_cal = PQfnumber(DBQuery_cal, "file_name");

            if (!PQgetisnull(DBQuery_cal, 0, s_file_cal)) {
                file_db = PQgetvalue(DBQuery_cal, 0, s_file_cal);

                UDA_LOG(UDA_LOG_DEBUG, "file from db %s\n", file_db);

                //Add on the location
                char* file_path = getenv("MAST_GEOM_DATA");

                if (file_path == NULL) {
                    RAISE_PLUGIN_ERROR("Could not retrieve MAST_GEOM_DATA environment variable\n");
                }

                if (file == NULL) {
                    char* filename = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 2));
                    strcpy(filename, file_path);
                    strcat(filename, "/");
                    strcat(filename, file_db);
                    file = filename;
                }

                UDA_LOG(UDA_LOG_DEBUG, "file_path %s\n", file);
            }

            PQclear(DBQuery_cal);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Close connection\n");
        free(group_for_query);
    }

    char request_string[MAXSQL];
    sprintf(request_string, "NEWCDF4::readglobalmeta(file=%s)", file);
    int plugin_rc = callPlugin(idam_plugin_interface->pluginList, request_string, idam_plugin_interface);

    if (plugin_rc != 0) {
        RAISE_PLUGIN_ERROR("Error reading geometry meta-data!\n");
    }

    return plugin_rc;
};

int do_geom_list_groups(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect) {

    int shot = idam_plugin_interface->request_block->exp_number;
    UDA_LOG(UDA_LOG_DEBUG, "Exp number %d\n", shot);

    UDA_LOG(UDA_LOG_DEBUG, "Get connection\n");
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to mastgeom database failed.\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Create query\n");
    char query[MAXSQL];

    sprintf(query, "SELECT distinct(geomgroup) FROM config_data_source");

    if (shot > 0) {
        char expstr[MAXSQL];
        sprintf(expstr, " WHERE start_shot<=%d AND end_shot>%d", shot, shot);
        strcat(query, expstr);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Query is %s\n", query);

    /////////////////////////////
    // Query database
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    /////////////////////////////
    // No rows found?
    // For the future: if they've asked for a specific toroidal angle then try to generate geom from the CAD.
    //                 see commit 91ea8223480cf28228caf30cb43d05fa1c9947db for prototype
    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "no rows. \n");

        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    // Struct to store results
    struct DATASTRUCT {
        char** geomgroup;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    DATASTRUCT* data;
    data = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data->geomgroup = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->geomgroup, nRows, sizeof(char*), "STRING *");


    ///////////////////////////
    // Extract signal names
    int s_group = PQfnumber(DBQuery, "geomgroup");

    UDA_LOG(UDA_LOG_DEBUG, "retrieve results\n");
    int i;
    int stringLength;
    for (i = 0; i < nRows; i++) {
        // Retrieve signal alias
        if (!PQgetisnull(DBQuery, i, s_group)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_group)) + 1;
            data->geomgroup[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->geomgroup[i], PQgetvalue(DBQuery, i, s_group));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->geomgroup[i], stringLength, sizeof(char), "char");
        }
    }

    // Close db connection
    PQclear(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "define structure for datablock\n");

    // Output data block
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATASTRUCT");
    strcpy(parentTree.source, "idam3");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATASTRUCT);

    //Compound fields for datastruct
    initCompoundField(&field);
    strcpy(field.name, "geomgroup");
    defineField(&field, "geomgroup", "geomgroup", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    // Put data struct into data block for return
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "signals");
    strcpy(data_block->data_label, "signals");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "everything done\n");

    return 0;

}

int do_geom_list_sig(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect) {

    // User-defined stuff
    const char* geomgroup = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, geomgroup);

    int shot = idam_plugin_interface->request_block->exp_number;
    UDA_LOG(UDA_LOG_DEBUG, "Exp number %d\n", shot);

    if (shot <= 0) {
        RAISE_PLUGIN_ERROR("Please specify a shot number.\n");
    }

    float version = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, version);

    if (version > 0 && geomgroup == NULL) {
        RAISE_PLUGIN_ERROR("If specifying a version you must also specify a geomgroup.\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Getting version\n");

    int ver = -1;
    int rev = -1;
    if (version >= 0) {
        ver = (int)floor(version);
        rev = (int)floor(version * 10);
        UDA_LOG(UDA_LOG_DEBUG, "Version %d, Revision %d", ver, rev);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Get connection\n");
    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to mastgeom database failed.\n");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Create query\n");
    char query[MAXSQL];

    if (geomgroup == NULL && version < 0) {
        sprintf(query,
                "WITH ctableid "
                "  AS ( WITH mxtable AS (SELECT max(version+0.1*revision), geomgroup FROM config_data_source GROUP BY geomgroup) "
                "       SELECT c.config_data_source_id, c.geomgroup, c.start_shot, c.end_shot, m.max "
                "       FROM config_data_source c, mxtable m "
                "       WHERE m.geomgroup=c.geomgroup AND m.max=(c.version+0.1*c.revision) ) "
                " SELECT ggm.geomsignal_alias FROM ctableid cid, geomgroup_geomsignal_map ggm "
                " WHERE cid.config_data_source_id=ggm.config_data_source_id AND cid.start_shot<=%d AND cid.end_shot>%d;",
                shot, shot);
    } else {
        char* group_for_query = (char*)malloc((2 * strlen(geomgroup) + 1) * sizeof(char));
        int err = 0;
        PQescapeStringConn(DBConnect, group_for_query, geomgroup, strlen(geomgroup), &err);

        sprintf(query,
                "SELECT ggm.geomsignal_alias "
                "FROM geomgroup_geomsignal_map ggm, config_data_source c "
                "WHERE ggm.config_data_source_id=c.config_data_source_id"
                "      AND c.geomgroup='%s'"
                "      AND c.start_shot<=%d AND c.end_shot >%d", group_for_query, shot, shot);

        char verstr[MAXSQL];
        if (version <= 0) {
            sprintf(verstr,
                    " AND  (c.version+0.1*c.revision)="
                    "        (SELECT max(version+0.1*revision) FROM config_data_source "
                    "         WHERE geomgroup='%s' GROUP BY geomgroup)", group_for_query);
        } else {
            sprintf(verstr, " AND c.version=%d AND c.revision=%d", ver, rev);
        }
        strcat(query, verstr);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Query is %s\n", query);

    /////////////////////////////
    // Query database
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    /////////////////////////////
    // No rows found?
    // For the future: if they've asked for a specific toroidal angle then try to generate geom from the CAD.
    //                 see commit 91ea8223480cf28228caf30cb43d05fa1c9947db for prototype
    if (nRows == 0) {
        UDA_LOG(UDA_LOG_DEBUG, "no rows. \n");

        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }


    // Struct to store results
    struct DATASTRUCT {
        char** signal_alias;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    DATASTRUCT* data;
    data = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data->signal_alias = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_alias, nRows, sizeof(char*), "STRING *");


    ///////////////////////////
    // Extract signal names
    int s_alias = PQfnumber(DBQuery, "geomsignal_alias");

    UDA_LOG(UDA_LOG_DEBUG, "retrieve results\n");
    int i;
    int stringLength;
    for (i = 0; i < nRows; i++) {
        // Retrieve signal alias
        if (!PQgetisnull(DBQuery, i, s_alias)) {
            stringLength = (int)strlen(PQgetvalue(DBQuery, i, s_alias)) + 1;
            data->signal_alias[i] = (char*)malloc(stringLength * sizeof(char));
            strcpy(data->signal_alias[i], PQgetvalue(DBQuery, i, s_alias));
            addMalloc(idam_plugin_interface->logmalloclist, (void*)data->signal_alias[i], stringLength, sizeof(char), "char");
        }
    }

    // Close db connection
    PQclear(DBQuery);

    UDA_LOG(UDA_LOG_DEBUG, "define structure for datablock\n");

    // Output data block
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
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

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    // Put data struct into data block for return
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "signals");
    strcpy(data_block->data_label, "signals");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    UDA_LOG(UDA_LOG_DEBUG, "everything done\n");

    return 0;
}

int do_geom_get(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect)
{
    ////////////////////////////
    // Get function to return data from configuration file or corresponding calibration file.
    //
    // Arguments:
    // file : Location of file. If not given, then appropriate file name will be retrieved from the db
    //        for the given signal.
    // signal : Signal/Group to be retrieved from the file.
    // config : If argument is present, then the configuration file will be returned.
    // cal : If argument is present, then the calibration file will be returned.
    // version_config : Version number for Config file. If not set then the latest will be returned.
    // version_cal : Version number for calibration file. If not set then the latest will be returned.
    // three_d : If argument is present, then the 3D geometry is returned, otherwise the 2D geometry is returned.
    // tor_angle : If returning the 2D geometry, and the component is toroidal angle - dependent, then the user must
    //             give a toroidal angle at which to slice the model.

    UDA_LOG(UDA_LOG_DEBUG, "get file and signal names.\n");

    ////////////////////////////
    // Get parameters passed by user.
    const char* signal = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, signal);

    const char* file = NULL;
    FIND_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, file);

    int isConfig = findValue(&idam_plugin_interface->request_block->nameValueList, "config");
    int isCal = findValue(&idam_plugin_interface->request_block->nameValueList, "cal");

    float version = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, version);

    int ver = -1;
    int rev = -1;
    if (version >= 0) {
        ver = (int)floor(version);
        rev = (int)floor(version * 10);
        UDA_LOG(UDA_LOG_DEBUG, "Version %d, Revision %d", ver, rev);
    }

    // Toroidal angle dependence
    int three_d = findValue(&idam_plugin_interface->request_block->nameValueList, "three_d");
    float tor_angle = -1;
    FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, tor_angle);

    if (tor_angle < 0 || tor_angle > 2 * M_PI) {
        UDA_LOG(UDA_LOG_DEBUG,
                 "If providing a toroidal angle, it must be in the range 0 - 2 pi. Angle given will be ignored.\n");
        tor_angle = -1;
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Tor_angle is set to %f\n", tor_angle);
    }

    int shot = idam_plugin_interface->request_block->exp_number;
    UDA_LOG(UDA_LOG_DEBUG, "Exp number %d\n", shot);

    UDA_LOG(UDA_LOG_DEBUG, "Config? %d or cal %d \n", isConfig, isCal);

    if (file == NULL && !isCal && !isConfig) {
        RAISE_PLUGIN_ERROR("Filename wasn't given and didn't request configuration or calibration data.\n");
    }

    ////////////////////////////
    // If Config or cal arguments were given,
    // connect to database to retrieve filenames
    char* signal_type = NULL;
    char* signal_for_file = NULL;

    if (isConfig || isCal) {

        PGresult* DBQuery = NULL;

        if (PQstatus(DBConnect) != CONNECTION_OK) {
            PQfinish(DBConnect);
            RAISE_PLUGIN_ERROR("Connection to mastgeom database failed.\n");
        }

        char* signal_for_query = (char*)malloc((2 * strlen(signal) + 1) * sizeof(char));
        int err = 0;
        PQescapeStringConn(DBConnect, signal_for_query, signal, strlen(signal), &err);

        ///////////////////
        // Construct query to extract filename of the file that needs to be read in
        char query[MAXSQL];

        // Configuration files
        // For calibration, we want to check what the configuration file is, that it contains
        // the signal requested & then find the corresponding calibration file later
        sprintf(query,
                "SELECT cds.file_name, ggm.geomsignal_alias, cds.version, cds.revision, "
                "       ggm.geomsignal_shortname, cds.geomgroup, cds.config_data_source_id"
                " FROM config_data_source cds, geomgroup_geomsignal_map ggm"
                " WHERE cds.config_data_source_id=ggm.config_data_source_id"
                "   AND (lower(ggm.geomsignal_alias) LIKE lower('%s/%%')"
                "        OR lower(ggm.geomsignal_shortname) LIKE lower('%s/%%'))"
                "   AND cds.start_shot<=%d AND cds.end_shot>%d",
                signal_for_query, signal_for_query, shot, shot);

        // Add check for toroidal angle
        if (three_d == 1) {
            strcat(query, " AND cds.tor_angle=NULL;");
        } else if (tor_angle >= 0) {
            char tor_angle_statement[MAXSQL];
            sprintf(tor_angle_statement,
                    " AND (CASE WHEN cds.tor_angle_dependent=TRUE "
                    "THEN round(cds.tor_angle::numeric, 4)=round(%f, 4) "
                    "ELSE TRUE END)",
                    tor_angle);
            strcat(query, tor_angle_statement);
        } else {
            strcat(query, " AND cds.tor_angle_dependent=FALSE ");
        }

        // Add version check
        if (ver >= 0) {
            char ver_str[2];
            sprintf(ver_str, "%d", ver);
            char rev_str[2];
            sprintf(rev_str, "%d", rev);

            strcat(query, " AND cds.version=");
            strcat(query, ver_str);
            strcat(query, " AND cds.revision=");
            strcat(query, rev_str);
        } else {
            strcat(query, " ORDER BY cds.version DESC, cds.revision DESC LIMIT 1;");
        }

        UDA_LOG(UDA_LOG_DEBUG, "query is %s\n", query);

        /////////////////////////////
        // Query database
        if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
            RAISE_PLUGIN_ERROR("Database query failed.\n");
        }

        if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
            PQclear(DBQuery);
            RAISE_PLUGIN_ERROR("Database query failed.\n");
        }

        // Retrieve number of rows found in query
        int nRows = PQntuples(DBQuery);

        /////////////////////////////
        // No rows found?
        // For the future: if they've asked for a specific toroidal angle then try to generate geom from the CAD.
        //                 see commit 91ea8223480cf28228caf30cb43d05fa1c9947db for prototype
        if (nRows == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "no rows. isConfig %d, three_d %d, tor_angle %f\n", isConfig, three_d, tor_angle);

            PQclear(DBQuery);
            RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
        }

        /////////////////////////////
        // We have found a matching file, extract the filename and signal names
        int s_file = PQfnumber(DBQuery, "file_name");
        int s_alias = PQfnumber(DBQuery, "geomsignal_alias");
        int s_short = PQfnumber(DBQuery, "geomsignal_shortname");
        int s_ver = PQfnumber(DBQuery, "version");
        int s_rev = PQfnumber(DBQuery, "revision");
        int s_group = PQfnumber(DBQuery, "geomgroup");
        int s_id = PQfnumber(DBQuery, "config_data_source_id");

        int ver_found = -1;
        if (!PQgetisnull(DBQuery, 0, s_ver)) {
            ver_found = (int)strtol(PQgetvalue(DBQuery, 0, s_ver), NULL, 10);
        }

        int rev_found = -1;
        if (!PQgetisnull(DBQuery, 0, s_rev)) {
            rev_found = (int)strtol(PQgetvalue(DBQuery, 0, s_rev), NULL, 10);
        }

        char* geomgroup = NULL;

        if (!PQgetisnull(DBQuery, 0, s_group)) {
            geomgroup = PQgetvalue(DBQuery, 0, s_group);
        }

        int file_id = -1;
        if (!PQgetisnull(DBQuery, 0, s_id)) {
            file_id = atoi(PQgetvalue(DBQuery, 0, s_id));
        }

        char* file_db;

        // Only retrieve filename if config data was asked for.
        // Otherwise, we will want to retrieve cal filename corresponding to the config later
        if (!PQgetisnull(DBQuery, 0, s_file) && isConfig) {
            file_db = PQgetvalue(DBQuery, 0, s_file);

            UDA_LOG(UDA_LOG_DEBUG, "file from db %s\n", file_db);

            //Add on the location
            char* file_path = getenv("MAST_GEOM_DATA");

            if (file_path == NULL) {
                RAISE_PLUGIN_ERROR("Could not retrieve MAST_GEOM_DATA environment variable\n");
            }

            if (file == NULL) {
                char* filename = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 2));
                strcpy(filename, file_path);
                strcat(filename, "/");
                strcat(filename, file_db);
                file = filename;
            }

            UDA_LOG(UDA_LOG_DEBUG, "file_path %s\n", file);
        }

        int use_alias = 0;

        // Check if the signal given is a match to the shortname.
        // If so, then we want to use the signal alias instead since it has the
        // full netcdf path
        if (!PQgetisnull(DBQuery, 0, s_short)) {
            char* sigShort = PQgetvalue(DBQuery, 0, s_short);

            UDA_LOG(UDA_LOG_DEBUG, "sig short %s, sig %s\n", sigShort, signal_for_query);

            if (!strncmp(sigShort, signal_for_query, strlen(sigShort) - 1)) {
                signal_type = (char*)malloc(sizeof(char) * 7 + 1);
                strcpy(signal_type, "element");
                use_alias = 1;
            }
        }

        // If the signal matches that asked for by the user, then this means that this
        // is the signal itself. If it does not match, then we are at the group level
        // This info is useful for the wrappers
        if (!PQgetisnull(DBQuery, 0, s_alias)) {
            char* sigAlias = PQgetvalue(DBQuery, 0, s_alias);

            UDA_LOG(UDA_LOG_DEBUG, "sig alias %s use alias? %d\n", sigAlias, use_alias);

            // Need to ignore last character of sigAlias, which will be a /
            if (!strncmp(sigAlias, signal_for_query, strlen(sigAlias) - 1)
                && !use_alias) {
                signal_type = (char*)malloc(sizeof(char) * 7 + 1);
                strcpy(signal_type, "element");
                signal_for_file = (char*)malloc(sizeof(char) * (strlen(signal) + 1));
                strcpy(signal_for_file, signal);
            } else if (use_alias) {
                // In this case, we want to use the signal alias since it is the full path.
                // But, it doesn't work if it has the / on the end, so don't include the last
                // character
                signal_for_file = (char*)malloc(sizeof(char) * (strlen(sigAlias) + 1));
                strncpy(signal_for_file, sigAlias, strlen(sigAlias));
                signal_for_file[strlen(sigAlias) - 1] = '\0';
            } else {
                signal_type = (char*)malloc(sizeof(char) * 5 + 1);
                strcpy(signal_type, "group");
                signal_for_file = (char*)malloc(sizeof(char) * (strlen(signal) + 1));
                strcpy(signal_for_file, signal);
            }
        }

        UDA_LOG(UDA_LOG_DEBUG, "Clear db query\n");

        //Close db connection
        PQclear(DBQuery);

        /////////////////////////////
        // If the version and revision numbers were not given by the user
        // check that we are retrieving the latest version of the file.
        // This prevents someone retrieving signals from old versions without directly
        // requesting the old version
        PGresult* DBQuery_ver = NULL;
        if (ver < 0) {
            char query_ver[MAXSQL];
            sprintf(query_ver,
                    "SELECT max(version + 0.1*revision) "
                            "FROM config_data_source "
                            "WHERE geomgroup='%s' "
                            " AND start_shot <= %d "
                            " AND end_shot > %d;", geomgroup, shot, shot);

            UDA_LOG(UDA_LOG_DEBUG, "Version query %s\n", query_ver);

            if ((DBQuery_ver = PQexec(DBConnect, query_ver)) == NULL) {
                RAISE_PLUGIN_ERROR("Database query for version number failed.\n");
            }

            if (PQresultStatus(DBQuery_ver) != PGRES_TUPLES_OK && PQresultStatus(DBQuery_ver) != PGRES_COMMAND_OK) {
                PQclear(DBQuery_ver);
                RAISE_PLUGIN_ERROR("Database query for version failed.\n");
            }

            // Retrieve number of rows found in query
            nRows = PQntuples(DBQuery_ver);

            /////////////////////////////
            // No rows found?
            if (nRows == 0) {
                UDA_LOG(UDA_LOG_DEBUG, "no rows for version query\n");

                PQclear(DBQuery_ver);
                RAISE_PLUGIN_ERROR("No rows were found in database matching version query\n");
            }

            int s_max_version = PQfnumber(DBQuery_ver, "max");
            if (!PQgetisnull(DBQuery_ver, 0, s_max_version)) {
                float max_version = strtof(PQgetvalue(DBQuery_ver, 0, s_max_version), NULL);
                float this_version = ver_found + 0.1f * rev_found;

                // Allow for some floating point error, versions will be in 0.1 increments
                if (this_version < (max_version - 0.05)) {
                    UDA_LOG(UDA_LOG_DEBUG,
                             "The user did not specify a particular version, and for this signal only old legacy versions are available.\n");

                    PQclear(DBQuery_ver);
                    RAISE_PLUGIN_ERROR(
                            "Only legacy signals are available for the requested signal. To retrieve legacy signals the version must be explicitly given\n");
                }
            } else {
                UDA_LOG(UDA_LOG_DEBUG, "no max field for version query\n");

                PQclear(DBQuery_ver);
                RAISE_PLUGIN_ERROR("No max field version query\n");
            }
        }

        // Close db connection
        UDA_LOG(UDA_LOG_DEBUG, "Clear version query\n");
        PQclear(DBQuery_ver);

        //////////////////////////
        // If this is calibration, get cal filename
        if (isCal) {
            float version_cal = -1;
            FIND_FLOAT_VALUE(idam_plugin_interface->request_block->nameValueList, version_cal);

            int ver_cal = -1;
            int rev_cal = -1;
            if (version >= 0) {
                ver_cal = (int)floor(version_cal);
                rev_cal = (int)floor(version_cal * 10);
                UDA_LOG(UDA_LOG_DEBUG, "Cal Version %d, Revision %d", ver, rev);
            }

            PGresult* DBQuery_cal = NULL;
            char query_cal[MAXSQL];

            sprintf(query_cal,
                    "SELECT file_name FROM cal_data_source "
                    " WHERE config_data_source_id=%d "
                            "AND start_shot <= %d "
                            "AND end_shot > %d",
                    file_id, shot, shot);

            // Add version check
            if (ver_cal >= 0) {
                char ver_str[2];
                sprintf(ver_str, "%d", ver_cal);
                char rev_str[2];
                sprintf(rev_str, "%d", rev_cal);

                strcat(query, " AND version=");
                strcat(query, ver_str);
                strcat(query, " AND revision=");
                strcat(query, rev_str);
            } else {
                strcat(query, " ORDER BY version DESC, revision DESC LIMIT 1;");
            }

            UDA_LOG(UDA_LOG_DEBUG, "Cal file query %s\n", query_cal);

            if ((DBQuery_cal = PQexec(DBConnect, query_cal)) == NULL) {
                RAISE_PLUGIN_ERROR("Database query for version number failed.\n");
            }

            if (PQresultStatus(DBQuery_cal) != PGRES_TUPLES_OK && PQresultStatus(DBQuery_cal) != PGRES_COMMAND_OK) {
                PQclear(DBQuery_cal);
                RAISE_PLUGIN_ERROR("Database query for version failed.\n");
            }

            // Retrieve number of rows found in query
            nRows = PQntuples(DBQuery_cal);

            /////////////////////////////
            // No rows found?
            if (nRows == 0) {
                UDA_LOG(UDA_LOG_DEBUG, "no rows for version query\n");

                PQclear(DBQuery_cal);
                RAISE_PLUGIN_ERROR("No rows were found in database matching version query\n");
            }

            int s_file_cal = PQfnumber(DBQuery_cal, "file_name");

            if (!PQgetisnull(DBQuery_cal, 0, s_file_cal)) {
                file_db = PQgetvalue(DBQuery_cal, 0, s_file_cal);

                UDA_LOG(UDA_LOG_DEBUG, "file from db %s\n", file_db);

                //Add on the location
                char* file_path = getenv("MAST_GEOM_DATA");

                if (file_path == NULL) {
                    RAISE_PLUGIN_ERROR("Could not retrieve MAST_GEOM_DATA environment variable\n");
                }

                if (file == NULL) {
                    char* filename = (char*)malloc(sizeof(char) * (strlen(file_db) + strlen(file_path) + 2));
                    strcpy(filename, file_path);
                    strcat(filename, "/");
                    strcat(filename, file_db);
                    file = filename;
                }

                UDA_LOG(UDA_LOG_DEBUG, "file_path %s\n", file);
            }

            PQclear(DBQuery_cal);
        }

        UDA_LOG(UDA_LOG_DEBUG, "Close connection\n");
        free(signal_for_query);

    } else {
        signal_type = (char*)malloc(2 * sizeof(char));
        strcpy(signal_type, "a");
        signal_for_file = (char*)malloc(sizeof(char) * (strlen(signal) + 1));
        strcpy(signal_for_file, signal);
    }

    UDA_LOG(UDA_LOG_DEBUG, "signal_type %s \n", signal_type);

    ///////////////////////////
    // Setup signal and file path in signal_desc
//    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
//    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
//    strcpy(signal_desc->signal_name, signal_for_file);
//    strcpy(data_source->path, file);

    /////////////////////////////
    // Read in the file
    UDA_LOG(UDA_LOG_DEBUG, "Calling readCDF to retrieve file %s with signal for file %s\n", file, signal_for_file);

    IDAM_PLUGIN_INTERFACE new_plugin_interface = *idam_plugin_interface;
    DATA_BLOCK data_block_file;
    initDataBlock(&data_block_file);
    new_plugin_interface.data_block = &data_block_file;

    char request_string[MAXSQL];
    sprintf(request_string, "NEWCDF4::read(file=%s, signal=%s)", file, signal_for_file);
    int plugin_rc = callPlugin(idam_plugin_interface->pluginList, request_string, &new_plugin_interface);


//    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
//    int errConfig = readCDF(*data_source, *signal_desc, *request_block, &data_block_file, &logmalloclist,
//                            &userdefinedtypelist);

    UDA_LOG(UDA_LOG_DEBUG, "Read in file\n");

    if (plugin_rc != 0) {
        RAISE_PLUGIN_ERROR("Error reading geometry data!\n");
    }

    if (data_block_file.data_type != UDA_TYPE_COMPOUND) {
        RAISE_PLUGIN_ERROR("Non-structured type returned from data reader!\n");
    }

    /////////////////////////////
    // Combine datablock and signal_type into one structure
    // to be returned
    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;
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
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATAPLUSTYPE");
    strcpy(parentTree.source, "netcdf");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATAPLUSTYPE);

    //Compound field for calibration file
    initCompoundField(&field);
    strcpy(field.name, "data");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, udt->name);
    strcpy(field.desc, "data");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(void*);
    field.offset = (int)newoffset((size_t)offset, field.type);
    field.offpad = (int)padding((size_t)offset, field.type);
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

    addMalloc(logmalloclist, (void*)data, 1, sizeof(DATAPLUSTYPE), "DATAPLUSTYPE");
    addMalloc(logmalloclist, (void*)data->signal_type, 1, stringLength * sizeof(char), "char");

    //Return data
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Data plus type");
    strcpy(data_block->data_label, "Data plus type");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATAPLUSTYPE", 0);

    // Free heap data associated with the two DATA_BLOCKS
    // Nothing to free?
    data_block_file.data = NULL;

    free(signal_for_file);

    return 0;
}

int do_config_filename(IDAM_PLUGIN_INTERFACE* idam_plugin_interface, PGconn* DBConnect) {

    ////////////////////////////
    // Return filenames and source ids for given geom signal aliases
    //
    // Arguments:
    // signal: Geom signal group
    ////////////////////////////
    const char *signal = NULL;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_block->nameValueList, signal);

    UDA_LOG(UDA_LOG_DEBUG, "Using signal name: %s\n", signal);

    int shot = idam_plugin_interface->request_block->exp_number;
    UDA_LOG(UDA_LOG_DEBUG, "Exp number %d\n", shot);

    PGresult* DBQuery = NULL;

    if (PQstatus(DBConnect) != CONNECTION_OK) {
        PQfinish(DBConnect);
        RAISE_PLUGIN_ERROR("Connection to mastgeom database failed.\n");
    }

    char* signal_for_query = (char*)malloc((2 * strlen(signal) + 1) * sizeof(char));
    int err = 0;
    PQescapeStringConn(DBConnect, signal_for_query, signal, strlen(signal), &err);

    ///////////////////
    // Construct query to extract filename of the file that needs to be read in
    char query[MAXSQL];

    sprintf(query, "SELECT distinct(cds.file_name), cds.config_data_source_id, cds.geomgroup "
            " FROM config_data_source cds, geomgroup_geomsignal_map ggm"
            " WHERE cds.config_data_source_id=ggm.config_data_source_id"
            " AND (lower(ggm.geomsignal_alias) LIKE lower('%s/%%')"
            "      OR lower(ggm.geomsignal_shortname) LIKE lower('%s/%%'))"
            " AND cds.start_shot <= %d"
            " AND cds.end_shot > %d;",
            signal_for_query, signal_for_query, shot, shot);

    UDA_LOG(UDA_LOG_DEBUG, "query is %s\n", query);

    /////////////////////////////
    // Query database
    if ((DBQuery = PQexec(DBConnect, query)) == NULL) {
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    if (PQresultStatus(DBQuery) != PGRES_TUPLES_OK && PQresultStatus(DBQuery) != PGRES_COMMAND_OK) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("Database query failed.\n");
    }

    // Retrieve number of rows found in query
    int nRows = PQntuples(DBQuery);

    if (nRows == 0) {
        PQclear(DBQuery);
        RAISE_PLUGIN_ERROR("No rows were found in database matching query\n");
    }

    /////////////////////////////
    // We have found a matching file, extract the filename and associated group
    int s_file = PQfnumber(DBQuery, "file_name");
    int s_group = PQfnumber(DBQuery, "geomgroup");
    int s_id = PQfnumber(DBQuery, "config_data_source_id");

    struct DATASTRUCT {
        char** filenames;
        char** geomgroups;
        int* ids;
    };
    typedef struct DATASTRUCT DATASTRUCT;

    LOGMALLOCLIST* logmalloclist = idam_plugin_interface->logmalloclist;

    DATASTRUCT* data_out;
    data_out = (DATASTRUCT*)malloc(sizeof(DATASTRUCT));
    addMalloc(logmalloclist, (void*)data_out, 1, sizeof(DATASTRUCT), "DATASTRUCT");

    data_out->filenames = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(logmalloclist, (void*)data_out->filenames, nRows, sizeof(char*), "STRING *");
    data_out->geomgroups = (char**)malloc((nRows) * sizeof(char*));
    addMalloc(logmalloclist, (void*)data_out->geomgroups, nRows, sizeof(char*), "STRING *");
    data_out->ids = (int*)malloc((nRows)*sizeof(int));
    addMalloc(logmalloclist, (void*)data_out->ids, nRows, sizeof(int), "INT");

    int i = 0;
    size_t stringLength;

    for (i = 0; i < nRows; i++) {
        if (!PQgetisnull(DBQuery, i, s_file)) {
            char* file_name = PQgetvalue(DBQuery, i, s_file);
            stringLength = strlen(file_name) + 1;
            data_out->filenames[i] = (char*)malloc(sizeof(char) * stringLength);
            strcpy(data_out->filenames[i], file_name);
            addMalloc(logmalloclist, (void*)data_out->filenames[i], (int)stringLength, sizeof(char), "char");
        }

        if (!PQgetisnull(DBQuery, i, s_group)) {
            char* group = PQgetvalue(DBQuery, i, s_group);
            stringLength = strlen(group) + 1;
            data_out->geomgroups[i] = (char*)malloc(sizeof(char) * stringLength);
            strcpy(data_out->geomgroups[i], group);
            addMalloc(logmalloclist, (void*)data_out->geomgroups[i], (int)stringLength, sizeof(char), "char");
        }

        if (!PQgetisnull(DBQuery, i, s_id)) {
            int id = atoi(PQgetvalue(DBQuery, i, s_id));
            data_out->ids[i] = id;
        }
    }

    //Close db connection
    UDA_LOG(UDA_LOG_DEBUG, "Close connection\n");
    PQclear(DBQuery);
    free(signal_for_query);

    /////////////////////////////
    // Put into datablock to be returned
    /////////////////////////////
    USERDEFINEDTYPE parentTree;
    COMPOUNDFIELD field;
    int offset = 0;

    //User defined type to describe data structure
    initUserDefinedType(&parentTree);
    parentTree.idamclass = UDA_TYPE_COMPOUND;
    strcpy(parentTree.name, "DATASTRUCT");
    strcpy(parentTree.source, "IDAM3");
    parentTree.ref_id = 0;
    parentTree.imagecount = 0;
    parentTree.image = NULL;
    parentTree.size = sizeof(DATASTRUCT);

    // For filenames
    initCompoundField(&field);
    strcpy(field.name, "filenames");
    defineField(&field, "filenames", "filenames", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    // For geomgroup
    initCompoundField(&field);
    strcpy(field.name, "geomgroups");
    defineField(&field, "geomgroups", "geomgroups", &offset, ARRAYSTRING);
    addCompoundField(&parentTree, field);

    // For ids
    initCompoundField(&field);
    strcpy(field.name, "ids");
    defineField(&field, "ids", "ids", &offset, ARRAYINT);
    addCompoundField(&parentTree, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = idam_plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, parentTree);

    //Return data
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;            // Scalar structure (don't need a DIM array)
    data_block->data_n = 1;
    data_block->data = (char*)data_out;

    strcpy(data_block->data_desc, "Data");
    strcpy(data_block->data_label, "Data");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "DATASTRUCT", 0);

    // Free heap data associated with the two DATA_BLOCKS
    // Nothing to free?

    return 0;
}
