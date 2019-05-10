/*---------------------------------------------------------------
* v1 IDAM Plugin: netCDF4 Data Reader
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		0 if the plugin functionality was successful
*			otherwise a Error Code is returned 
*
* Standard functionality:
*
*	help	a description of what this plugin does together with a list of functions available
*
*	reset	frees all previously allocated heap, closes file handles and resets all static parameters.
*		This has the same functionality as setting the housekeeping directive in the plugin interface
*		data structure to TRUE (1)
*
*	init	Initialise the plugin: read all required data and process. Retain staticly for
*		future reference.	
*
*---------------------------------------------------------------------------------------------------------------*/
#include "netCDF4.h"

#include <clientserver/stringUtils.h>
#include <plugins/managePluginFiles.h>

#include "readCDF4.h"

IDAMPLUGINFILELIST pluginFileList;    // Private list of open data file handles

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_meta(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static void mastArchiveFilePath(int pulno, int pass, char* file, char* path, const ENVIRONMENT* environment)
{
    char strint[56];
    char* env = NULL;

    // Path Root

    strcpy(path, DEFAULT_ARCHIVE_DIR);        // Default location

    if ((env = getenv("MAST_DATA")) != NULL) {    // MAST Archive Root Directory Environment Variable?
        strcpy(path, env);
        if (path[strlen(path)] != '/') strcat(path, "/");
    }

    // Alternative Paths

    if (environment->data_path_id == 0) {
        sprintf(strint, "%d", pulno);
        strcat(path, strint);            // Original naming convention
    } else {
        if (pulno <= 99999)
            sprintf(strint, "/0%d/%d", pulno / 1000, pulno);
        else
            sprintf(strint, "/%d/%d", pulno / 1000, pulno);
        strcat(path, strint);
    }

    // Add the Pass element to the Path

    if (pass == -1) {
        strcat(path, "/LATEST/");
    } else {
        strcat(path, "/Pass");
        sprintf(strint, "%d", pass);
        strcat(path, strint);
        strcat(path, "/");
    }

    strcat(path, file);        // Full filename path
}


extern int idamCDF4(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static short init = 0;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    unsigned short housekeeping = idam_plugin_interface->housekeeping;

    if (housekeeping || STR_IEQUALS(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

        // Free Heap & reset counters

        closeIdamPluginFiles(&pluginFileList);    // Close all open files

        init = 0;

        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        initIdamPluginFileList(&pluginFileList);

        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    int err = 0;

    if (STR_IEQUALS(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "version")) {
        err = do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "builddate")) {
        err = do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "defaultmethod")) {
        err = do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "maxinterfaceversion")) {
        err = do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "read")) {
        err = do_read(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "put")) {
        err = do_put(idam_plugin_interface);
    }  else if (STR_IEQUALS(request_block->function, "readglobalmeta")) {
        err = do_meta(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }

    return err;
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nnewCDF4: get - Read data from a netCDF4 file\n\n";
    const char* desc = "newCDF4: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

/**
 * Read top-level meta data from a netCDF4 File
 * @param idam_plugin_interface
 * @return
 */
int do_meta(IDAM_PLUGIN_INTERFACE* idam_plugin_interface) {
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    const char* file = NULL;
    FIND_STRING_VALUE(request_block->nameValueList, file);

    const char* signal = NULL;
    FIND_STRING_VALUE(request_block->nameValueList, signal);

    if (signal != NULL) {
        strcpy(signal_desc->signal_name, signal);
    }

    if (file == NULL && (signal == NULL || request_block->exp_number == 0)) {
        RAISE_PLUGIN_ERROR("Must give either file or signal and shot number.\n");
    }

    if (STR_EQUALS(file, "")) {
        sprintf(data_source->path, "%s%06d.nc", data_source->source_alias, request_block->exp_number);
    } else {
        strcpy(data_source->path, file);
    }

    char* tmp = strdup(data_source->path);

    // TODO: This should be handled by the MAST plugin
    if (data_source->type == 'R') {
        mastArchiveFilePath(request_block->exp_number, -1, tmp, data_source->path,
                            idam_plugin_interface->environment);    // Always Latest
    } else if (data_source->path[0] == '\0') {
        mastArchiveFilePath(request_block->exp_number, request_block->pass, tmp, data_source->path,
                            idam_plugin_interface->environment);
    }

    // Legacy data reader!
    int err = readCDFGlobalMeta(data_source->path, data_block, &idam_plugin_interface->logmalloclist, &idam_plugin_interface->userdefinedtypelist);

    return err;
}

/**
 * Read data from a netCDF4 File
 * @param idam_plugin_interface
 * @return
 */
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    const char* file = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, file);

    const char* signal = NULL;
    FIND_REQUIRED_STRING_VALUE(request_block->nameValueList, signal);

    strcpy(data_source->path, file);
    strcpy(signal_desc->signal_name, signal);

    // Legacy data reader!
    int err = readCDF(*data_source, *signal_desc, *request_block, data_block,
            &idam_plugin_interface->logmalloclist, &idam_plugin_interface->userdefinedtypelist);

    return err;
}

/**
 * Save data to a netCDF4 File
 * @param idam_plugin_interface
 * @return
 */
int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    RAISE_PLUGIN_ERROR("The PUT method has not yet been implemented");
}
