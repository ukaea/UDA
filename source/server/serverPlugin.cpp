/*---------------------------------------------------------------
 * Identify the correct UDA Data Server Plugin
 *---------------------------------------------------------------------------------------------------------------------*/
#include "serverPlugin.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <io.h>
#  include <winsock2.h>
#  define dup _dup
#  define dup2 _dup2
#endif

#include "clientserver/errorLog.h"
#include "clientserver/expand_path.h"
#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"
#include "clientserver/printStructs.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "structures/struct.h"
#include "uda/plugins.h"
#include "config/config.h"

#define REQUEST_READ_START 1000
#define REQUEST_PLUGIN_MCOUNT 100 // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP 10   // Increase heap by 10 records once the maximum is exceeded

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;
using namespace uda::structures;

/**
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param format
 * @param plugin_list
 * @return
 */
int uda::server::findPluginIdByFormat(const char* format, const std::vector<PluginData>& plugin_list)
{
    size_t id = 0;
    for (const auto& plugin : plugin_list) {
        if (plugin.name == format) {
            return id;
        }
    }
    return -1;
}

/**
 * Find the Plugin identity: return the reference id or -1 if not found.
 * @param device
 * @param plugin_list
 * @return
 */
int uda::server::findPluginIdByDevice(const char* device, const std::vector<PluginData>& plugin_list)
{
    size_t id = 0;
    for (const auto& plugin : plugin_list) {
        if (plugin.type == UDA_PLUGIN_CLASS_DEVICE && plugin.name == device) {
            return id;
        }
    }
    return -1;
}

void resetPlugins(const std::vector<PluginData>& plugin_list)
{
    RequestData request_block;
    UdaPluginInterface plugin_interface;
    init_request_data(&request_block);
    strcpy(request_block.function, "reset");

    plugin_interface.interfaceVersion = 1;
    plugin_interface.housekeeping = 1; // Force a full reset
    plugin_interface.request_data = &request_block;
    for (const auto& plugin : plugin_list) {
        if (plugin.handle) {
            plugin.entry_func(static_cast<UDA_PLUGIN_INTERFACE*>(&plugin_interface)); // Call the housekeeping method
        }
    }
}

void uda::server::freePluginList(std::vector<PluginData>& plugin_list)
{
    resetPlugins(plugin_list);
    plugin_list.clear();
}

