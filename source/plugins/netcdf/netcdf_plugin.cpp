#include "netcdf_plugin.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include <clientserver/stringUtils.h>

#include "read_netcdf.hpp"
#include "file_handle.hpp"

namespace uda::plugins::netcdf {

class Plugin {
public:
    ~Plugin() = default;

    void init() {
        if (!init_) {
        }
        init_ = true;
    }

    void reset() {
        init_ = false;
        // closes all cached file handles
        files_.clear();
    }

    static int help(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int version(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);

    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    /**
     * Get the cached handle for a file, opening (and caching) it if it is not already open.
     * If opening fails nothing is cached and the exception propagates to the caller.
     */
    const FileHandle& get_file(const std::string& file_path) {
        auto it = files_.find(file_path);
        if (it == files_.end()) {
            it = files_.emplace(file_path, std::make_unique<FileHandle>(file_path)).first;
        }
        return *it->second;
    }

    bool init_ = false;
    std::unordered_map<std::string, std::unique_ptr<FileHandle>> files_;
};

} // namespace uda::plugins::netcdf

int netcdfPlugin(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    try {
        static uda::plugins::netcdf::Plugin plugin;

        if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
            RAISE_PLUGIN_ERROR_AND_EXIT(
                    "Plugin Interface Version Unknown to this plugin: Unable to execute the request!",
                    plugin_interface)
        }

        plugin_interface->pluginVersion = THISPLUGIN_VERSION;

        REQUEST_DATA* request = plugin_interface->request_data;

        if (plugin_interface->housekeeping || STR_IEQUALS(request->function, "reset")) {
            plugin.reset();
            return 0;
        }

        plugin.init();

        if (STR_IEQUALS(request->function, "init") || STR_IEQUALS(request->function, "initialise")) {
            return 0;
        }

        int err;
        if (STR_IEQUALS(request->function, "help")) {
            err = uda::plugins::netcdf::Plugin::help(plugin_interface);
        } else if (STR_IEQUALS(request->function, "version")) {
            err = uda::plugins::netcdf::Plugin::version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "builddate")) {
            err = uda::plugins::netcdf::Plugin::build_date(plugin_interface);
        } else if (STR_IEQUALS(request->function, "defaultmethod")) {
            err = uda::plugins::netcdf::Plugin::default_method(plugin_interface);
        } else if (STR_IEQUALS(request->function, "maxinterfaceversion")) {
            err = uda::plugins::netcdf::Plugin::max_interface_version(plugin_interface);
        } else if (STR_IEQUALS(request->function, "read") || STR_IEQUALS(request->function, "")) {
            err = plugin.read(plugin_interface);
        } else {
            RAISE_PLUGIN_ERROR_AND_EXIT("Unknown function requested!", plugin_interface)
        }
        concatUdaError(&plugin_interface->error_stack);
        return err;
    } catch (std::exception& ex) {
        RAISE_PLUGIN_ERROR_AND_EXIT(ex.what(), plugin_interface)
    }
}

/**
 * Help: A Description of library functionality
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::help(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    const char* help = "\nnewCDF4: get - Read data from a netCDF4 file\n\n";
    const char* desc = "newCDF4: help = description of this plugin";

    return setReturnDataString(plugin_interface->data_block, help, desc);
}

/**
 * Plugin version
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::version(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_VERSION, "Plugin version number");
}

/**
 * Plugin Build Date
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::build_date(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    return setReturnDataString(plugin_interface->data_block, __DATE__, "Plugin build date");
}

/**
 * Plugin Default Method
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::default_method(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    return setReturnDataString(plugin_interface->data_block, THISPLUGIN_DEFAULT_METHOD, "Plugin default method");
}

/**
 * Plugin Maximum Interface Version
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    return setReturnDataIntScalar(plugin_interface->data_block, THISPLUGIN_MAX_INTERFACE_VERSION,
                                  "Maximum Interface Version");
}

/**
 * Read data from a netCDF4 File
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::read(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    REQUEST_DATA* request = plugin_interface->request_data;

    const char* file_path = nullptr;
    FIND_REQUIRED_STRING_VALUE(request->nameValueList, file_path)

    const char* data_path = nullptr;
    FIND_REQUIRED_STRING_VALUE(request->nameValueList, data_path)

    int err_code = 0;

    // TODO: write the tree to the UDA data block (ignored for now)

    const FileHandle& file = get_file(file_path);
    [[maybe_unused]] Node tree = read_netcdf_tree(file, data_path);

    return err_code;
}
