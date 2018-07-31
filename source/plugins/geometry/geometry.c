/*
*  geometry.c
*
*  Created on: 24 Feb 2016
*      Author: lkogan
*
*  Reads in geometry db and signal mapping netcdf files.
*
*  Methods that are implemented:
*  help: display help for this plugin
*
*  get : retrieve netcdf file for geometry (either config or cal)
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
*         - exp_number : Taken from request_block->exp_number
*  
*  getConfigFilenames : retrieve geometry config filenames and geometry groups associated with a geometry signal
*        Arguments:
*         - signal : The geometry signal or group to find the filenames / groups for
*         - exp_number : Taken from request_block->exp_number
*
*  getSignalFile : read netcdf signal mapping file (containing information about signal - geometry component mappings)
*        Arguments:
*         - signal : Signal or group to extract from file of the data signal
*         - version : file version number
*         - exp_number : Taken from request_block->exp_number
*
*  getSignalFilename : retrieve name of filename containing information about data signals associated with a particular geomsignal group or element.
*        Arguments:
*         - geomsignal : Group or signal for which to retrieve corresponding signal filename with signal info
*         - version : file version number. If not set then the latest will be returned.
*         - exp_number : Taken from request_block->exp_number
*/
#include "geometry.h"

#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "geomSignalMap.h"
#include "geomConfig.h"

static PGconn* geomOpenDatabase(const char* host, const char* port, const char* dbname, const char* user)
{

//-------------------------------------------------------------
// Debug Trace Queries

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection: host %s\n", host);
    UDA_LOG(UDA_LOG_DEBUG, "                port %s\n", port);
    UDA_LOG(UDA_LOG_DEBUG, "                db   %s\n", dbname);
    UDA_LOG(UDA_LOG_DEBUG, "                user %s\n", user);

//-------------------------------------------------------------
// Connect to the Database Server

    PGconn* DBConnect = NULL;

    if ((DBConnect = PQsetdbLogin(host, port, NULL, NULL, dbname, user, NULL)) == NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "SQL Server Connect Error\n");
        PQfinish(DBConnect);
        return NULL;
    }

    if (PQstatus(DBConnect) == CONNECTION_BAD) {
        UDA_LOG(UDA_LOG_DEBUG, "Bad SQL Server Connect Status\n");
        PQfinish(DBConnect);
        return NULL;
    }

    UDA_LOG(UDA_LOG_DEBUG, "SQL Connection Options: %s\n", PQoptions(DBConnect));

    return DBConnect;
}


int idamGeom(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{

    static int init = 0;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    static PGconn *DBConnect = NULL;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!

        PQfinish(DBConnect);
        DBConnect = NULL;

        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        //////////////////////////////
        // Open the connection
        UDA_LOG(UDA_LOG_DEBUG, "trying to get connection\n");

        char* db_host = getenv("GEOM_DB_HOST");
        char* db_port_str = getenv("GEOM_DB_PORT");
        char* db_name = getenv("GEOM_DB_NAME");
        char* db_user = getenv("GEOM_DB_USER");

        if (db_host == NULL || db_port_str == NULL || db_name == NULL || db_user == NULL) {
            RAISE_PLUGIN_ERROR("Geom db host, port, name and user env variables were not set.\n");
        }

        DBConnect = geomOpenDatabase(db_host, db_port_str, db_name, db_user);

        if (DBConnect == NULL) {
            RAISE_PLUGIN_ERROR("Could not open database connection\n");
        }

        init = 1;

        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }

    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------
    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "getSignalFile")) {
        return do_signal_file(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "getSignalFilename")) {
        return do_signal_filename(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "getConfigFilenames")) {
        return do_config_filename(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "getMetaData")) {
        return do_meta(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "get")) {
        return do_geom_get(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "listGeomSignals")) {
        return do_geom_list_sig(idam_plugin_interface, DBConnect);
    } else if (STR_IEQUALS(request_block->function, "listGeomGroups")){
        return do_geom_list_groups(idam_plugin_interface, DBConnect);
    }else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*)malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\ngeometry: Retrieve geometry data from netcdf files, signal mapping data & filenames\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "geometry: help = description of this plugin");

    data_block->data = p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;

}