int uda::server::udaServerRedirectStdStreams(int reset)
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
        if (!single_file) {
            const char* env = getenv("UDA_PLUGIN_DEBUG_SINGLEFILE"); // Use a single file for all plugin data requests
            if (env != nullptr) {
                single_file = true; // Define UDA_PLUGIN_DEBUG to retain the file
            }
        }

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

        UDA_LOG(UDA_LOG_DEBUG, "Redirect standard output to temporary file");

        if (mksdir_template[0] == '\0') {
            const char* env = getenv("UDA_PLUGIN_REDIVERT");

            if (env == nullptr) {
                if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
                    snprintf(mksdir_template, MaxPath, "%s/idamPLUGINXXXXXX", env);
                } else {
                    strcpy(mksdir_template, "/tmp/idamPLUGINXXXXXX");
                }
            } else {
                strcpy(mksdir_template, env);
            }
        }

        strcpy(temp_file, mksdir_template);

        // Open the message Trap

        errno = 0;

        int fd = mkstemp(temp_file);
        if (fd < 0 || errno != 0) {
            int err = (errno != 0) ? errno : 994;
            UDA_THROW_ERROR(err, "Unable to Obtain a Temporary File Name");
        }

        mdsmsg_fh = fdopen(fd, "a");

        if (mdsmsg_fh == nullptr || errno != 0) {
            UDA_THROW_ERROR(999, "Unable to Trap Plugin Error Messages.");
        }

        // Multi platform compliance
        // stdout = mdsmsgFH; // Redirect to a temporary file
        // stderr = mdsmsgFH;
        dup2(fileno(mdsmsg_fh), fileno(stdout));
        dup2(fileno(mdsmsg_fh), fileno(stderr));
    } else {
        if (mdsmsg_fh != nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles and removing temporary file");

            if (!single_file) {
                if (mdsmsg_fh != nullptr) {
                    errno = 0;
                    int rc = fclose(mdsmsg_fh);
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err));
                    }
                }
                mdsmsg_fh = nullptr;
                if (getenv("UDA_PLUGIN_DEBUG") == nullptr) {
                    errno = 0;
                    int rc = remove(temp_file); // Delete the temporary file
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err));
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

            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles");

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
int uda::server::udaServerPlugin(const uda::config::Config& config, RequestData* request, DataSource* data_source,
                                 SignalDesc* signal_desc, const std::vector<PluginData>& plugin_list)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Start");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data reader plug-in

    if ((err = make_request_data(config, request, plugin_list)) != 0) {
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "request_block");
    print_request_data(*request);

    //----------------------------------------------------------------------------------------------
    // Does the Path to Private Files contain hierarchical components not seen by the server?
    // If so make a substitution to resolve path problems.

    if (strlen(request->server) == 0 && request->request != (int)Request::ReadServerside) {
        // Must be a File plugin
        if ((err = path_replacement(config, request->path)) != 0) {
            return err;
        }
    }

    //----------------------------------------------------------------------
    // Copy request details into the data_source structure mimicking a SQL query

    strcpy(data_source->source_alias, trim_string(request->file));
    strcpy(data_source->filename, trim_string(request->file));
    strcpy(data_source->path, trim_string(request->path));

    copy_string(trim_string(request->signal), signal_desc->signal_name, MaxName);

    strcpy(data_source->server, trim_string(request->server));

    strcpy(data_source->format, trim_string(request->format));
    strcpy(data_source->archive, trim_string(request->archive));
    strcpy(data_source->device_name, trim_string(request->device_name));

    data_source->exp_number = request->exp_number;
    data_source->pass = request->pass;
    data_source->type = ' ';

    UDA_LOG(UDA_LOG_DEBUG, "End");

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
// private malloc log and userdefinedtypelist

