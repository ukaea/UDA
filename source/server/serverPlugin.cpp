/*---------------------------------------------------------------
* Identify the correct UDA Data Server Plugin
*---------------------------------------------------------------------------------------------------------------------*/
#include "serverPlugin.h"

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

void allocPluginList(int count, PLUGINLIST* plugin_list)
{
    if (count >= plugin_list->mcount) {
        plugin_list->mcount = plugin_list->mcount + REQUEST_PLUGIN_MSTEP;
        plugin_list->plugin = (PLUGIN_DATA*)realloc((void*)plugin_list->plugin,
                                                    plugin_list->mcount * sizeof(PLUGIN_DATA));
    }
}

void resetPlugins(const PLUGINLIST* plugin_list)
{
    REQUEST_DATA request_block;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;
    initRequestData(&request_block);
    strcpy(request_block.function, "reset");

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.housekeeping = 1;            // Force a full reset
    idam_plugin_interface.request_data = &request_block;
    for (int i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].pluginHandle != nullptr) {
            plugin_list->plugin[i].idamPlugin(&idam_plugin_interface);        // Call the housekeeping method
        }
    }
}

void freePluginList(PLUGINLIST* plugin_list)
{
    resetPlugins(plugin_list);
    for (int i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].pluginHandle != nullptr) {
            dlclose(plugin_list->plugin[i].pluginHandle);
        }
    }
    free(plugin_list->plugin);
    plugin_list->plugin = nullptr;
    plugin_list->count = 0;
    plugin_list->mcount = 0;
}

void initPluginData(PLUGIN_DATA* plugin)
{
    plugin->format[0] = '\0';
    plugin->library[0] = '\0';
    plugin->symbol[0] = '\0';
    plugin->extension[0] = '\0';
    plugin->desc[0] = '\0';
    plugin->example[0] = '\0';
    plugin->method[0] = '\0';
    plugin->deviceProtocol[0] = '\0';
    plugin->deviceHost[0] = '\0';
    plugin->devicePort[0] = '\0';
    plugin->request = REQUEST_READ_UNKNOWN;
    plugin->plugin_class = UDA_PLUGIN_CLASS_UNKNOWN;
    plugin->external = UDA_PLUGIN_INTERNAL;
    plugin->status = UDA_PLUGIN_NOT_OPERATIONAL;
    plugin->is_private = UDA_PLUGIN_PRIVATE;            // All services are private: Not accessible to external users
    plugin->cachePermission = UDA_PLUGIN_CACHE_DEFAULT; // Data are OK or Not for the Client to Cache
    plugin->interfaceVersion = 1;                       // Maximum Interface Version
    plugin->pluginHandle = nullptr;
    plugin->idamPlugin = nullptr;
}

void printPluginList(FILE* fd, const PLUGINLIST* plugin_list)
{
    for (int i = 0; i < plugin_list->count; i++) {
        fprintf(fd, "Request    : %d\n", plugin_list->plugin[i].request);
        fprintf(fd, "Format     : %s\n", plugin_list->plugin[i].format);
        fprintf(fd, "Library    : %s\n", plugin_list->plugin[i].library);
        fprintf(fd, "Symbol     : %s\n", plugin_list->plugin[i].symbol);
        fprintf(fd, "Extension  : %s\n", plugin_list->plugin[i].extension);
        fprintf(fd, "Method     : %s\n", plugin_list->plugin[i].method);
        fprintf(fd, "Description: %s\n", plugin_list->plugin[i].desc);
        fprintf(fd, "Example    : %s\n", plugin_list->plugin[i].example);
        fprintf(fd, "Protocol   : %s\n", plugin_list->plugin[i].deviceProtocol);
        fprintf(fd, "Host       : %s\n", plugin_list->plugin[i].deviceHost);
        fprintf(fd, "Port       : %s\n", plugin_list->plugin[i].devicePort);
        fprintf(fd, "Class      : %d\n", plugin_list->plugin[i].plugin_class);
        fprintf(fd, "External   : %d\n", plugin_list->plugin[i].external);
        fprintf(fd, "Status     : %d\n", plugin_list->plugin[i].status);
        fprintf(fd, "Private    : %d\n", plugin_list->plugin[i].is_private);
        fprintf(fd, "cachePermission : %d\n", plugin_list->plugin[i].cachePermission);
        fprintf(fd, "interfaceVersion: %d\n\n", plugin_list->plugin[i].interfaceVersion);
    }
}

