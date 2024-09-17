#include "bytesPlugin.h"

#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>

#include "readBytesNonOptimally.h"

#if defined __has_include
#  if !__has_include(<filesystem>)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#  else
#include <filesystem>
namespace filesystem = std::filesystem;
#  endif
#else
#include <filesystem>
namespace filesystem = std::filesystem;
#endif

#include <boost/algorithm/string.hpp>

static int do_help(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* plugin_interface);

static int do_read(IDAM_PLUGIN_INTERFACE* plugin_interface);

int bytesPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static int init = 0;

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
        if (!init) return 0; // Not previously initialised: Nothing to do!
        // Free Heap & reset counters
        init = 0;
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || STR_IEQUALS(request->function, "init")
        || STR_IEQUALS(request->function, "initialise")) {

        init = 1;
        if (STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise"))
            return 0;
    }

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    //----------------------------------------------------------------------------------------
    // Standard methods: version, builddate, defaultmethod, maxinterfaceversion

    if (STR_IEQUALS(request->function, "help")) {
        return do_help(plugin_interface);
    } else if (STR_IEQUALS(request->function, "version")) {
        return do_version(plugin_interface);
    } else if (STR_IEQUALS(request->function, "builddate")) {
        return do_builddate(plugin_interface);
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        return do_defaultmethod(plugin_interface);
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        return do_maxinterfaceversion(plugin_interface);
    } else if (STR_IEQUALS(request->function, "read")) {
        return do_read(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "\nbytes: data reader to access files as a block of bytes without interpretation\n\n";
    const char* desc = "bytes: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param plugin_interface
 * @return
 */
int do_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int do_builddate(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int do_defaultmethod(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

//----------------------------------------------------------------------------------------
// Add functionality here ....

// Check if path starts with pre-approved file path
// Raises Plugin Error if not
int check_allowed_path(const char* expandedPath) {
    std::string full_path;
    try { 
        full_path = filesystem::canonical(expandedPath).string();
    } catch (filesystem::filesystem_error& e) {
        UDA_LOG(UDA_LOG_DEBUG, "Filepath [%s] not found! Error: %s\n", full_path.c_str(), e.what());
        RAISE_PLUGIN_ERROR("Provided File Path Not Found!\n");
    }
    const char* env_str = std::getenv("UDA_BYTES_PLUGIN_ALLOWED_PATHS");
    std::vector<std::string> allowed_paths;
    if (env_str) {
        // gotta check if environment variable exists before using it
        boost::split(allowed_paths, env_str, boost::is_any_of(";"));
    } 
    bool good_path = false;
    for (const auto& allowed_path : allowed_paths) {
        if (full_path.rfind(allowed_path, 0) != std::string::npos) {
            good_path = true;
            break;
        }
    }
    if (!good_path) {
        UDA_LOG(UDA_LOG_DEBUG, "Bad Path Provided %s\n", expandedPath);
        RAISE_PLUGIN_ERROR("Bad File Path Provided\n");
    }
    return 0;
}

int do_read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_SOURCE* data_source = plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = plugin_interface->signal_desc;
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* path;
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, path);

    StringCopy(data_source->path, path, MAXPATH);
    UDA_LOG(UDA_LOG_DEBUG, "expand_environment_variables! \n");
    expand_environment_variables(data_source->path);    
    
    check_allowed_path(data_source->path);

    return readBytes(*data_source, *signal_desc, data_block, plugin_interface->environment);
}