int uda::server::udaProvenancePlugin(const uda::config::Config& config, ClientBlock* client_block,
                                     RequestData* original_request, DataSource* data_source,
                                     SignalDesc* signal_desc, const std::vector<PluginData>& plugin_list,
                                     const char* logRecord)
{

    if (STR_EQUALS(client_block->DOI, "")) {
        // No Provenance to Capture
        return 0;
    }

    // Identify the Provenance Gathering plugin (must be a function library type plugin)

    static int plugin_id = -2;
    static int exec_method = 1; // The default method used to write efficiently to the backend SQL server
    char* env = nullptr;

    struct timeval tv_start = {};
    struct timeval tv_stop = {};

    gettimeofday(&tv_start, nullptr);

    auto external_user = bool(config.get("server.external_user"));

    if (plugin_id == -2) { // On initialisation
        plugin_id = -1;
        if ((env = getenv("UDA_PROVENANCE_PLUGIN")) != nullptr) {
            // Must be set in the server startup script
            UDA_LOG(UDA_LOG_DEBUG, "Plugin name: {}", env);
            int id = findPluginIdByFormat(env, plugin_list); // Must be defined in the server plugin configuration file
            UDA_LOG(UDA_LOG_DEBUG, "Plugin id: {}", id);
            if (id >= 0) {
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION = {}",
                        plugin_list[id].type == UDA_PLUGIN_CLASS_FUNCTION);
                UDA_LOG(UDA_LOG_DEBUG, "!environment->external_user = {}", !external_user);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list[id].handle != nullptr = {}",
                        plugin_list[id].handle != nullptr);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list[id].entry_func   != nullptr = {}",
                        plugin_list[id].entry_func != nullptr);
            }
            if (id >= 0 && plugin_list[id].type == UDA_PLUGIN_CLASS_FUNCTION &&
                !external_user && plugin_list[id].handle && plugin_list[id].entry_func != nullptr) {
                plugin_id = id;
            }
        }
        if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != nullptr) {
            exec_method = atoi(env);
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Plugin id: {}", plugin_id);

    if (plugin_id <= 0) {
        // Not possible to record anything - no provenance plugin!
        return 0;
    }

    RequestData request;
    init_request_data(&request);

    strcpy(request.api_delim, "::");
    strcpy(request.source, "");

    // need 1> record the original and the actual signal and source terms with the source file DOI
    // mimic a client request

    if (logRecord == nullptr || strlen(logRecord) == 0) {
        snprintf(request.signal, MaxMeta,
                 "%s::putSignal(uuid='%s',requestedSignal='%s',requestedSource='%s', "
                 "trueSignal='%s', trueSource='%s', trueSourceDOI='%s', execMethod=%d, status=new)",
                 plugin_list[plugin_id].name.c_str(), client_block->DOI, original_request->signal,
                 original_request->source, signal_desc->signal_name, data_source->path, "", exec_method);
    } else {

        // need 2> record the server log record

        snprintf(request.signal, MaxMeta, "%s::putSignal(uuid='%s',logRecord='%s', execMethod=%d, status=update)",
                 plugin_list[plugin_id].name.c_str(), client_block->DOI, logRecord, exec_method);
    }

    // Activate the plugin

    UDA_LOG(UDA_LOG_DEBUG, "Provenance Plugin signal: {}", request.signal);

    make_request_data(config, &request, plugin_list);

    int err, rc, reset;
    DataBlock data_block;
    UdaPluginInterface plugin_interface;

    // Initialise the Data Block

    init_data_block(&data_block);

    UDA_LOG(UDA_LOG_DEBUG, "Creating plugin interface");

    // Check the Interface Compliance

    if (plugin_list[plugin_id].interface_version > 1) {
        UDA_THROW_ERROR(999, "The Provenance Plugin's Interface Version is not Implemented.");
    }

    UserDefinedTypeList userdefinedtypelist;
    init_user_defined_type_list(&userdefinedtypelist);

    LogMallocList logmalloclist;
    init_log_malloc_list(&logmalloclist);

    plugin_interface.interfaceVersion = 1;
    plugin_interface.pluginVersion = 0;
    plugin_interface.data_block = &data_block;
    plugin_interface.client_block = client_block;
    plugin_interface.request_data = &request;
    plugin_interface.data_source = data_source;
    plugin_interface.signal_desc = signal_desc;
    plugin_interface.config = &config;
    plugin_interface.housekeeping = 0;
    plugin_interface.changePlugin = 0;
    plugin_interface.pluginList = &plugin_list;
    plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    plugin_interface.logmalloclist = &logmalloclist;
    plugin_interface.error_stack.nerrors = 0;
    plugin_interface.error_stack.idamerror = nullptr;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = udaServerRedirectStdStreams(reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin

    UDA_LOG(UDA_LOG_DEBUG, "entering the provenance plugin");

    err = plugin_list[plugin_id].entry_func(&plugin_interface);

    UDA_LOG(UDA_LOG_DEBUG, "returned from the provenance plugin");

    // No data are returned in this context so free everything

    UDA_LOG(UDA_LOG_DEBUG, "housekeeping");

    free_name_value_list(&request.nameValueList);

    UDA_LOG(UDA_LOG_DEBUG, "testing for bug!!!");
    if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN || data_block.opaque_count != 0 ||
        data_block.opaque_block != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "bug detected: mitigation!!!");
        data_block.opaque_block = nullptr;
    }

    free_data_block(&data_block);

    // Reset Redirected Output

    reset = 1;
    if ((rc = udaServerRedirectStdStreams(reset)) != 0 || err != 0) {
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

    UDA_LOG(UDA_LOG_DEBUG, "end of housekeeping");
    UDA_LOG(UDA_LOG_DEBUG, "Timing (ms) = {}", msecs);

    return 0;
}

//------------------------------------------------------------------------------------------------
// Identify the Plugin to use to resolve Generic Name mappings and return its ID

