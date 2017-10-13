#include "readMetaNew.h"
#include "readIdamMeta.h"

#include <stdlib.h>

#include <strings.h>

#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int readMetaNew(IDAM_PLUGIN_INTERFACE* idam_plugin_interface) 
{
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
    } else if (STR_IEQUALS(request_block->function, "lastshot")) {
      return get_lastshot(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "lastpass")) {
      return get_lastpass(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "shotdatetime")) {
      return get_shotdatetime(idam_plugin_interface);
    } else if (STR_IEQUALS(request_block->function, "listsignals")) {
      return get_listsignals(idam_plugin_interface);
    }/*  else if (STR_IEQUALS(request_block->function, "listsources")) { */
    /*   return get_listsources(idam_plugin_interface); */
    /* } */
    else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

// Help: A Description of library functionality
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nputreadMetaNew: Replacement for readMeta plugin for eg. listing signals\n\n";

    return setReturnDataString(idam_plugin_interface->data_block, help, "readMetaNew: help = description of this plugin");
}

