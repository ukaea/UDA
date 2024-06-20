#include "bytesPlugin.h"

#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>

#include "readBytesNonOptimally.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_version(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_builddate(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_defaultmethod(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_maxinterfaceversion(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

static int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

int bytesPlugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    static int init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    if (idam_plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    idam_plugin_interface->pluginVersion = THISPLUGIN_VERSION;

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

    REQUEST_DATA* request = idam_plugin_interface->request_data;

    if (idam_plugin_interface->housekeeping || STR_IEQUALS(request->function, "reset")) {
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
        return do_help(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "version")) {
        return do_version(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "builddate")) {
        return do_builddate(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "defaultmethod")) {
        return do_defaultmethod(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
        return do_maxinterfaceversion(idam_plugin_interface);
    } else if (STR_IEQUALS(request->function, "read")) {
        return do_read(idam_plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR("Unknown function requested!");
    }
}

/**
 * Help: A Description of library functionality
 * @param idam_plugin_interface
 * @return
 */
int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    const char* help = "\nbytes: data reader to access files as a block of bytes without interpretation\n\n";
    const char* desc = "bytes: help = description of this plugin";

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

//----------------------------------------------------------------------------------------
// Add functionality here ....

// Check if path starts with pre-approved file path
// Raises Plugin Error if not
std::vector<std::string> split_string(const std::string& input_string, const std::string& delim) {
    size_t pos = 0, prev_pos = 0, delim_len = delim.length();
    std::vector<std::string> string_list = {};
    // Using for loop to make sure the loop ends, should cover all cases, 
    //   including if the string made up of just the delim.
    for (size_t i=0; i<input_string.size(); i++) {
        pos = input_string.find(delim, prev_pos);
        if (pos == std::string::npos) {
            string_list.emplace_back(input_string.substr(prev_pos, input_string.size()-prev_pos));
            break;
        }
        string_list.emplace_back(input_string.substr(prev_pos, pos-prev_pos));
        prev_pos = pos + delim_len;
    }
    return string_list;
}

std::string join_string(std::vector<std::string> string_list, std::string delim) {
    std::string str;
    std::string tmp_delim = "";
    for (std::string element : string_list) {
        str += tmp_delim + element;
        tmp_delim = delim;
    }
    return str;
}
 std::string resolve_filepath(char* path) {
    std::vector<std::string> foldernames = split_string(path, "/");
    for (int i=foldernames.size()-1; i>=0; i--) {
        if (foldernames[i].compare("..") == 0) {
            foldernames.erase(foldernames.begin() + i);
            foldernames.erase(foldernames.begin() + i-1);
            i--;
        }
    }
    if (foldernames[0].compare(".") == 0) {
        foldernames[0] = std::getenv("$PWD");
    } else if  (foldernames[0].compare("~") == 0) {
        foldernames[0] = (std::string("/home/")+std::getenv("USER")).c_str();
    }
    return join_string(foldernames, "/");
}

int check_allowed_path(char* expandedPath) {
    char* env_str = std::getenv("HOME");
    std::vector<std::string> allowed_paths;
    if (env_str) { // gotta check if environment variable exists before using it
        allowed_paths = split_string(env_str, ",");
    }
    std::string resolved_path = resolve_filepath(expandedPath);
    bool good_path = false;
    for (std::string allowed_path : allowed_paths) {
        if (resolved_path.rfind(allowed_path.c_str(), 0) != std::string::npos) {
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

int do_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_SOURCE* data_source = idam_plugin_interface->data_source;
    SIGNAL_DESC* signal_desc = idam_plugin_interface->signal_desc;
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    const char* path;
    FIND_REQUIRED_STRING_VALUE(idam_plugin_interface->request_data->nameValueList, path);

    StringCopy(data_source->path, path, MAXPATH);
    UDA_LOG(UDA_LOG_DEBUG, "expandEnvironmentvariables! \n");
    expand_environment_variables(data_source->path);    
    
    check_allowed_path(data_source->path);

    return readBytes(*data_source, *signal_desc, data_block, idam_plugin_interface->environment);
}
