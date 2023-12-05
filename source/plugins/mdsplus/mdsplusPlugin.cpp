#include "mdsplusPlugin.h"

#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>

class MDSplusPlugin
{
public:
    void init(IDAM_PLUGIN_INTERFACE* plugin_interface)
    {
        REQUEST_DATA* request = plugin_interface->request_data;
        if (!init_
                || STR_IEQUALS(request->function, "init")
                || STR_IEQUALS(request->function, "initialise")) {
            reset(plugin_interface);
            // Initialise plugin
            init_ = true;
        }
    }
    void reset(IDAM_PLUGIN_INTERFACE* plugin_interface)
    {
        if (!init_) {
            // Not previously initialised: Nothing to do!
            return;
        }
        // Free Heap & reset counters
        init_ = false;
    }

    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    bool init_ = false;
};

/**
 * Entry function
 */
int mdsplusPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static MDSplusPlugin plugin = {};

    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_DATA* request = plugin_interface->request_data;

    if (plugin_interface->housekeeping || STR_IEQUALS(request->function, "reset")) {
        plugin.reset(plugin_interface);
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise
    plugin.init(plugin_interface);
    if (STR_IEQUALS(request->function, "init")
        || STR_IEQUALS(request->function, "initialise")) {
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

    if (STR_IEQUALS(request->function, "help")) {
        return plugin.help(plugin_interface);
    } else if (STR_IEQUALS(request->function, "version")) {
        return plugin.version(plugin_interface);
    } else if (STR_IEQUALS(request->function, "builddate")) {
        return plugin.build_date(plugin_interface);
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        return plugin.default_method(plugin_interface);
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        return plugin.max_interface_version(plugin_interface);
    } else if (STR_IEQUALS(request->function, "read")) {
        return plugin.read(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int MDSplusPlugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param plugin_interface
 * @return
 */
int MDSplusPlugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int MDSplusPlugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int MDSplusPlugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int MDSplusPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

//----------------------------------------------------------------------------------------
// Add functionality here ....
int MDSplusPlugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    NameValueList& nvl = plugin_interface->request_data->nameValueList;

    // Read arguments

    const char* signal = nullptr;
    FIND_REQUIRED_STRING_VALUE(nvl, signal);

    // Read data from MDS+
    // ...

    // Return data via data_block

    setReturnDataString(data_block, "result", "result of MDSplusPlugin::read");

    return 0;
}
