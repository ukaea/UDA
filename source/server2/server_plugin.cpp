/*---------------------------------------------------------------
 * Identify the correct UDA Data Server Plugin
 *---------------------------------------------------------------------------------------------------------------------*/
#include "server_plugin.h"
#include "plugins.hpp"
#include "server.hpp"

#include <cerrno>
#include <cstring>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <io.h>
#  include <winsock2.h>
#  define dup _dup
#  define dup2 _dup2
#endif

#include "clientserver/error_log.h"
#include "clientserver/expand_path.h"
#include "clientserver/init_structs.h"
#include "clientserver/make_request_block.h"
#include "clientserver/print_structs.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "structures/struct.h"
#include "uda/plugins.h"
#include "config/config.h"

// constexpr int RequestReadStart = 1000;
// constexpr int RequestReadMCount = 100; // Maximum initial number of plugins that can be registered
// constexpr int RequestReadMStep = 10;   // Increase heap by 10 records once the maximum is exceeded

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::config;
using namespace uda::common;

int uda::server::server_redirect_std_streams(const Config& config, int reset)
{
    // Any OS messages will corrupt xdr streams so re-divert IO from plugin libraries to a temporary file

    // Multi platform compliance
    // static FILE* originalStdFH = nullptr;
    // static FILE* originalErrFH = nullptr;
    static int original_std_fh = 0;
    static int original_err_fh = 0;
    static FILE* mdsmsg_fh = nullptr;

    static char mksdir_template[MaxPath] = {0};
    static char temp_file[MaxPath] = {0};

    static bool single_file = false;

    if (!reset) {
        single_file = config.get("plugins.debug_single_file").as_or_default(false); // Define UDA_PLUGIN_DEBUG to retain the file

        if (mdsmsg_fh != nullptr && single_file) {
            // Multi platform compliance
            // stdout = mdsmsgFH;                                  // Redirect all IO to a temporary file
            // stderr = mdsmsgFH;
            dup2(fileno(mdsmsg_fh), fileno(stdout));
            dup2(fileno(mdsmsg_fh), fileno(stderr));
            return 0;
        }

        // Multi platform compliance
        // originalStdFH = stdout;                                 // Retain current values
        // originalErrFH = stderr;
        original_std_fh = dup(fileno(stdout));
        original_err_fh = dup(fileno(stderr));
        mdsmsg_fh = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "Redirect standard output to temporary file")

        if (mksdir_template[0] == '\0') {
            auto redirect = config.get("plugins.redirect");

            if (!redirect) {
                auto work_dir = config.get("plugins.work_dir");
                if (work_dir) {
                    snprintf(mksdir_template, MaxPath, "%s/idamPLUGINXXXXXX", work_dir.as<std::string>().c_str());
                } else {
                    strcpy(mksdir_template, "/tmp/idamPLUGINXXXXXX");
                }
            } else {
                strcpy(mksdir_template, redirect.as<std::string>().c_str());
            }
        }

        strcpy(temp_file, mksdir_template);

        // Open the message Trap

        errno = 0;

        int fd = mkstemp(temp_file);
        if (fd < 0 || errno != 0) {
            int err = (errno != 0) ? errno : 994;
            UDA_THROW_ERROR(err, "Unable to Obtain a Temporary File Name")
        }

        mdsmsg_fh = fdopen(fd, "a");

        if (mdsmsg_fh == nullptr || errno != 0) {
            UDA_THROW_ERROR(999, "Unable to Trap Plugin Error Messages.")
        }

        // Multi platform compliance
        // stdout = mdsmsgFH; // Redirect to a temporary file
        // stderr = mdsmsgFH;
        dup2(fileno(mdsmsg_fh), fileno(stdout));
        dup2(fileno(mdsmsg_fh), fileno(stderr));
    } else {
        if (mdsmsg_fh != nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles and removing temporary file")

            if (!single_file) {
                if (mdsmsg_fh != nullptr) {
                    errno = 0;
                    int rc = fclose(mdsmsg_fh);
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err))
                    }
                }
                mdsmsg_fh = nullptr;
                auto debug = config.get("plugins.debug").as_or_default(false);
                if (!debug) {
                    errno = 0;
                    int rc = remove(temp_file); // Delete the temporary file
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err))
                    }
                    temp_file[0] = '\0';
                }
            }

            // Multi platform compliance
            // stdout = originalStdFH;
            // stderr = originalErrFH;
            dup2(original_std_fh, fileno(stdout));
            dup2(original_err_fh, fileno(stderr));

        } else {

            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles")

            // Multi platform compliance
            // stdout = originalStdFH;
            // stderr = originalErrFH;
            dup2(original_std_fh, fileno(stdout));
            dup2(original_err_fh, fileno(stderr));
        }
    }

    return 0;
}

