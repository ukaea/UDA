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

#include <strings.h>

#include <server/udaServer.h>
#include <structures/struct.h>
#include <server/managePluginFiles.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>

#include "readCDF4.h"

IDAMPLUGINFILELIST pluginFileList;    // Private list of open data file handles

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

extern int idamCDF4(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {

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
    const char* help = "\nnetCDF4: get - Read data from a netCDF4 file\n\n";
    const char* desc = "netCDF4: help = description of this plugin";

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
 * Read data from a netCDF4 File
 * @param idam_plugin_interface
 * @return
 */
int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    const char* file = NULL;
    bool is_file = FIND_STRING_VALUE(request_block->nameValueList, file);

    const char* signal = NULL;
    bool is_signal = FIND_STRING_VALUE(request_block->nameValueList, signal);

    // If the client has specified a specific file, the path will be found at request_block->path
    // otherwise the path is determined by a query against the metadata catalog

    if (is_file) {
        strcpy(data_source->path, file);
    } else if (data_source->path[0] == '\0') {
        strcpy(data_source->path, request_block->path);
    }

    if (is_signal) {
        strcpy(signal_desc->signal_name, signal);
    } else if (signal_desc->signal_name[0] == '\0') {
        strcpy(signal_desc->signal_name, request_block->signal);
    }

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    return readerCDF4(data_source, signal_desc, request_block, data_block); // Legacy data reader!
}

/**
 * Put data into a netCDF4 File
 * @param idam_plugin_interface
 * @return
 */
int do_put(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    RAISE_PLUGIN_ERROR("The PUT method has not yet been implemented");
}