/*---------------------------------------------------------------
* v1 IDAM Plugin: netCDF4 Data Reader
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *plugin_interface
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
#include "netcdf_plugin.hpp"
#include "netcdf_reader.hpp"

#include "clientserver/stringUtils.h"
#include "clientserver/udaTypes.h"
#include "clientserver/initStructs.h"
#include "plugins/managePluginFiles.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <queue>
#include <memory>
#include <fmt/format.h>

namespace uda::plugins::netcdf {

class FileHandleCache {
public:
    inline explicit FileHandleCache(unsigned int max_length = 20) : _max_length(max_length) {}

    Reader* get_handle(const std::string& filename) {
        auto itr = _files.find(filename);
        if (itr == _files.end()) {
            return add_handle(filename);
        }
        return itr->second.get();
    }

    [[nodiscard]] inline unsigned int count() const {
        return _files.size();
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Reader>> _files;
    unsigned int _max_length;
    std::queue<std::string> _file_ordering;

    Reader* add_handle(const std::string& filename) {
        if (_files.size() >= _max_length) {
            std::string stale_handle = _file_ordering.front();
            _files.erase(_files.find(stale_handle));
            _file_ordering.pop();
        }
        _files[filename] = std::make_unique<Reader>(filename);
        _file_ordering.push(filename);

        return _files.at(filename).get();
    }
};

class Plugin {
public:
    ~Plugin() = default;

    void init() {
        if (!init_) {
            // initIdamPluginFileList(&plugin_file_list_);
            // registerIdamPluginFileClose(&plugin_file_list_, (void*)ncclose);
        }
        init_ = true;
    }

    void reset() {
        // closeIdamPluginFiles(&plugin_file_list_);
        init_ = false;
    }

    static int help(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int version(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);

    static int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);

    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);

    int put(IDAM_PLUGIN_INTERFACE* plugin_interface);

private:
    // UDA_PLUGIN_FILE_LIST plugin_file_list_;
    FileHandleCache _plugin_file_list;

    static std::string mast_archive_file_path(int pulno, int pass, const std::string& file, const ENVIRONMENT* environment);

    bool init_ = false;
};

}


std::string uda::plugins::netcdf::Plugin::mast_archive_file_path(int pulno, int pass, const std::string& file, const ENVIRONMENT* environment) {
    // Path Root

    std::string path = DEFAULT_ARCHIVE_DIR; // Default location

    char* env;
    if ((env = getenv("MAST_DATA")) != nullptr) {    // MAST Archive Root Directory Environment Variable?
        path = env;
        if (path.back() != '/') {
            path += "/";
        }
    }

    // Alternative Paths

    if (environment->data_path_id == 0) {
        path += std::to_string(pulno);
    } else {
        if (pulno <= 99999) {
            path += fmt::format("/0{}/{}", pulno / 1000, pulno);
        } else {
            path += fmt::format("/{}/{}", pulno / 1000, pulno);
        }
    }

    // Add the Pass element to the Path

    if (pass == -1) {
        path += "/LATEST/";
    } else {
        path += "/Pass" + std::to_string(pass) + "/";
    }

    path += file;

    return path;
}

int capnp_netcdf(IDAM_PLUGIN_INTERFACE* plugin_interface) {
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
        } else if (STR_IEQUALS(request->function, "put")) {
            err = plugin.put(plugin_interface);
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


void write_structured_data_to_data_block(const Buffer& buffer, DATA_BLOCK* data_block) {
    initDataBlock(data_block);
    data_block->data_n = static_cast<int>(buffer.size);
    data_block->data = buffer.data;
    data_block->dims = nullptr;
    data_block->data_type = UDA_TYPE_CAPNP;
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

    auto file_reader = _plugin_file_list.get_handle(file_path);
    UDA_LOG(UDA_LOG_DEBUG, "Plugin file count after get_handle(): %d\n", _plugin_file_list.count());
    UDA_LOG(UDA_LOG_DEBUG, "netcdf_reader, file is open: %d\n", file_reader->file_is_open());

    auto capnp_buffer = file_reader->read_data(data_path);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    write_structured_data_to_data_block(capnp_buffer, data_block);

    return 0;
}

/**
 * Save data to a netCDF4 File
 * @param plugin_interface
 * @return
 */
int uda::plugins::netcdf::Plugin::put(IDAM_PLUGIN_INTERFACE* plugin_interface) {
    RAISE_PLUGIN_ERROR("The PUT method has not yet been implemented");
}
