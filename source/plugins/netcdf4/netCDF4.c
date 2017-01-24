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
* Change History
*
* 13Mar2015	D.G.Muir	Original Version
*---------------------------------------------------------------------------------------------------------------*/

#include "netCDF4.h"

#include <idamErrorLog.h>
#include <initStructs.h>
#include <managePluginFiles.h>
#include <idamLog.h>
#include <structures/struct.h>
#include "idamserver.h"
#include "readCDF4.h"

IDAMPLUGINFILELIST pluginFileList;    // Private list of open data file handles

extern int idamCDF4(IDAM_PLUGIN_INTERFACE * idam_plugin_interface)
{
    int i, err = 0;
    char * p;

    static short init = 0;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK * data_block;
    REQUEST_BLOCK * request_block;
    DATA_SOURCE * data_source;
    SIGNAL_DESC * signal_desc;

#ifndef USE_PLUGIN_DIRECTLY
    IDAMERRORSTACK * idamErrorStack = getIdamServerPluginErrorStack();        // Server's error stack
    logmalloclist = getIdamServerLogMallocList();
    userdefinedtypelist = getIdamServerUserDefinedTypeList();
    parseduserdefinedtypelist = *getIdamServerParsedUserDefinedTypeList();

    initIdamErrorStack(&idamerrorstack);        // Initialise Local Error Stack (defined in idamclientserver.h)
#else
    IDAMERRORSTACK *idamErrorStack = &idamerrorstack;		// local and server are the same!
#endif

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
            idamLog(LOG_ERROR,
                    "ERROR newCDF4: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "newCDF4", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        concatIdamError(idamerrorstack, idamErrorStack);
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;
    data_source = idam_plugin_interface->data_source;
    signal_desc = idam_plugin_interface->signal_desc;

    housekeeping = idam_plugin_interface->housekeeping;

#ifndef USE_PLUGIN_DIRECTLY
// Don't copy the structure if housekeeping is requested - may dereference a NULL or freed pointer!     
    if (!housekeeping && idam_plugin_interface->environment != NULL) environment = *idam_plugin_interface->environment;
#endif

// Additional interface components (must be defined at the bottom of the standard data structure)
// Versioning must be consistent with the macro THISPLUGIN_MAX_INTERFACE_VERSION and the plugin registration with the server

    //if(idam_plugin_interface->interfaceVersion >= 2){
    // NEW COMPONENTS
    //}

//----------------------------------------------------------------------------------------
// Heap Housekeeping 

// Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
// Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
// Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
// Plugin must destroy lists at end of housekeeping

// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {

        if (!init) return 0;        // Not previously initialised: Nothing to do!

// Free Heap & reset counters

        closeIdamPluginFiles(&pluginFileList);    // Close all open files

        init = 0;

        return 0;
    }

//----------------------------------------------------------------------------------------
// Initialise 

    if (!init || !strcasecmp(request_block->function, "init")
        || !strcasecmp(request_block->function, "initialise")) {

        //cdfProperties = 0;

        initIdamPluginFileList(&pluginFileList);

        init = 1;
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

//----------------------------------------------------------------------------------------
// Plugin Functions 
//----------------------------------------------------------------------------------------

    do {

// Help: A Description of library functionality

        if (!strcasecmp(request_block->function, "help")) {

            p = (char *) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nnewCDF4: get - Read data from a netCDF4 file\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS *) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "newCDF4: help = description of this plugin");

            data_block->data = (char *) p;

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = strlen(p) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

//----------------------------------------------------------------------------------------    
// Standard methods: version, builddate, defaultmethod, maxinterfaceversion 

        if (!strcasecmp(request_block->function, "version")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int * data = (int *) malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char *) data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Build Date

        if (!strcasecmp(request_block->function, "builddate")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char * data = (char *) malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = (char *) data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Default Method

        if (!strcasecmp(request_block->function, "defaultmethod")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char * data = (char *) malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = (char *) data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else

// Plugin Maximum Interface Version

        if (!strcasecmp(request_block->function, "maxinterfaceversion")) {
            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int * data = (int *) malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char *) data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else

//---------------------------------------------------------------------------------------- 
// Read data from a netCDF4 File

        if (!strcasecmp(request_block->function, "get")) {

// If the client has specified a specific file, the path will be found at request_block->path
// otherwise the path is determined by a query against the metadata catalog

            if (data_source->path[0] == '\0') strcpy(data_source->path, request_block->path);
            if (signal_desc->signal_name[0] == '\0') strcpy(signal_desc->signal_name, request_block->signal);

            //cdfProperties = 0; //NC_IGNOREHIDDENATTS;

            err = readerCDF4(data_source, signal_desc, request_block, data_block);        // Legacy data reader!

            break;
        } else

//---------------------------------------------------------------------------------------- 
// Put data into a netCDF4 File

        if (!strcasecmp(request_block->function, "put")) {

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "newCDF4", err, "The PUT method has not yet been implemented");
            break;

        } else

//---------------------------------------------------------------------------------------- 
// Add additional functionality here ....

        if (!strcasecmp(request_block->function, "test")) {

            p = (char *) malloc(sizeof(char) * 30);

            strcpy(p, "Hello World!");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS *) malloc(data_block->rank * sizeof(DIMS));
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "newCDF4: 'Hello World' single string returned");

            data_block->data = (char *) p;

            data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
            data_block->dims[0].dim_n = strlen(p) + 1;
            data_block->dims[0].compressed = 1;
            data_block->dims[0].dim0 = 0.0;
            data_block->dims[0].diff = 1.0;
            data_block->dims[0].method = 0;

            data_block->data_n = data_block->dims[0].dim_n;

            strcpy(data_block->data_label, "");
            strcpy(data_block->data_units, "");

            break;
        } else

//======================================================================================
// Error ...

            err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "newCDF4", err, "Unknown function requested!");
        break;

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    concatIdamError(idamerrorstack, idamErrorStack);    // Combine local errors with the Server's error stack

#ifndef USE_PLUGIN_DIRECTLY
    closeIdamError(&idamerrorstack);            // Free local plugin error stack
#endif

    return err;
}

