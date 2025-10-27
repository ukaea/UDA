/*---------------------------------------------------------------
* Identify the correct UDA Data Server Plugin
*---------------------------------------------------------------------------------------------------------------------*/
#include "server_plugin.h"
#include "plugins.hpp"
#include "server.hpp"

#include <cstdlib>
#include <cerrno>
#include <dlfcn.h>
#include <cstring>
#if defined(__GNUC__)
#  include <unistd.h>
#else
#  include <winsock2.h>
#  include <io.h>
#  define dup _dup
#  define dup2 _dup2
#endif

#include <cache/memcache.hpp>
#include <client/udaClient.h>
#include <clientserver/expand_path.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/printStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaErrors.h>
#include <clientserver/protocol.h>
#include <clientserver/mkstemp.h>
#include <structures/struct.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

int uda::serverRedirectStdStreams(int reset)
{
    // Any OS messages will corrupt xdr streams so re-divert IO from plugin libraries to a temporary file

    // Multi platform compliance
    //static FILE* originalStdFH = nullptr;
    //static FILE* originalErrFH = nullptr;
    static int originalStdFH = 0;
    static int originalErrFH = 0;
    static FILE* mdsmsgFH = nullptr;

    static char mksdir_template[MAXPATH] = { 0 };
    static char tempFile[MAXPATH] = { 0 };

    static bool singleFile = false;

    if (!reset) {
        if (!singleFile) {
            const char* env = getenv("UDA_PLUGIN_DEBUG_SINGLEFILE");        // Use a single file for all plugin data requests
            if (env != nullptr) {
                singleFile = true;                    // Define UDA_PLUGIN_DEBUG to retain the file
            }
        }

        if (mdsmsgFH != nullptr && singleFile) {
            // Multi platform compliance
            //stdout = mdsmsgFH;                                  // Redirect all IO to a temporary file
            //stderr = mdsmsgFH;
            dup2(fileno(mdsmsgFH), fileno(stdout));
            dup2(fileno(mdsmsgFH), fileno(stderr));
            return 0;
        }

        // Multi platform compliance
        //originalStdFH = stdout;                                 // Retain current values
        //originalErrFH = stderr;
        originalStdFH = dup(fileno(stdout));
        originalErrFH = dup(fileno(stderr));
        mdsmsgFH = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "Redirect standard output to temporary file\n");

        if (mksdir_template[0] == '\0') {
            const char* env = getenv("UDA_PLUGIN_REDIVERT");

            if (env == nullptr) {
                if ((env = getenv("UDA_WORK_DIR")) != nullptr) {
                    snprintf(mksdir_template, MAXPATH, "%s/idamPLUGINXXXXXX", env);
                } else {
                    strcpy(mksdir_template, "/tmp/idamPLUGINXXXXXX");
                }
            } else {
                strcpy(mksdir_template, env);
            }
        }

        strcpy(tempFile, mksdir_template);

        // Open the message Trap

        errno = 0;

        int fd = mkstemp(tempFile);
        if (fd < 0 || errno != 0) {
            int err = (errno != 0) ? errno : 994;
            UDA_THROW_ERROR(err, "Unable to Obtain a Temporary File Name");
        }

        mdsmsgFH = fdopen(fd, "a");

        if (mdsmsgFH == nullptr || errno != 0) {
            UDA_THROW_ERROR(999, "Unable to Trap Plugin Error Messages.");
        }

        // Multi platform compliance
        //stdout = mdsmsgFH; // Redirect to a temporary file
        //stderr = mdsmsgFH;
        dup2(fileno(mdsmsgFH), fileno(stdout));
        dup2(fileno(mdsmsgFH), fileno(stderr));
    } else {
        if (mdsmsgFH != nullptr) {
            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles and removing temporary file\n");

            if (!singleFile) {
                if (mdsmsgFH != nullptr) {
                    errno = 0;
                    int rc = fclose(mdsmsgFH);
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err));
                    }
                }
                mdsmsgFH = nullptr;
                if (getenv("UDA_PLUGIN_DEBUG") == nullptr) {
                    errno = 0;
                    int rc = remove(tempFile);    // Delete the temporary file
                    if (rc) {
                        int err = errno;
                        UDA_THROW_ERROR(err, strerror(err));
                    }
                    tempFile[0] = '\0';
                }
            }

            // Multi platform compliance
            //stdout = originalStdFH;
            //stderr = originalErrFH;
            dup2(originalStdFH, fileno(stdout));
            dup2(originalErrFH, fileno(stderr));

        } else {

            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles\n");

            // Multi platform compliance
            //stdout = originalStdFH;
            //stderr = originalErrFH;
            dup2(originalStdFH, fileno(stdout));
            dup2(originalErrFH, fileno(stderr));
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
int uda::serverPlugin(REQUEST_DATA* request, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc,
                      const Plugins& plugins, const ENVIRONMENT* environment)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Start\n");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data reader plug-in

    if ((err = makeRequestData(request, plugins.as_plugin_list(), environment)) != 0) {
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "request_block\n");
    printRequestData(*request);

    //----------------------------------------------------------------------------------------------
    // Does the Path to Private Files contain hierarchical components not seen by the server?
    // If so make a substitution to resolve path problems.

    if (strlen(request->server) == 0 && request->request != REQUEST_READ_SERVERSIDE) {
        // Must be a File plugin
        if ((err = pathReplacement(request->path, environment)) != 0) {
            return err;
        }
    }

    //----------------------------------------------------------------------
    // Copy request details into the data_source structure mimicking a SQL query

    strcpy(data_source->source_alias, TrimString(request->file));
    strcpy(data_source->filename, TrimString(request->file));
    strcpy(data_source->path, TrimString(request->path));

    copyString(TrimString(request->signal), signal_desc->signal_name, MAXNAME);

    strcpy(data_source->server, TrimString(request->server));

    strcpy(data_source->format, TrimString(request->format));
    strcpy(data_source->archive, TrimString(request->archive));
    strcpy(data_source->device_name, TrimString(request->device_name));

    data_source->exp_number = request->exp_number;
    data_source->pass = request->pass;
    data_source->type = ' ';

    UDA_LOG(UDA_LOG_DEBUG, "End\n");

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

int uda::provenancePlugin(ClientBlock* client_block, RequestData* original_request, const Plugins& plugins,
                          const char* logRecord, const server::Environment& environment, uda::MetadataBlock& metadata)
{

    if (STR_EQUALS(client_block->DOI, "")) {
        // No Provenance to Capture
        return 0;
    }

    // Identify the Provenance Gathering plugin (must be a function library type plugin)

    static bool initialised = false;
    static boost::optional<PluginData> plugin = {};
    static int exec_method = 1;        // The default method used to write efficiently to the backend SQL server
    char* env = nullptr;

    struct timeval tv_start = {};
    struct timeval tv_stop = {};

    gettimeofday(&tv_start, nullptr);

    if (!initialised) {
        // On initialisation
        if ((env = getenv("UDA_PROVENANCE_PLUGIN")) != nullptr) {
            // Must be set in the server startup script
            UDA_LOG(UDA_LOG_DEBUG, "Plugin name: %s\n", env);
            auto maybe_plugin = plugins.find_by_format(env);
            if (maybe_plugin) {
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION = %d\n",
                        maybe_plugin->plugin_class == UDA_PLUGIN_CLASS_FUNCTION);
                UDA_LOG(UDA_LOG_DEBUG, "!environment->external_user = %d\n", !environment->external_user);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL = %d\n",
                        maybe_plugin->status == UDA_PLUGIN_OPERATIONAL);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].pluginHandle != nullptr = %d\n",
                        maybe_plugin->pluginHandle != nullptr);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].idamPlugin   != nullptr = %d\n",
                        maybe_plugin->idamPlugin != nullptr);
            }
            if (maybe_plugin &&
                    maybe_plugin->plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
                    !environment->external_user &&
                    maybe_plugin->status == UDA_PLUGIN_OPERATIONAL &&
                    maybe_plugin->pluginHandle != nullptr &&
                    maybe_plugin->idamPlugin != nullptr) {
                plugin = maybe_plugin.get();
            }
        }
        if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != nullptr) {
            exec_method = atoi(env);
        }
        initialised = true;
    }

    if (!plugin) {
        // Not possible to record anything - no provenance plugin!
        return 0;
    }

    RequestData request = {};
    initRequestData(&request);

    strcpy(request.api_delim, "::");
    strcpy(request.source, "");

    // need 1> record the original and the actual signal and source terms with the source file DOI
    // mimic a client request

    if (logRecord == nullptr || strlen(logRecord) == 0) {
        snprintf(request.signal, MAXMETA, "%s::putSignal(uuid='%s',requestedSignal='%s',requestedSource='%s', "
                                      "trueSignal='%s', trueSource='%s', trueSourceDOI='%s', execMethod=%d, status=new)",
                plugin->format, client_block->DOI, original_request->signal, original_request->source,
                metadata.signal_desc.signal_name, metadata.data_source.path, "", exec_method);
    } else {

        // need 2> record the server log record

        snprintf(request.signal, MAXMETA, "%s::putSignal(uuid='%s',logRecord='%s', execMethod=%d, status=update)",
                plugin->format, client_block->DOI, logRecord, exec_method);
    }

    // Activate the plugin

    UDA_LOG(UDA_LOG_DEBUG, "Provenance Plugin signal: %s\n", request.signal);

    makeRequestData(&request, plugins.as_plugin_list(), environment.p_env());

    int err, rc, reset;
    DataBlock data_block = {};
    IdamPluginInterface plugin_interface = {};

    // Initialise the Data Block

    initDataBlock(&data_block);

    UDA_LOG(UDA_LOG_DEBUG, "Creating plugin interface\n");

    // Check the Interface Compliance

    if (plugin->interfaceVersion > 1) {
        UDA_THROW_ERROR(999, "The Provenance Plugin's Interface Version is not Implemented.");
    }

    USERDEFINEDTYPELIST userdefinedtypelist;
    initUserDefinedTypeList(&userdefinedtypelist);

    LOGMALLOCLIST logmalloclist;
    initLogMallocList(&logmalloclist);

    auto plugin_list = plugins.as_plugin_list();
    plugin_interface.interfaceVersion = 1;
    plugin_interface.pluginVersion = 0;
    plugin_interface.data_block = &data_block;
    plugin_interface.client_block = client_block;
    plugin_interface.request_data = &request;
    plugin_interface.data_source = &metadata.data_source;
    plugin_interface.signal_desc = &metadata.signal_desc;
    plugin_interface.environment = environment.p_env();
    plugin_interface.housekeeping = 0;
    plugin_interface.changePlugin = 0;
    plugin_interface.pluginList = &plugin_list;
    plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    plugin_interface.logmalloclist = &logmalloclist;
    plugin_interface.error_stack.nerrors = 0;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = uda::serverRedirectStdStreams(reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin

    UDA_LOG(UDA_LOG_DEBUG, "entering the provenance plugin\n");

    err = plugin->idamPlugin(&plugin_interface);

    UDA_LOG(UDA_LOG_DEBUG, "returned from the provenance plugin\n");

    // No data are returned in this context so free everything

    UDA_LOG(UDA_LOG_DEBUG, "housekeeping\n");

    freeNameValueList(&request.nameValueList);

    UDA_LOG(UDA_LOG_DEBUG, "testing for bug!!!\n");
    if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN ||
        data_block.opaque_count != 0 ||
        data_block.opaque_block != nullptr) {
        UDA_LOG(UDA_LOG_DEBUG, "bug detected: mitigation!!!\n");
        data_block.opaque_block = nullptr;
    }

    freeDataBlock(&data_block);

    // Reset Redirected Output

    reset = 1;
    if ((rc = uda::serverRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, rc, "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) {
            return err;
        }
        return rc;
    }

    gettimeofday(&tv_stop, nullptr);
    int msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;

    UDA_LOG(UDA_LOG_DEBUG, "end of housekeeping\n");
    UDA_LOG(UDA_LOG_DEBUG, "Timing (ms) = %d\n", msecs);

    return 0;
}

