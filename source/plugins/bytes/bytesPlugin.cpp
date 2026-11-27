// Route the legacy RAISE_PLUGIN_ERROR / FIND_REQUIRED_* macros through the plugin interface in scope
#define UDA_PLUGIN_INTERFACE_VAR plugin_interface

#include "bytesPlugin.h"

#include <clientserver/stringUtils.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/initStructs.h>
#include <exception>
#include <plugins/utils.h>
#include <stdexcept>
#include <version.h>

#include "readBytesNonOptimally.h"

#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <string>
#include <regex>
#include <sys/stat.h>

#define BYTEFILEHEAPERROR           100005

namespace {
class FileDeleter
{
public:
    void operator()(FILE* file) const
    {
        if (file) {
            fclose(file);
        }
    }
};
} // anon namespace

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
        file_map_.clear();
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
    using file_ptr = std::unique_ptr<FILE, FileDeleter>;

    bool init_ = false;
    std::unordered_map<std::string, file_ptr> file_map_ = {};
};

int bytesPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    try {
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

        int err {0};
        if (STR_IEQUALS(request->function, "help")) {
            err = plugin.help(plugin_interface);
        } else if (STR_IEQUALS(request->function, "version")) {
            err = plugin.version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "builddate")) {
            err = plugin.build_date(plugin_interface);
        } else if (STR_IEQUALS(request->function, "defaultmethod")) {
            err = plugin.default_method(plugin_interface);
        } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
            err = plugin.max_interface_version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "read")) {
            err = plugin.read(plugin_interface);
        } else if (STR_IEQUALS(request->function, "size")) {
            err = plugin.size(plugin_interface);
        } else {
            RAISE_PLUGIN_ERROR_AND_EXIT("Unknown function requested!", plugin_interface);
        }

        return err;

    } catch (const std::exception& e) {
        std::string err_msg = std::string("Exception raised in bytes plugin: ") + e.what();
        RAISE_PLUGIN_ERROR_AND_EXIT(err_msg.c_str(), plugin_interface);
    } catch (...) {
        RAISE_PLUGIN_ERROR_AND_EXIT("Unknown exception raised by bytes plugin", plugin_interface);
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

int check_path(IDAM_PLUGIN_INTERFACE* plugin_interface, const std::string& path)
{
    const ENVIRONMENT* environment = plugin_interface->environment;
    int err = 0;

    //----------------------------------------------------------------------
    // Block Access to External Users

    if (environment->external_user) {
        UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "Disabled Service - Requested File: %s \n", path.c_str());
        RAISE_PLUGIN_ERROR("This Service is Disabled");
    }

    //----------------------------------------------------------------------
    // Test the filepath

    if (!IsLegalFilePath(path.c_str())) {
        UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "The directory path has incorrect syntax [%s] \n", path.c_str());
        RAISE_PLUGIN_ERROR("The directory path has incorrect syntax");
    }

    //----------------------------------------------------------------------
    // Data Source Details

    UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "File Name  : %s \n", path.c_str());

    return err;
}

int BytesPlugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* path;
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, path);

    int max_bytes = -1;
    FIND_INT_VALUE(plugin_interface->request_data->nameValueList, max_bytes);

    unsigned long offset = 0;
    FIND_UNSIGNED_LONG_VALUE(plugin_interface->request_data->nameValueList, offset);

    const char* checksum = nullptr;
    FIND_STRING_VALUE(plugin_interface->request_data->nameValueList, checksum);

    bool opaque = findValue(&plugin_interface->request_data->nameValueList, "opaque");

    char tmp_path[MAXPATH];
    StringCopy(tmp_path, path, MAXPATH);
    UDA_DIAGNOSTICS diag = udaPluginDiagnostics(plugin_interface);
    udaExpandEnvironmentVariables(&diag, tmp_path);
    UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "udaExpandEnvironmentVariables: path=%s \n", tmp_path);

    if (checksum == nullptr) {
        checksum = "";
    }

    int rc = check_path(plugin_interface, tmp_path);
    if (rc != 0) {
        return rc;
    }

    // Cached files were checked when they were opened.
    auto entry = file_map_.find(tmp_path);
    if (entry == file_map_.end()) {
        FILE* opened = nullptr;
        rc = udaOpenDownloadableFile(plugin_interface, tmp_path, &opened);
        if (rc != 0) {
            return rc;
        }
        entry = file_map_.emplace(tmp_path, file_ptr(opened)).first;
    }
    FILE* file = entry->second.get();

    // Size the open file, not the name, which may now refer to something else.
    struct stat status = {};
    if (fstat(fileno(file), &status) != 0) {
        UDA_PLUGIN_ADD_SYS_ERROR(plugin_interface, errno, "");
        file_map_.erase(entry);
        RAISE_PLUGIN_ERROR("Unable to determine file size");
    }
    if (offset >= static_cast<unsigned long>(status.st_size)) {
        RAISE_PLUGIN_ERROR("Offset specified is out of bounds");
    }

    return readBytes(plugin_interface, file, data_block, offset, max_bytes, checksum, opaque);
}

int BytesPlugin::size(IDAM_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    const char* path = "";
    FIND_REQUIRED_STRING_VALUE(plugin_interface->request_data->nameValueList, path);

    char tmp_path[MAXPATH];
    StringCopy(tmp_path, path, MAXPATH);
    UDA_DIAGNOSTICS diag = udaPluginDiagnostics(plugin_interface);
    udaExpandEnvironmentVariables(&diag, tmp_path);
    UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "udaExpandEnvironmentVariables: path=%s \n", tmp_path);

    int rc = check_path(plugin_interface, tmp_path);
    if (rc != 0) {
        return rc;
    }

    FILE* opened = nullptr;
    rc = udaOpenDownloadableFile(plugin_interface, tmp_path, &opened);
    if (rc != 0) {
        return rc;
    }
    const file_ptr file(opened);

    struct stat status = {};
    if (fstat(fileno(file.get()), &status) != 0) {
        UDA_PLUGIN_ADD_SYS_ERROR(plugin_interface, errno, "");
        RAISE_PLUGIN_ERROR("Unable to determine file size");
    }
    const auto file_size = static_cast<size_t>(status.st_size);

    return setReturnDataLongScalar(data_block, (long)file_size, nullptr);
}
