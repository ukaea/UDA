#include "bytesPlugin.h"

#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/filesystemUtils.h>
#include <version.h>

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
#include <string>
#include <regex>

#define BYTEFILEOPENERROR           100004
#define BYTEFILEHEAPERROR           100005

class BytesPlugin
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
    int size(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    using file_ptr = std::unique_ptr<FILE, decltype(&fclose)>;

    bool init_ = false;
    std::unordered_map<std::string, file_ptr> file_map_ = {};
};

int bytesPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    static BytesPlugin plugin = {};

    if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
        RAISE_PLUGIN_ERROR("Plugin Interface Version Unknown to this plugin: Unable to execute the request!");
    }

    plugin_interface->pluginVersion = THISPLUGIN_VERSION;

    REQUEST_DATA* request = plugin_interface->request_data;

    if (plugin_interface->housekeeping || STR_IEQUALS(request->function, "reset")) {
        plugin.reset(plugin_interface);
        return 0;
    }

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
    } else if (STR_IEQUALS(request->function, "size")) {
        return plugin.size(plugin_interface);
    } else {
        RAISE_PLUGIN_ERROR_AND_EXIT("Unknown function requested!", plugin_interface);
    }
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int BytesPlugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface)
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
int BytesPlugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, UDA_BUILD_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int BytesPlugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int BytesPlugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int BytesPlugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION, "Maximum Interface Version");
}

//----------------------------------------------------------------------------------------
// Add functionality here ....

int check_path(const Environment* environment, const std::string& path)
{
    int err = 0;

    //----------------------------------------------------------------------
    // Block Access to External Users

    if (environment->external_user) {
        err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "readBytes", err, "This Service is Disabled");
        UDA_LOG(UDA_LOG_DEBUG, "Disabled Service - Requested File: %s \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!IsLegalFilePath(path.c_str())) {
        err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "readBytes", err, "The directory path has incorrect syntax");
        UDA_LOG(UDA_LOG_DEBUG, "The directory path has incorrect syntax [%s] \n", path.c_str());
        return err;
    }

    //----------------------------------------------------------------------
    // Data Source Details

    UDA_LOG(UDA_LOG_DEBUG, "File Name  : %s \n", path.c_str());

    return err;
}

int BytesPlugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* path;
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, path);

    int max_bytes = -1;
    FIND_INT_VALUE(plugin_interface->request_data->nameValueList, max_bytes);

    int offset = -1;
    FIND_INT_VALUE(plugin_interface->request_data->nameValueList, offset);

    const char* checksum = nullptr;
    FIND_STRING_VALUE(plugin_interface->request_data->nameValueList, checksum);

    bool opaque = findValue(&plugin_interface->request_data->nameValueList, "opaque");

    char tmp_path[MAXPATH];
    StringCopy(tmp_path, path, MAXPATH);
    UDA_LOG(UDA_LOG_DEBUG, "expand_environment_variables! \n");
    expand_environment_variables(tmp_path);
    
    int rc = check_allowed_path(tmp_path);
    if (rc != 0) {
        return rc;
    }

    if (checksum == nullptr) {
        checksum = "";
    }

    rc = check_path(plugin_interface->environment, tmp_path);
    if (rc != 0) {
        return rc;
    }

    errno = 0;

    FILE* file = nullptr;
    if (file_map_.count(tmp_path) == 0) {
        file_ptr ptr = {fopen(tmp_path, "rb"), fclose};
        file = ptr.get();
        file_map_.emplace(tmp_path, std::move(ptr));
    } else {
        file = file_map_.at(tmp_path).get();
    }

    int serrno = errno;

    if (file == nullptr || ferror(file) || serrno != 0) {
        int err = BYTEFILEOPENERROR;
        if (serrno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, "readBytes", serrno, "");
        }
        addIdamError(UDA_CODE_ERROR_TYPE, "readBytes", err, "Unable to Open the File for Read Access");
        file_map_.erase(tmp_path);
        return err;
    }

    return readBytes(file, data_block, offset, max_bytes, checksum, opaque);
}

int BytesPlugin::size(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* path = "";
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, path);

    size_t file_size = filesystem::file_size(path);

    return setReturnDataLongScalar(data_block, (long)file_size, nullptr);
}