//------------------------------------------------------------------------------------------------
// Identify the Plugin to use to resolve Generic Name mappings and return its ID

boost::optional<PluginData> uda::find_metadata_plugin(const Plugins& plugins, const server::Environment& environment)
{
    static bool no_plugin_registered = false;
    static boost::optional<PluginData> plugin = {};

    UDA_LOG(UDA_LOG_DEBUG, "Entered: no_plugin_registered state = %d\n", no_plugin_registered);

    if (plugin) {
        return plugin.get();     // Plugin previously identified
    }
    if (no_plugin_registered) {
        return {};        // No Plugin for the MetaData Catalog to resolve Generic Name mappings
    }

    // Identify the MetaData Catalog plugin (must be a function library type plugin)

    char* env = nullptr;
    if ((env = getenv("UDA_METADATA_PLUGIN")) != nullptr) {        // Must be set in the server startup script
        auto maybe_plugin = plugins.find_by_format(env);
        if (maybe_plugin &&
            maybe_plugin->plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
            maybe_plugin->status == UDA_PLUGIN_OPERATIONAL &&
            maybe_plugin->pluginHandle != nullptr &&
            maybe_plugin->idamPlugin != nullptr) {
            plugin = maybe_plugin.get();
        }

        if (plugin && plugin->is_private == UDA_PLUGIN_PRIVATE && environment->external_user) {
            // Not available to external users
            plugin = {};
        }

        UDA_LOG(UDA_LOG_DEBUG, "Generic Name Mapping Plugin Name: %s\n", env);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_CLASS_FUNCTION?: %d\n", maybe_plugin->plugin_class == UDA_PLUGIN_CLASS_FUNCTION);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_PRIVATE?: %d\n", maybe_plugin->is_private == UDA_PLUGIN_PRIVATE);
        UDA_LOG(UDA_LOG_DEBUG, "External User?: %d\n", environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "Private?: %d\n",
                maybe_plugin->is_private == UDA_PLUGIN_PRIVATE && environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_OPERATIONAL?: %d\n", maybe_plugin->status == UDA_PLUGIN_OPERATIONAL);
        UDA_LOG(UDA_LOG_DEBUG, "Plugin OK?: %d\n",
                maybe_plugin->pluginHandle != nullptr && maybe_plugin->idamPlugin != nullptr);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin identified\n");
    }

    if (!plugin) {
        no_plugin_registered = true;        // No Plugin found (registered)
    }

    return plugin;
}