// 1. open configuration file
// 2. read plugin details
//   2.1 format
//   2.2 file or server
//   2.3 library name
//   2.4 symbol name
// 3. check format is unique
// 4. issue a request ID
// 5. open the library
// 6. get plugin function address
// 7. close the file
int uda::server::server_plugin(const Config& config, RequestData* request, MetaData* meta_data, const Plugins& plugins)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Start")

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data reader plug-in

    auto& plugin_list = plugins.plugin_list();
    if ((err = make_request_data(config, request, plugin_list)) != 0) {
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "request_block")
    print_request_data(*request);

    //----------------------------------------------------------------------------------------------
    // Does the Path to Private Files contain hierarchical components not seen by the server?
    // If so make a substitution to resolve path problems.

    if (strlen(request->server) == 0 && request->request != static_cast<int>(Request::ReadServerside)) {
        // Must be a File plugin
        if ((err = path_replacement(config, request->path)) != 0) {
            return err;
        }
    }

    //----------------------------------------------------------------------
    // Copy request details into the data_source structure mimicking a SQL query

    meta_data->set("source_alias", trim_string(request->file));
    meta_data->set("filename", trim_string(request->file));
    meta_data->set("path", trim_string(request->path));
    meta_data->set("signal_name", trim_string(request->signal));
    meta_data->set("server", trim_string(request->server));
    meta_data->set("format", trim_string(request->format));
    meta_data->set("archive", trim_string(request->archive));
    meta_data->set("device_name", trim_string(request->device_name));
    meta_data->set("exp_number", request->exp_number);
    meta_data->set("pass", request->pass);
    meta_data->set("type", ' ');

    UDA_LOG(UDA_LOG_DEBUG, "End")

    return err;
}

//------------------------------------------------------------------------------------------------
// Provenance gathering plugin with a separate database.
// Functionality exposed to both server (special plugin with standard methods)
// and client application (behaves as a normal plugin)
//
// Server needs are (private to the server):
//    record (put) the original and the actual signal and source terms with the source file DOI
//    record (put) the server log record
// Client needs are (the plugin exposes these to the client in the regular manner):
//    list all provenance records for a specific client DOI - must be given
//    change provenance records status to closed
//    delete all closed records for a specific client DOI
//
// changePlugin option disabled in this context
// private malloc log and user_defined_type_list

