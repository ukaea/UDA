/*---------------------------------------------------------------
* v1 IDAM Plugin: HDF5 Data Reader
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
#include "hdf5plugin.h"

#include <stdlib.h>
#include <strings.h>

#include <server/managePluginFiles.h>
#include <clientserver/initStructs.h>
#include <clientserver/udaTypes.h>

#include "readHDF58.h"

IDAMPLUGINFILELIST pluginFileList;    // Private list of open data file handles

extern int idamHDF5(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;

    static short init = 0;

//----------------------------------------------------------------------------------------
// Standard v1 Plugin Interface

    DATA_BLOCK* data_block;
    REQUEST_BLOCK* request_block;
    DATA_SOURCE* data_source;
    SIGNAL_DESC* signal_desc;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        err = 999;
        idamLog(LOG_ERROR,
                "ERROR newHDF5: Plugin Interface Version Unknown to this plugin: Unable to execute the request!\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "newHDF5", err,
                     "Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
        return err;
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    data_block = idam_plugin_interface->data_block;
    request_block = idam_plugin_interface->request_block;
    data_source = idam_plugin_interface->data_source;
    signal_desc = idam_plugin_interface->signal_desc;

    housekeeping = idam_plugin_interface->housekeeping;

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

            char* p = (char*) malloc(sizeof(char) * 2 * 1024);

            strcpy(p, "\nnewHDF5: get - Read data from a HDF5 file\n\n");

            initDataBlock(data_block);

            data_block->rank = 1;
            data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
            int i;
            for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

            data_block->data_type = TYPE_STRING;
            strcpy(data_block->data_desc, "newHDF5: help = description of this plugin");

            data_block->data = (char*) p;

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
        } else if (!strcasecmp(request_block->function, "version")) {

            //----------------------------------------------------------------------------------------
            // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Plugin version number");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else if (!strcasecmp(request_block->function, "builddate")) {

            // Plugin Build Date

            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(__DATE__) + 1;
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, __DATE__);
            data_block->data = data;
            strcpy(data_block->data_desc, "Plugin build date");
            strcpy(data_block->data_label, "date");
            strcpy(data_block->data_units, "");
            break;
        } else if (!strcasecmp(request_block->function, "defaultmethod")) {

            // Plugin Default Method

            initDataBlock(data_block);
            data_block->data_type = TYPE_STRING;
            data_block->rank = 0;
            data_block->data_n = strlen(THISPLUGIN_DEFAULT_METHOD) + 1;
            char* data = (char*) malloc(data_block->data_n * sizeof(char));
            strcpy(data, THISPLUGIN_DEFAULT_METHOD);
            data_block->data = data;
            strcpy(data_block->data_desc, "Plugin default method");
            strcpy(data_block->data_label, "method");
            strcpy(data_block->data_units, "");
            break;
        } else if (!strcasecmp(request_block->function, "maxinterfaceversion")) {

            // Plugin Maximum Interface Version

            initDataBlock(data_block);
            data_block->data_type = TYPE_INT;
            data_block->rank = 0;
            data_block->data_n = 1;
            int* data = (int*) malloc(sizeof(int));
            data[0] = THISPLUGIN_MAX_INTERFACE_VERSION;
            data_block->data = (char*) data;
            strcpy(data_block->data_desc, "Maximum Interface Version");
            strcpy(data_block->data_label, "version");
            strcpy(data_block->data_units, "");
            break;
        } else if (!strcasecmp(request_block->function, "get")) {

            //----------------------------------------------------------------------------------------
            // Read data from a HDF5 File

            // If the client has specified a specific file, the path will be found at request_block->path
            // otherwise the path is determined by a query against the metadata catalog

            if (data_source->path[0] == '\0') strcpy(data_source->path, request_block->path);
            if (signal_desc->signal_name[0] == '\0') strcpy(signal_desc->signal_name, request_block->signal);

            err = getHDF5(data_source, signal_desc, data_block);    // Legacy data reader!

            break;
        } else if (!strcasecmp(request_block->function, "put")) {

            //----------------------------------------------------------------------------------------
            // Put data into a HDF5 File

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "newHDF5", err, "The PUT method has not yet been implemented");
            break;

        } else {

            //======================================================================================
            // Error ...

            err = 999;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "newHDF5", err, "Unknown function requested!");
            break;
        }

    } while (0);

//--------------------------------------------------------------------------------------
// Housekeeping

    return err;
}

