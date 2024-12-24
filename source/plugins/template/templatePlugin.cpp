/*---------------------------------------------------------------
* v1 UDA Plugin Template: Standardised plugin design template, just add ...
*
* Input Arguments:    IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:        0 if the plugin functionality was successful
*            otherwise a Error Code is returned
*
* Standard functionality:
*
*    help    a description of what this plugin does together with a list of functions available
*
*    reset    frees all previously allocated heap, closes file handles and resets all static parameters.
*        This has the same functionality as setting the housekeeping directive in the plugin interface
*        data structure to TRUE (1)
*
*    init    Initialise the plugin: read all required data and process. Retain staticly for
*        future reference.
*
*---------------------------------------------------------------------------------------------------------------*/
#include "templatePlugin.h"

#ifdef __GNUC__
#  include <strings.h>
#endif


#include <clientserver/stringUtils.h>
#include <clientserver/initStructs.h>

class TemplatePlugin
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
    int function(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    bool init_ = false;
};

int templatePlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static TemplatePlugin plugin = {};

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

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
    } else if (STR_IEQUALS(request->function, "functionName")) {
        return plugin.function(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int TemplatePlugin::help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\ntemplatePlugin: Add Functions Names, Syntax, and Descriptions\n\n";
    const char* desc = "templatePlugin: help = description of this plugin";

    return setReturnDataString(idam_plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param idam_plugin_interface
 * @return
 */
int TemplatePlugin::version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDatastring(idam_plugin_interface->data_block, UDA_BUILD_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param idam_plugin_interface
 * @return
 */
int TemplatePlugin::build_date(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param idam_plugin_interface
 * @return
 */
int TemplatePlugin::default_method(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataString(idam_plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param idam_plugin_interface
 * @return
 */
int TemplatePlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    return setReturnDataIntScalar(idam_plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

namespace {

template <typename T>
std::string to_string(T* array, size_t size) {
    std::string result;
    const char* delim = "";
    for (size_t i = 0; i < size; ++i) {
        result += delim + std::to_string(array[i]);
        delim = ", ";
    }
    return result;
}

} // anon namespace

//----------------------------------------------------------------------------------------
// Add functionality here ....
int TemplatePlugin::function(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    NameValueList& nvl = idam_plugin_interface->request_data->nameValueList;

    const char* required = nullptr;
    FIND_REQUIRED_STRING_VALUE(nvl, required);

    double* array = nullptr;
    size_t narray = 0;
    FIND_REQUIRED_DOUBLE_ARRAY(nvl, array);

    int optional = -1;
    bool optional_found = FIND_INT_VALUE(nvl, optional);

    std::string result = std::string("Passed args: required=") + required
            + ", array=[" + to_string(array, narray) + "]";
    if (optional_found) {
        result += ", optional=" + std::to_string(optional) + ")";
    } else {
        result += ", optional=<NOT PASSED>)";
    }

    setReturnDataString(data_block, result.c_str(), "result of TemplatePlugin::function");

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}