int uda::server::provenance_plugin(const Config& config, ClientBlock *client_block,
                                   RequestData *original_request,
                                   const Plugins& plugins, const char* logRecord, MetaData& meta_data)
{

    if (STR_EQUALS(client_block->DOI, "")) {
        // No Provenance to Capture
        return 0;
    }

    // Identify the Provenance Gathering plugin (must be a function library type plugin)

    static bool initialised = false;
    static boost::optional<const uda::client_server::PluginData&> plugin = {};
    static int exec_method = 1; // The default method used to write efficiently to the backend SQL server

    struct timeval tv_start = {};
    struct timeval tv_stop = {};

    gettimeofday(&tv_start, nullptr);

    auto external_user = bool(config.get("server.external_user"));

    if (!initialised) {
        // On initialisation
        auto provenance_plugin = config.get("plugins.provenance_plugin");
        if (provenance_plugin) {
            auto plugin_name = provenance_plugin.as<std::string>();
            // Must be set in the server startup script
            UDA_LOG(UDA_LOG_DEBUG, "Plugin name: {}", plugin_name.c_str())
            auto [id, maybe_plugin] = plugins.find_by_name(plugin_name);
            if (maybe_plugin) {
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION = {}",
                        maybe_plugin->type == UDA_PLUGIN_CLASS_FUNCTION)
                UDA_LOG(UDA_LOG_DEBUG, "!external_user = {}", !external_user)
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].handle != nullptr = {}",
                        maybe_plugin->handle != nullptr)
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].entry_func   != nullptr = {}",
                        maybe_plugin->entry_func != nullptr)
            }
            if (maybe_plugin && maybe_plugin->type == UDA_PLUGIN_CLASS_FUNCTION
                && !external_user
                && maybe_plugin->handle != nullptr && maybe_plugin->entry_func != nullptr) {
                plugin = maybe_plugin;
            }
        }
        auto provenance_exec_method = config.get("plugins.provenance_exec_method");
        if (provenance_exec_method) {
            exec_method = provenance_exec_method.as<int>();
        }
        initialised = true;
    }

    if (!plugin) {
        // Not possible to record anything - no provenance plugin!
        return 0;
    }

    RequestData request = {};
    init_request_data(&request);

    strcpy(request.api_delim, "::");
    strcpy(request.source, "");

    // need 1> record the original and the actual signal and source terms with the source file DOI
    // mimic a client request

    if (logRecord == nullptr || strlen(logRecord) == 0) {
        snprintf(request.signal, MaxMeta,
                 "%s::putSignal(uuid='%s',requestedSignal='%s',requestedSource='%s', "
                 "trueSignal='%s', trueSource='%s', trueSourceDOI='%s', execMethod=%d, status=new)",
                 plugin->name.c_str(), client_block->DOI, original_request->signal, original_request->source,
                 meta_data.find("signal_name").data(), meta_data.find("path").data(), "", exec_method);
    } else {

        // need 2> record the server log record

        snprintf(request.signal, MaxMeta, "%s::putSignal(uuid='%s',logRecord='%s', execMethod=%d, status=update)",
                 plugin->name.c_str(), client_block->DOI, logRecord, exec_method);
    }

    // Activate the plugin

    UDA_LOG(UDA_LOG_DEBUG, "Provenance Plugin signal: {}", request.signal)

    const auto& plugin_list = plugins.plugin_list();
    make_request_data(config, &request, plugin_list);

    int err, rc, reset;
    DataBlock data_block = {};
    UdaPluginInterface plugin_interface = {};

    // Initialise the Data Block

    init_data_block(&data_block);

    UDA_LOG(UDA_LOG_DEBUG, "Creating plugin interface")

    // Check the Interface Compliance

    if (plugin->interface_version > 1) {
        UDA_THROW_ERROR(999, "The Provenance Plugin's Interface Version is not Implemented.")
    }

    UserDefinedTypeList user_defined_type_list{};
    init_user_defined_type_list(&user_defined_type_list);

    LogMallocList log_malloc_list{};
    init_log_malloc_list(&log_malloc_list);

    plugin_interface.interface_version = 1;
    plugin_interface.data_block = &data_block;
    plugin_interface.client_block = client_block;
    plugin_interface.request_data = &request;
    plugin_interface.meta_data = &meta_data;
    plugin_interface.config = &config;
    plugin_interface.house_keeping = false;
    plugin_interface.change_plugin = false;
    plugin_interface.pluginList = &plugin_list;
    plugin_interface.user_defined_type_list = &user_defined_type_list;
    plugin_interface.log_malloc_list = &log_malloc_list;
    plugin_interface.error_stack = {};

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = server_redirect_std_streams(config, reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output")
    }

    // Call the plugin

    UDA_LOG(UDA_LOG_DEBUG, "entering the provenance plugin")

    err = plugin->entry_func(static_cast<UDA_PLUGIN_INTERFACE*>(&plugin_interface));

    UDA_LOG(UDA_LOG_DEBUG, "returned from the provenance plugin")

    // No data are returned in this context so free everything

    UDA_LOG(UDA_LOG_DEBUG, "housekeeping")

    UDA_LOG(UDA_LOG_DEBUG, "testing for bug!!!")
    if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN || data_block.opaque_count != 0 ||
        data_block.opaque_block != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "bug detected: mitigation!!!")
        data_block.opaque_block = nullptr;
    }

    free_data_block(&data_block);

    // Reset Redirected Output

    reset = 1;
    if ((rc = uda::server::server_redirect_std_streams(config, reset)) != 0 || err != 0) {
        if (rc != 0) {
            add_error(ErrorType::Code, __func__, rc, "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) {
            return err;
        }
        return rc;
    }

    gettimeofday(&tv_stop, nullptr);
    int msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;

    UDA_LOG(UDA_LOG_DEBUG, "end of housekeeping")
    UDA_LOG(UDA_LOG_DEBUG, "Timing (ms) = {}", msecs)

    return 0;
}