//------------------------------------------------------------------------------------------------
// Execute the Generic Name mapping Plugin

int uda::call_metadata_plugin(const PluginData& plugin, RequestData* request_block, const server::Environment& environment,
                              const uda::Plugins& plugins, uda::MetadataBlock& metadata)
{
    int err, reset, rc;
    IdamPluginInterface idam_plugin_interface = {};

    // Check the Interface Compliance

    if (plugin.interfaceVersion > 1) {
        UDA_THROW_ERROR(999, "The Plugin's Interface Version is not Implemented.");
    }

    DataBlock data_block = {};
    initDataBlock(&data_block);
    data_block.signal_rec = &metadata.signal_rec;

    UserDefinedTypeList userdefinedtypelist = {};
    initUserDefinedTypeList(&userdefinedtypelist);

    LogMallocList logmalloclist = {};
    initLogMallocList(&logmalloclist);

    auto plugin_list = plugins.as_plugin_list();
    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.pluginVersion = 0;
    idam_plugin_interface.data_block = &data_block;
    idam_plugin_interface.client_block = nullptr;
    idam_plugin_interface.request_data = request_block;
    idam_plugin_interface.data_source = &metadata.data_source;
    idam_plugin_interface.signal_desc = &metadata.signal_desc;
    idam_plugin_interface.environment = environment.p_env();
    idam_plugin_interface.housekeeping = 0;
    idam_plugin_interface.changePlugin = 0;
    idam_plugin_interface.pluginList = &plugin_list;
    idam_plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    idam_plugin_interface.logmalloclist = &logmalloclist;
    idam_plugin_interface.error_stack.nerrors = 0;
    //RC idam_plugin_interface.error_stack.idamerror = nullptr;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = uda::serverRedirectStdStreams(reset)) != 0) {
        UDA_THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin (Error handling is managed within)

    err = plugin.idamPlugin(&idam_plugin_interface);

    // Reset Redirected Output

    reset = 1;
    if ((rc = uda::serverRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, rc, "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) return err;
        return rc;
    }

    return err;
}
