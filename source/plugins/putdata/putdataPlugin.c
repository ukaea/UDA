#include "putdataPlugin.h"

#include <stdlib.h>
#include <strings.h>

#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaTypes.h>

#include "putOpenClose.h"
#include "putGroup.h"
#include "putVariable.h"
#include "putAttribute.h"
#include "putDevice.h"
#include "putCoordinate.h"

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int putdataPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    idamSetLogLevel(LOG_DEBUG);

    static int init = 0;

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request_block->function, "reset")) {
        if (!init) return 0; // Not previously initialised: Nothing to do!
        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request_block->function, "init")
        || STR_IEQUALS(request_block->function, "initialise")) {

        init = 1;
        if (STR_IEQUALS(request_block->function, "init") || STR_IEQUALS(request_block->function, "initialise")) {
            return 0;
        }
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

    if (STR_IEQUALS(request_block->function, "help")) {
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "open")) {
        return do_open(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "close")) {
        return do_close(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "group")) {
        return do_group(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "variable")) {
        return do_variable(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "device")) {
        return do_device(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "dimension")) {
        return do_dimension(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "attribute")) {
        return do_attribute(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "coordinate")) {
        return do_coordinate(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    char* p = (char*) malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "\nputdataPlugin: Add Functions Names, Syntax, and Descriptions\n\n");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc, "putdataPlugin: help = description of this plugin");

    data_block->data = p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
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