//------------------------------------------------------------------------------------------------
// Identify the Plugin to use to resolve Generic Name mappings and return its ID

boost::optional<const PluginData&> uda::server::find_metadata_plugin(const Config& config, const Plugins& plugins)
{
    static bool no_plugin_registered = false;
    static boost::optional<const PluginData&> plugin = {};

    UDA_LOG(UDA_LOG_DEBUG, "Entered: no_plugin_registered state = {}", no_plugin_registered)

    if (no_plugin_registered) {
        return {}; // No Plugin for the MetaData Catalog to resolve Generic Name mappings
    }
    if (plugin) {
        return plugin; // Plugin previously identified
    }

    // Identify the MetaData Catalog plugin (must be a function library type plugin)

    auto external_user = static_cast<bool>(config.get("server.external_user"));

    auto metadata_plugin = config.get("plugins.metadata_plugin");

    if (metadata_plugin) {
        size_t _;
        std::tie(_, plugin) = plugins.find_by_name(metadata_plugin.as<std::string>());
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin specified")
    }

    if (plugin) { // Must be set in the server startup script

        if (plugin->type != UDA_PLUGIN_CLASS_FUNCTION
            || plugin->handle == nullptr
            || plugin->entry_func == nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "Invalid generic plugin")
            plugin = {};
        }

        if (plugin->is_private == UDA_PLUGIN_PRIVATE && external_user) {
            UDA_LOG(UDA_LOG_DEBUG, "Generic plugin not available to external users")
            plugin = {};
        }

        if (plugin) {
            UDA_LOG(UDA_LOG_DEBUG, "Generic Name Mapping Plugin Name: {}", metadata_plugin.as<std::string>().c_str())
            UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_CLASS_FUNCTION?: {}", plugin->type == UDA_PLUGIN_CLASS_FUNCTION)
            UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_PRIVATE?: {}", plugin->is_private == UDA_PLUGIN_PRIVATE)
            UDA_LOG(UDA_LOG_DEBUG, "External User?: {}", external_user)
            UDA_LOG(UDA_LOG_DEBUG, "Private?: {}", plugin->is_private == UDA_PLUGIN_PRIVATE && external_user)
            UDA_LOG(UDA_LOG_DEBUG, "Plugin OK?: {}", plugin->handle != nullptr && plugin->entry_func != nullptr)
        }
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin identified")
    }

    if (!plugin) {
        no_plugin_registered = true; // No Plugin found (registered)
    }

    return plugin;
}

//------------------------------------------------------------------------------------------------
// Execute the Generic Name mapping Plugin

int uda::server::call_metadata_plugin(const Config& config, const uda::client_server::PluginData& plugin,
                                      RequestData* request_block, const Plugins& plugins,
                                      MetaData& meta_data)
{
    int err, reset, rc;
    UdaPluginInterface plugin_interface = {};

    // Check the Interface Compliance

    if (plugin.interface_version > 1) {
        UDA_THROW_ERROR(999, "The Plugin's Interface Version is not Implemented.")
    }

    DataBlock data_block = {};
    init_data_block(&data_block);

    UserDefinedTypeList user_defined_type_list = {};
    init_user_defined_type_list(&user_defined_type_list);

    LogMallocList log_malloc_list = {};
    init_log_malloc_list(&log_malloc_list);

    const auto& plugin_list = plugins.plugin_list();
    plugin_interface.interface_version = 1;
    plugin_interface.data_block = &data_block;
    plugin_interface.client_block = nullptr;
    plugin_interface.request_data = request_block;
    plugin_interface.meta_data = &meta_data;
    plugin_interface.config = &config;
    plugin_interface.house_keeping = false;
    plugin_interface.change_plugin = false;
    plugin_interface.pluginList = &plugin_list;
    plugin_interface.user_defined_type_list = &user_defined_type_list;
    plugin_interface.log_malloc_list = &log_malloc_list;
    plugin_interface.error_stack = {};

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = server_redirect_std_streams(config, reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output")
    }

    // Call the plugin (Error handling is managed within)

    err = plugin.entry_func(static_cast<UDA_PLUGIN_INTERFACE*>(&plugin_interface));

    // Reset Redirected Output

    reset = 1;
    if ((rc = server_redirect_std_streams(config, reset)) != 0 || err != 0) {
        if (rc != 0) {
            add_error(ErrorType::Code, __func__, rc, "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) {
            return err;
        }
        return rc;
    }

    return err;
}