int udaServerRedirectStdStreams(int reset)
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
                    sprintf(mksdir_template, "%s/idamPLUGINXXXXXX", env);
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
            THROW_ERROR(err, "Unable to Obtain a Temporary File Name");
        }

        mdsmsgFH = fdopen(fd, "a");

        if (mdsmsgFH == nullptr || errno != 0) {
            THROW_ERROR(999, "Unable to Trap Plugin Error Messages.");
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
                        THROW_ERROR(err, strerror(err));
                    }
                }
                mdsmsgFH = nullptr;
                if (getenv("UDA_PLUGIN_DEBUG") == nullptr) {
                    errno = 0;
                    int rc = remove(tempFile);    // Delete the temporary file
                    if (rc) {
                        int err = errno;
                        THROW_ERROR(err, strerror(err));
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
int udaServerPlugin(REQUEST_DATA* request, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc,
                    const PLUGINLIST* plugin_list, const ENVIRONMENT* environment)
{
    int err = 0;

    UDA_LOG(UDA_LOG_DEBUG, "Start\n");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data reader plug-in

    if ((err = makeRequestData(request, *plugin_list, environment)) != 0) {
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

int udaProvenancePlugin(CLIENT_BLOCK* client_block, REQUEST_DATA* original_request,
                        DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, const PLUGINLIST* plugin_list,
                        const char* logRecord, const ENVIRONMENT* environment)
{

    if (STR_EQUALS(client_block->DOI, "")) {
        // No Provenance to Capture
        return 0;
    }

    // Identify the Provenance Gathering plugin (must be a function library type plugin)

    static int plugin_id = -2;
    static int execMethod = 1;        // The default method used to write efficiently to the backend SQL server
    char* env = nullptr;

    struct timeval tv_start = {};
    struct timeval tv_stop = {};

    gettimeofday(&tv_start, nullptr);

    if (plugin_id == -2) {        // On initialisation
        plugin_id = -1;
        if ((env = getenv("UDA_PROVENANCE_PLUGIN")) != nullptr) {
            // Must be set in the server startup script
            UDA_LOG(UDA_LOG_DEBUG, "Plugin name: %s\n", env);
            int id = findPluginIdByFormat(env, plugin_list); // Must be defined in the server plugin configuration file
            UDA_LOG(UDA_LOG_DEBUG, "Plugin id: %d\n", id);
            if (id >= 0) {
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION = %d\n",
                        plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION);
                UDA_LOG(UDA_LOG_DEBUG, "!environment->external_user = %d\n", !environment->external_user);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL = %d\n",
                        plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].pluginHandle != nullptr = %d\n",
                        plugin_list->plugin[id].pluginHandle != nullptr);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].idamPlugin   != nullptr = %d\n",
                        plugin_list->plugin[id].idamPlugin != nullptr);
            }
            if (id >= 0 &&
                plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
                !environment->external_user &&
                plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL &&
                plugin_list->plugin[id].pluginHandle != nullptr &&
                plugin_list->plugin[id].idamPlugin != nullptr) {
                plugin_id = id;
            }
        }
        if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != nullptr) {
            execMethod = atoi(env);
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Plugin id: %d\n", plugin_id);

    if (plugin_id <= 0) {
        // Not possible to record anything - no provenance plugin!
        return 0;
    }

    REQUEST_DATA request;
    initRequestData(&request);

    strcpy(request.api_delim, "::");
    strcpy(request.source, "");

    // need 1> record the original and the actual signal and source terms with the source file DOI
    // mimic a client request

    if (logRecord == nullptr || strlen(logRecord) == 0) {
        sprintf(request.signal, "%s::putSignal(uuid='%s',requestedSignal='%s',requestedSource='%s', "
                                      "trueSignal='%s', trueSource='%s', trueSourceDOI='%s', execMethod=%d, status=new)",
                plugin_list->plugin[plugin_id].format, client_block->DOI,
                original_request->signal, original_request->source,
                signal_desc->signal_name, data_source->path, "", execMethod);
    } else {

        // need 2> record the server log record

        sprintf(request.signal, "%s::putSignal(uuid='%s',logRecord='%s', execMethod=%d, status=update)",
                plugin_list->plugin[plugin_id].format, client_block->DOI, logRecord, execMethod);
    }

    // Activate the plugin

    UDA_LOG(UDA_LOG_DEBUG, "Provenance Plugin signal: %s\n", request.signal);

    makeRequestData(&request, *plugin_list, environment);

    int err, rc, reset;
    DATA_BLOCK data_block;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;

    // Initialise the Data Block

    initDataBlock(&data_block);

    UDA_LOG(UDA_LOG_DEBUG, "Creating plugin interface\n");

    // Check the Interface Compliance

    if (plugin_list->plugin[plugin_id].interfaceVersion > 1) {
        THROW_ERROR(999, "The Provenance Plugin's Interface Version is not Implemented.");
    }

    USERDEFINEDTYPELIST userdefinedtypelist;
    initUserDefinedTypeList(&userdefinedtypelist);

    LOGMALLOCLIST logmalloclist;
    initLogMallocList(&logmalloclist);

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.pluginVersion = 0;
    idam_plugin_interface.data_block = &data_block;
    idam_plugin_interface.client_block = client_block;
    idam_plugin_interface.request_data = &request;
    idam_plugin_interface.data_source = data_source;
    idam_plugin_interface.signal_desc = signal_desc;
    idam_plugin_interface.environment = environment;
    idam_plugin_interface.housekeeping = 0;
    idam_plugin_interface.changePlugin = 0;
    idam_plugin_interface.pluginList = plugin_list;
    idam_plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    idam_plugin_interface.logmalloclist = &logmalloclist;
    idam_plugin_interface.error_stack.nerrors = 0;
    idam_plugin_interface.error_stack.idamerror = nullptr;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = udaServerRedirectStdStreams(reset)) != 0) {
        THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin

    UDA_LOG(UDA_LOG_DEBUG, "entering the provenance plugin\n");

    err = plugin_list->plugin[plugin_id].idamPlugin(&idam_plugin_interface);

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
    if ((rc = udaServerRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(CODEERRORTYPE, __func__, rc, "Error Resetting Redirected Plugin Message Output");
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

int udaServerMetaDataPluginId(const PLUGINLIST* plugin_list, const ENVIRONMENT* environment)
{
    static unsigned short noPluginRegistered = 0;
    static int plugin_id = -1;

    UDA_LOG(UDA_LOG_DEBUG, "Entered: noPluginRegistered state = %d\n", noPluginRegistered);
    UDA_LOG(UDA_LOG_DEBUG, "Entered: plugin_id state = %d\n", plugin_id);

    if (plugin_id >= 0) return plugin_id;     // Plugin previously identified
    if (noPluginRegistered) return -1;        // No Plugin for the MetaData Catalog to resolve Generic Name mappings

    // Identify the MetaData Catalog plugin (must be a function library type plugin)

    char* env = nullptr;
    if ((env = getenv("UDA_METADATA_PLUGIN")) != nullptr) {        // Must be set in the server startup script
        int id = findPluginIdByFormat(env, plugin_list);        // Must be defined in the server plugin configuration file
        if (id >= 0 &&
            plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION &&
            plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL &&
            plugin_list->plugin[id].pluginHandle != nullptr &&
            plugin_list->plugin[id].idamPlugin != nullptr) {
            plugin_id = (short)id;
        }

        if (id >= 0 && plugin_list->plugin[id].is_private == UDA_PLUGIN_PRIVATE && environment->external_user) {
            // Not available to external users
            plugin_id = -1;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Generic Name Mapping Plugin Name: %s\n", env);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_CLASS_FUNCTION?: %d\n", plugin_list->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FUNCTION);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_PRIVATE?: %d\n", plugin_list->plugin[id].is_private == UDA_PLUGIN_PRIVATE);
        UDA_LOG(UDA_LOG_DEBUG, "External User?: %d\n", environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "Private?: %d\n",
                plugin_list->plugin[id].is_private == UDA_PLUGIN_PRIVATE && environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "UDA_PLUGIN_OPERATIONAL?: %d\n", plugin_list->plugin[id].status == UDA_PLUGIN_OPERATIONAL);
        UDA_LOG(UDA_LOG_DEBUG, "Plugin OK?: %d\n",
                plugin_list->plugin[id].pluginHandle != nullptr && plugin_list->plugin[id].idamPlugin != nullptr);
        UDA_LOG(UDA_LOG_DEBUG, "id: %d\n", id);
        UDA_LOG(UDA_LOG_DEBUG, "plugin_id: %d\n", plugin_id);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin identified\n");
    }

    if (plugin_id < 0) noPluginRegistered = 1;        // No Plugin found (registered)

    return plugin_id;
}

//------------------------------------------------------------------------------------------------
// Execute the Generic Name mapping Plugin

int udaServerMetaDataPlugin(const PLUGINLIST* plugin_list, int plugin_id, REQUEST_DATA* request_block,
                            SIGNAL_DESC* signal_desc, SIGNAL* signal_rec, DATA_SOURCE* data_source,
                            const ENVIRONMENT* environment)
{
    int err, reset, rc;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;

    // Check the Interface Compliance

    if (plugin_list->plugin[plugin_id].interfaceVersion > 1) {
        THROW_ERROR(999, "The Plugin's Interface Version is not Implemented.");
    }

    DATA_BLOCK data_block;
    initDataBlock(&data_block);
    data_block.signal_rec = signal_rec;

    USERDEFINEDTYPELIST userdefinedtypelist;
    initUserDefinedTypeList(&userdefinedtypelist);

    LOGMALLOCLIST logmalloclist;
    initLogMallocList(&logmalloclist);

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.pluginVersion = 0;
    idam_plugin_interface.data_block = &data_block;
    idam_plugin_interface.client_block = nullptr;
    idam_plugin_interface.request_data = request_block;
    idam_plugin_interface.data_source = data_source;
    idam_plugin_interface.signal_desc = signal_desc;
    idam_plugin_interface.environment = environment;
    idam_plugin_interface.housekeeping = 0;
    idam_plugin_interface.changePlugin = 0;
    idam_plugin_interface.pluginList = plugin_list;
    idam_plugin_interface.userdefinedtypelist = &userdefinedtypelist;
    idam_plugin_interface.logmalloclist = &logmalloclist;
    idam_plugin_interface.error_stack.nerrors = 0;
    idam_plugin_interface.error_stack.idamerror = nullptr;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = udaServerRedirectStdStreams(reset)) != 0) {
        THROW_ERROR(err, "Error Redirecting Plugin Message Output");
    }

    // Call the plugin (Error handling is managed within)

    err = plugin_list->plugin[plugin_id].idamPlugin(&idam_plugin_interface);

    // Reset Redirected Output

    reset = 1;
    if ((rc = udaServerRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(CODEERRORTYPE, __func__, rc, "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) return err;
        return rc;
    }

    return err;
}