int uda::server::udaServerMetaDataPluginId(const uda::config::Config& config, const std::vector<PluginData>& plugin_list)
{
    static unsigned short noPluginRegistered = 0;
    static int plugin_id = -1;

    UDA_LOG(UDA_LOG_DEBUG, "Entered: noPluginRegistered state = {}", noPluginRegistered);
    UDA_LOG(UDA_LOG_DEBUG, "Entered: plugin_id state = {}", plugin_id);

    if (plugin_id >= 0) {
        return plugin_id; // Plugin previously identified
    }
    if (noPluginRegistered) {
        return -1; // No Plugin for the MetaData Catalog to resolve Generic Name mappings
    }

    // Identify the MetaData Catalog plugin (must be a function library type plugin)

    char* env = nullptr;
    if ((env = getenv("UDA_METADATA_PLUGIN")) != nullptr) { // Must be set in the server startup script
        int id = findPluginIdByFormat(env, plugin_list);    // Must be defined in the server plugin configuration file
        if (id >= 0 && plugin_list[id].type == UDA_PLUGIN_CLASS_FUNCTION &&
            plugin_list[id].handle != nullptr && plugin_list[id].entry_func != nullptr) {
            plugin_id = (short)id;
        }

        auto external_user = config.get("server.external_user");
        if (id >= 0 && plugin_list[id].is_private == UDA_PLUGIN_PRIVATE && external_user) {
            // Not available to external users
            plugin_id = -1;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Generic Name Mapping Plugin Name: {}", env);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_CLASS_FUNCTION?: {}",
                plugin_list[id].type == UDA_PLUGIN_CLASS_FUNCTION);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_PRIVATE?: {}", plugin_list[id].is_private == UDA_PLUGIN_PRIVATE);
        UDA_LOG(UDA_LOG_DEBUG, "External User?: {}", bool(external_user));
        UDA_LOG(UDA_LOG_DEBUG, "Private?: {}",
                plugin_list[id].is_private == UDA_PLUGIN_PRIVATE && external_user);
        UDA_LOG(UDA_LOG_DEBUG, "Plugin OK?: {}",
                plugin_list[id].handle != nullptr && plugin_list[id].entry_func != nullptr);
        UDA_LOG(UDA_LOG_DEBUG, "id: {}", id);
        UDA_LOG(UDA_LOG_DEBUG, "plugin_id: {}", plugin_id);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin identified");
    }

    if (plugin_id < 0) {
        noPluginRegistered = 1; // No Plugin found (registered)
    }

    return plugin_id;
}

//------------------------------------------------------------------------------------------------
// Execute the Generic Name mapping Plugin

int uda::server::udaServerMetaDataPlugin(const uda::config::Config& config, const std::vector<PluginData>& plugin_list,
                                         int plugin_id, RequestData* request_block, SignalDesc* signal_desc,
                                         Signal* signal_rec, DataSource* data_source)
{
    int err, reset, rc;
    UdaPluginInterface plugin_interface;

    // Check the Interface Compliance

    if (plugin_list[plugin_id].interface_version > 1) {
        UDA_THROW_ERROR(999, "The Plugin's Interface Version is not Implemented.");
    }

    DataBlock data_block;
    init_data_block(&data_block);
    data_block.signal_rec = signal_rec;

    UserDefinedTypeList userdefinedtypelist;
    init_user_defined_type_list(&userdefinedtypelist);

    LogMallocList logmalloclist;
    init_log_malloc_list(&logmalloclist);

    plugin_interface.interfaceVersion = 1;
    plugin_interface.pluginVersion = 0;
    plugin_interface.data_block = &data_block;
    plugin_interface.client_block = nullptr;
    plugin_interface.request_data = request_block;
    plugin_interface.data_source = data_source;
    plugin_interface.signal_desc = signal_desc;
    plugin_interface.config = &config;
    plugin_interface.housekeeping = 0;
    plugin_interface.changePlugin = 0;
    plugin_interface.pluginList = &plugin_list;
    plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    plugin_interface.logmalloclist = &logmalloclist;
    plugin_interface.error_stack.nerrors = 0;
    plugin_interface.error_stack.idamerror = nullptr;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = udaServerRedirectStdStreams(reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin (Error handling is managed within)

    err = plugin_list[plugin_id].entry_func(&plugin_interface);

    // Reset Redirected Output

    reset = 1;
    if ((rc = udaServerRedirectStdStreams(reset)) != 0 || err != 0) {
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
