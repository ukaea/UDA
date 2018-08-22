/*---------------------------------------------------------------
* Identify the correct IDAM Data Server Plugin
*---------------------------------------------------------------------------------------------------------------------*/
#include "serverPlugin.h"

#include <stdlib.h>
#include <errno.h>
#include <dlfcn.h>
#include <strings.h>

#include <cache/cache.h>
#include <client/udaClient.h>
#include <clientserver/expand_path.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaErrors.h>
#include <structures/struct.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/makeRequestBlock.h>

#define REQUEST_READ_START      1000
#define REQUEST_PLUGIN_MCOUNT   100    // Maximum initial number of plugins that can be registered
#define REQUEST_PLUGIN_MSTEP    10    // Increase heap by 10 records once the maximum is exceeded

static void parseIDAPath(REQUEST_BLOCK *request_block) {
    char *token=NULL;
    char work[STRING_LENGTH]="";

    if(request_block->path[0] == '\0') return;	// Nothing to work with!

    //------------------------------------------------------------------------------
    // Extract Exp_Number or Source Name

    if(request_block->path[0] == '/')
        strcpy(work, request_block->path+1);	// the leading character is a / so ignore
    else
        strcpy(work, request_block->path);

    token = strtok(work, "/");

    if(token != NULL) {					// Tokenise the remaining path string
        if(IsNumber(token)) {				// Is the First token an integer number?
            request_block->exp_number = atoi(token);	// It must be the Exp_number
            if((token = strtok(NULL, "/")) != NULL) {	// Next Token
                if(IsNumber(token)) {
                    request_block->pass = atoi(token);	// Followed by the Pass number
                } else {
                    strcpy(request_block->tpass, token);	// or something else known to the plugin
                }
            }
            strcpy(request_block->path, "");
            strncpy(request_block->file, request_block->signal, 3);	// IDA Source alias
            request_block->file[3] = '\0';
        }
    } else {
        if(IsNumber(work)) {				// Is the Only token an integer number?
            request_block->exp_number = atoi(work);	// It must be the Exp_number
            strcpy(request_block->path, "");
            strncpy(request_block->file, request_block->signal, 3);
            request_block->file[3] = '\0';
        }
    }
}

static void parseXMLPath(REQUEST_BLOCK *request_block) {
    char *token=NULL;
    char work[STRING_LENGTH]="";

    if(request_block->path[0] == '\0') return;	// Nothing to work with!

    //------------------------------------------------------------------------------
    // Extract Exp_Number and Pass Number

    if(request_block->path[0] == '/')
        strcpy(work, request_block->path+1);	// the leading character is a / so ignore
    else
        strcpy(work, request_block->path);

    token = strtok(work, "/");

    if(token != NULL) {					// Tokenise the remaining string
        if(IsNumber(token)) {				// Is the First token an integer number?
            request_block->exp_number = atoi(token);	// It must be the Exp_number
            if((token = strtok(NULL, "/")) != NULL) {	// Next Token
                if(IsNumber(token)) {
                    request_block->pass = atoi(token);	// Followed by the Pass number
                } else {
                    strcpy(request_block->tpass, token);	// or something else known to the plugin
                }
            }
            strcpy(request_block->path, "");
        }
    } else {
        if(IsNumber(work)) {				// Is the Only token an integer number?
            request_block->exp_number = atoi(work);	// It must be the Exp_number
            strcpy(request_block->path, "");
        }
    }
}

void allocPluginList(int count, PLUGINLIST* plugin_list)
{
    if (count >= plugin_list->mcount) {
        plugin_list->mcount = plugin_list->mcount + REQUEST_PLUGIN_MSTEP;
        plugin_list->plugin = (PLUGIN_DATA*)realloc((void*)plugin_list->plugin,
                                                    plugin_list->mcount * sizeof(PLUGIN_DATA));
    }
}

void closePluginList(const PLUGINLIST* plugin_list)
{
    int i;
    REQUEST_BLOCK request_block;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;
    initRequestBlock(&request_block);
    strcpy(request_block.function, "reset");

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.housekeeping = 1;            // Force a full reset
    idam_plugin_interface.request_block = &request_block;
    for (i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].pluginHandle != NULL) {
            plugin_list->plugin[i].idamPlugin(&idam_plugin_interface);        // Call the housekeeping method
        }
    }
}

void freePluginList(PLUGINLIST* plugin_list)
{
    int i;
    closePluginList(plugin_list);
    for (i = 0; i < plugin_list->count; i++) {
        if (plugin_list->plugin[i].pluginHandle != NULL) {
            dlclose(plugin_list->plugin[i].pluginHandle);
        }
    }
    free((void*)plugin_list->plugin);
    plugin_list->plugin = NULL;
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
    plugin->plugin_class = PLUGINUNKNOWN;
    plugin->external = PLUGINNOTEXTERNAL;
    plugin->status = PLUGINNOTOPERATIONAL;
    plugin->is_private = PLUGINPRIVATE;                    // All services are private: Not accessible to external users
    plugin->cachePermission = PLUGINCACHEDEFAULT;       // Data are OK or Not for the Client to Cache
    plugin->interfaceVersion = 1;                       // Maximum Interface Version
    plugin->pluginHandle = NULL;
    plugin->idamPlugin = NULL;
}

void printPluginList(FILE* fd, const PLUGINLIST* plugin_list)
{
    int i;
    for (i = 0; i < plugin_list->count; i++) {
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

int idamServerRedirectStdStreams(int reset)
{
    // Any OS messages will corrupt xdr streams so re-divert IO from plugin libraries to a temporary file

    static FILE* originalStdFH = NULL;
    static FILE* originalErrFH = NULL;
    static FILE* mdsmsgFH = NULL;

    char* env;
    static char tempFile[MAXPATH];

    static int singleFile = 0;

    if (!reset) {
        if (!singleFile) {
            env = getenv("UDA_PLUGIN_DEBUG_SINGLEFILE");        // Use a single file for all plugin data requests
            if (env != NULL) singleFile = 1;                    // Define IDAM_PLUGIN_DEBUG to retain the file
        }

        if (mdsmsgFH != NULL && singleFile) {
            stdout = mdsmsgFH;                                  // Redirect all IO to a temporary file
            stderr = mdsmsgFH;
            return 0;
        }

        originalStdFH = stdout;                                 // Retain current values
        originalErrFH = stderr;
        mdsmsgFH = NULL;

        UDA_LOG(UDA_LOG_DEBUG, "Redirect standard output to temporary file\n");

        env = getenv("UDA_PLUGIN_REDIVERT");

        if (env == NULL) {
            if ((env = getenv("UDA_WORK_DIR")) != NULL) {
                sprintf(tempFile, "%s/idamPLUGINXXXXXX", env);
            } else {
                strcpy(tempFile, "/tmp/idamPLUGINXXXXXX");
            }
        } else {
            strcpy(tempFile, env);
        }

        // Open the message Trap

        errno = 0;
        int fd = mkstemp(tempFile);
        if (fd < 0 || errno != 0) {
            int err = (errno != 0) ? errno : 994;
            THROW_ERROR(err, "Unable to Obtain a Temporary File Name");
        }

        mdsmsgFH = fdopen(fd, "a");

        if (mdsmsgFH == NULL || errno != 0) {
            THROW_ERROR(999, "Unable to Trap Plugin Error Messages.");
        }

        stdout = mdsmsgFH; // Redirect to a temporary file
        stderr = mdsmsgFH;
    } else {
        if (mdsmsgFH != NULL) {
            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles and removing temporary file\n");

            if (!singleFile) {
                if (mdsmsgFH != NULL) {
                    errno = 0;
                    int rc = fclose(mdsmsgFH);
                    if (rc) {
                        int err = errno;
                        THROW_ERROR(err, strerror(err));
                    }
                }
                mdsmsgFH = NULL;
                if (getenv("UDA_PLUGIN_DEBUG") == NULL) {
                    errno = 0;
                    int rc = remove(tempFile);    // Delete the temporary file
                    if (rc) {
                        int err = errno;
                        THROW_ERROR(err, strerror(err));
                    }
                    tempFile[0] = '\0';
                }
            }

            stdout = originalStdFH;
            stderr = originalErrFH;

        } else {

            UDA_LOG(UDA_LOG_DEBUG, "Resetting original file handles\n");

            stdout = originalStdFH;
            stderr = originalErrFH;
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
int idamServerPlugin(REQUEST_BLOCK* request_block, DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc,
                     const PLUGINLIST* plugin_list, const ENVIRONMENT* environment)
{
    int err = 0;
    char* token = NULL;
    char work[STRING_LENGTH];

    UDA_LOG(UDA_LOG_DEBUG, "Start\n");

    //----------------------------------------------------------------------------
    // Start of Error Trap

    do {

        //----------------------------------------------------------------------------------------------
        // Decode the API Arguments: determine appropriate data reader plug-in

        if ((err = makeRequestBlock(request_block, *plugin_list, environment)) != 0) break;

        UDA_LOG(UDA_LOG_DEBUG, "request_block\n");
        printRequestBlock(*request_block);

        //----------------------------------------------------------------------------------------------
        // Does the Path to Private Files contain hierarchical components not seen by the server?
        // If so make a substitution to resolve path problems.

        if (strlen(request_block->server) == 0 && request_block->request != REQUEST_READ_SERVERSIDE) {
            // Must be a File plugin
            if ((err = pathReplacement(request_block->path, environment)) != 0) break;
        }

        //----------------------------------------------------------------------
        // Some legacy stuff ....

        if (request_block->request == REQUEST_READ_IDA) {
            parseIDAPath(request_block);
        } else {
            if (request_block->request == REQUEST_READ_XML) {
                parseXMLPath(request_block);
            }
        }

        //----------------------------------------------------------------------
        // Copy request details into the data_source structure mimicking a SQL query

        strcpy(data_source->source_alias, TrimString(request_block->file));
        strcpy(data_source->filename, TrimString(request_block->file));
        strcpy(data_source->path, TrimString(request_block->path));

        copyString(TrimString(request_block->signal), signal_desc->signal_name, MAXNAME);

        strcpy(data_source->server, TrimString(request_block->server));

        strcpy(data_source->format, TrimString(request_block->format));
        strcpy(data_source->archive, TrimString(request_block->archive));
        strcpy(data_source->device_name, TrimString(request_block->device_name));

        data_source->exp_number = request_block->exp_number;
        data_source->pass = request_block->pass;
        data_source->type = ' ';

        // Legacy Exceptions ...

        switch (request_block->request) {

            case REQUEST_READ_MDS:

                if (strlen(signal_desc->signal_name) == MAXNAME - 1) {
                    copyString(TrimString(request_block->signal), signal_desc->xml, MAXMETA);    // Pass via XML member
                    signal_desc->signal_name[0] = '\0';
                }
                break;

            case REQUEST_READ_NOTHING:

                if (data_source->exp_number == 0 && data_source->pass == -1) {    // May be passed in Path String
                    strcpy(work, request_block->path);
                    if (work[0] == '/' && (token = strtok(work, "/")) != NULL) {    // Tokenise the remaining string
                        if (IsNumber(token)) {                    // Is the First token an integer number?
                            request_block->exp_number = atoi(token);
                            if ((token = strtok(NULL, "/")) != NULL) {        // Next Token
                                if (IsNumber(token)) {
                                    request_block->pass = atoi(token);        // Must be the Pass number
                                } else {
                                    strcpy(request_block->tpass, token);        // anything else
                                }
                            }
                        }
                        data_source->exp_number = request_block->exp_number;        // Size of Data Block
                        data_source->pass = request_block->pass;        // Compressible or Not
                    }
                }
                break;
        }

        if (err != 0) break;

        //------------------------------------------------------------------------------------------------
        // Trap any unexpected output to stdout or stderr

        //------------------------------------------------------------------------------------------------
        // Locate and Execute the Required Plugin

        //------------------------------------------------------------------------------------------------
        // End of Error Trap

    } while (0);

    UDA_LOG(UDA_LOG_DEBUG, "End\n");

    return err;
}

//------------------------------------------------------------------------------------------------
// Provenance gathering plugin with a separate database.
// Functionality exposed to both server (special plugin with standard methods)
// and client application (behaves as a normal plugin)
//
// Server needs are (private to the server):
//	record (put) the original and the actual signal and source terms with the source file DOI
//	record (put) the server log record
// Client needs are (the plugin exposes these to the client in the regular manner):
//	list all provenance records for a specific client DOI - must be given
//	change provenance records status to closed
//	delete all closed records for a specific client DOI
//
// changePlugin option disabled in this context
// private malloc log and userdefinedtypelist

int idamProvenancePlugin(CLIENT_BLOCK* client_block, REQUEST_BLOCK* original_request_block,
                         DATA_SOURCE* data_source, SIGNAL_DESC* signal_desc, const PLUGINLIST* plugin_list,
                         char* logRecord, const ENVIRONMENT* environment)
{

    if (strcmp(client_block->DOI, "") || strlen(client_block->DOI) == 0) {
        // No Provenance to Capture
        return 0;
    }

    // Identify the Provenance Gathering plugin (must be a function library type plugin)

    static int plugin_id = -2;
    static int execMethod = 1;        // The default method used to write efficiently to the backend SQL server
    char* env = NULL;

    struct timeval tv_start, tv_stop;

    gettimeofday(&tv_start, NULL);

    if (plugin_id == -2) {        // On initialisation
        plugin_id = -1;
        if ((env = getenv("UDA_PROVENANCE_PLUGIN")) != NULL) {
            // Must be set in the server startup script
            UDA_LOG(UDA_LOG_DEBUG, "Plugin name: %s\n", env);
            int id = findPluginIdByFormat(env, plugin_list); // Must be defined in the server plugin configuration file
            UDA_LOG(UDA_LOG_DEBUG, "Plugin id: %d\n", id);
            if (id >= 0) {
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].plugin_class == PLUGINFUNCTION = %d\n",
                        plugin_list->plugin[id].plugin_class == PLUGINFUNCTION);
                UDA_LOG(UDA_LOG_DEBUG, "!environment->external_user = %d\n", !environment->external_user);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].status == PLUGINOPERATIONAL = %d\n",
                        plugin_list->plugin[id].status == PLUGINOPERATIONAL);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].pluginHandle != NULL = %d\n",
                        plugin_list->plugin[id].pluginHandle != NULL);
                UDA_LOG(UDA_LOG_DEBUG, "plugin_list->plugin[id].idamPlugin   != NULL = %d\n",
                        plugin_list->plugin[id].idamPlugin != NULL);
            }
            if (id >= 0 &&
                plugin_list->plugin[id].plugin_class == PLUGINFUNCTION &&
                !environment->external_user &&
                plugin_list->plugin[id].status == PLUGINOPERATIONAL &&
                plugin_list->plugin[id].pluginHandle != NULL &&
                plugin_list->plugin[id].idamPlugin != NULL) {
                plugin_id = id;
            }
        }
        if ((env = getenv("UDA_PROVENANCE_EXEC_METHOD")) != NULL) {
            execMethod = atoi(env);
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "Plugin id: %d\n", plugin_id);

    if (plugin_id <= 0) return 0;    // Not possible to record anything - no provenance plugin!

    REQUEST_BLOCK request_block;
    initRequestBlock(&request_block);
    strcpy(request_block.api_delim, "::");
    strcpy(request_block.source, "");

    // need 1> record the original and the actual signal and source terms with the source file DOI
    // mimic a client request

    if (logRecord == NULL || strlen(logRecord) == 0) {
        sprintf(request_block.signal, "%s::putSignal(uuid='%s',requestedSignal='%s',requestedSource='%s', "
                                      "trueSignal='%s', trueSource='%s', trueSourceDOI='%s', execMethod=%d, status=new)",
                plugin_list->plugin[plugin_id].format, client_block->DOI,
                original_request_block->signal, original_request_block->source,
                signal_desc->signal_name, data_source->path, "", execMethod);
    } else {

        // need 2> record the server log record

        sprintf(request_block.signal, "%s::putSignal(uuid='%s',logRecord='%s', execMethod=%d, status=update)",
                plugin_list->plugin[plugin_id].format, client_block->DOI, logRecord, execMethod);
    }

    // Activate the plugin

    UDA_LOG(UDA_LOG_DEBUG, "Provenance Plugin signal: %s\n", request_block.signal);

    makeRequestBlock(&request_block, *plugin_list, environment);

    int err, rc, reset;
    DATA_BLOCK data_block;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;

    // Initialise the Data Block

    initDataBlock(&data_block);

    UDA_LOG(UDA_LOG_DEBUG, "Creating plugin interface\n");

    // Check the Interface Compliance

    if (plugin_list->plugin[plugin_id].interfaceVersion > 1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "idamProvenancePlugin", err,
                     "The Provenance Plugin's Interface Version is not Implemented.");
        return err;
    }

    USERDEFINEDTYPELIST* userdefinedtypelist = NULL;
    copyUserDefinedTypeList(&userdefinedtypelist);                // Allocate and Copy the Master User Defined Type List

    LOGMALLOCLIST* logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
    initLogMallocList(logmalloclist);

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.pluginVersion = 0;
    idam_plugin_interface.sqlConnectionType = 0;
    idam_plugin_interface.data_block = &data_block;
    idam_plugin_interface.client_block = client_block;
    idam_plugin_interface.request_block = &request_block;
    idam_plugin_interface.data_source = data_source;
    idam_plugin_interface.signal_desc = signal_desc;
    idam_plugin_interface.environment = environment;
    idam_plugin_interface.sqlConnection = NULL;        // Private to the plugin
    idam_plugin_interface.housekeeping = 0;
    idam_plugin_interface.changePlugin = 0;
    idam_plugin_interface.pluginList = plugin_list;
    idam_plugin_interface.userdefinedtypelist = userdefinedtypelist;
    idam_plugin_interface.logmalloclist = logmalloclist;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = idamServerRedirectStdStreams(reset)) != 0) {
        addIdamError(CODEERRORTYPE, "idamProvenancePlugin", err,
                     "Error Redirecting Plugin Message Output");
        return err;
    }

    // Call the plugin

    UDA_LOG(UDA_LOG_DEBUG, "entering the provenance plugin\n");

    err = plugin_list->plugin[plugin_id].idamPlugin(&idam_plugin_interface);

    UDA_LOG(UDA_LOG_DEBUG, "returned from the provenance plugin\n");

    // No data are returned in this context so free everything

    UDA_LOG(UDA_LOG_DEBUG, "housekeeping\n");

    freeMallocLogList(logmalloclist);
    free((void*)logmalloclist);

    freeUserDefinedTypeList(userdefinedtypelist);
    free((void*)userdefinedtypelist);
    userdefinedtypelist = NULL;

    freeNameValueList(&request_block.nameValueList);

    UDA_LOG(UDA_LOG_DEBUG, "testing for bug!!!\n");
    if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN ||
        data_block.opaque_count != 0 ||
        data_block.opaque_block != NULL) {
        UDA_LOG(UDA_LOG_DEBUG, "bug detected: mitigation!!!\n");
        data_block.opaque_block = NULL;
    }

    freeDataBlock(&data_block);

    // Reset Redirected Output

    reset = 1;
    if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(CODEERRORTYPE, "idamProvenancePlugin", rc,
                         "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) return err;
        return rc;
    }

    gettimeofday(&tv_stop, NULL);
    int msecs = (int)(tv_stop.tv_sec - tv_start.tv_sec) * 1000 + (int)(tv_stop.tv_usec - tv_start.tv_usec) / 1000;

    UDA_LOG(UDA_LOG_DEBUG, "end of housekeeping\n");
    UDA_LOG(UDA_LOG_DEBUG, "Timing (ms) = %d\n", msecs);

    return 0;
}

//------------------------------------------------------------------------------------------------
// Identify the Plugin to use to resolve Generic Name mappings and return its ID 

int idamServerMetaDataPluginId(const PLUGINLIST* plugin_list, const ENVIRONMENT* environment)
{
    static unsigned short noPluginRegistered = 0;
    static int plugin_id = -1;

    UDA_LOG(UDA_LOG_DEBUG, "Entered: noPluginRegistered state = %d\n", noPluginRegistered);
    UDA_LOG(UDA_LOG_DEBUG, "Entered: plugin_id state = %d\n", plugin_id);

    if (plugin_id >= 0) return plugin_id;     // Plugin previously identified
    if (noPluginRegistered) return -1;        // No Plugin for the MetaData Catalog to resolve Generic Name mappings

    // Identify the MetaData Catalog plugin (must be a function library type plugin)

    char* env = NULL;
    if ((env = getenv("UDA_METADATA_PLUGIN")) != NULL) {        // Must be set in the server startup script
        int id = findPluginIdByFormat(env, plugin_list);        // Must be defined in the server plugin configuration file
        if (id >= 0 &&
            plugin_list->plugin[id].plugin_class == PLUGINFUNCTION &&
            plugin_list->plugin[id].status == PLUGINOPERATIONAL &&
            plugin_list->plugin[id].pluginHandle != NULL &&
            plugin_list->plugin[id].idamPlugin != NULL) {
            plugin_id = (short)id;
        }

        if (id >= 0 && plugin_list->plugin[id].is_private == PLUGINPRIVATE && environment->external_user) {
            plugin_id = -1;
        }        // Not available to external users

        UDA_LOG(UDA_LOG_DEBUG, "Generic Name Mapping Plugin Name: %s\n", env);
        UDA_LOG(UDA_LOG_DEBUG, "PLUGINFUNCTION?: %d\n", plugin_list->plugin[id].plugin_class == PLUGINFUNCTION);
        UDA_LOG(UDA_LOG_DEBUG, "PLUGINPRIVATE?: %d\n", plugin_list->plugin[id].is_private == PLUGINPRIVATE);
        UDA_LOG(UDA_LOG_DEBUG, "External User?: %d\n", environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "Private?: %d\n",
                plugin_list->plugin[id].is_private == PLUGINPRIVATE && environment->external_user);
        UDA_LOG(UDA_LOG_DEBUG, "PLUGINOPERATIONAL?: %d\n", plugin_list->plugin[id].status == PLUGINOPERATIONAL);
        UDA_LOG(UDA_LOG_DEBUG, "Plugin OK?: %d\n",
                plugin_list->plugin[id].pluginHandle != NULL && plugin_list->plugin[id].idamPlugin != NULL);
        UDA_LOG(UDA_LOG_DEBUG, "id: %d\n", id);
        UDA_LOG(UDA_LOG_DEBUG, "id: %d\n", plugin_id);
    } else {
        UDA_LOG(UDA_LOG_DEBUG, "NO Generic Name Mapping Plugin identified\n");
    }

    if (plugin_id < 0) noPluginRegistered = 1;        // No Plugin found (registered)

    return plugin_id;
}

//------------------------------------------------------------------------------------------------
// Execute the Generic Name mapping Plugin

int idamServerMetaDataPlugin(const PLUGINLIST* plugin_list, int plugin_id, REQUEST_BLOCK* request_block,
                             SIGNAL_DESC* signal_desc, DATA_SOURCE* data_source, LOGMALLOCLIST* logmalloclist,
                             const ENVIRONMENT* environment)
{
    int err, reset, rc;
    IDAM_PLUGIN_INTERFACE idam_plugin_interface;

    // Check the Interface Compliance

    if (plugin_list->plugin[plugin_id].interfaceVersion > 1) {
        err = 999;
        addIdamError(CODEERRORTYPE, "idamServerMetaDataPlugin", err,
                     "The Plugin's Interface Version is not Implemented.");
        return err;
    }

    USERDEFINEDTYPELIST* userdefinedtypelist = NULL;
    copyUserDefinedTypeList(&userdefinedtypelist);

    idam_plugin_interface.interfaceVersion = 1;
    idam_plugin_interface.pluginVersion = 0;
    idam_plugin_interface.sqlConnectionType = 0;
    idam_plugin_interface.data_block = NULL;
    idam_plugin_interface.client_block = NULL;
    idam_plugin_interface.request_block = request_block;
    idam_plugin_interface.data_source = data_source;
    idam_plugin_interface.signal_desc = signal_desc;
    idam_plugin_interface.environment = environment;    // Legacy Global variable
    idam_plugin_interface.sqlConnection = NULL;        // Private to the plugin
    idam_plugin_interface.housekeeping = 0;
    idam_plugin_interface.changePlugin = 0;
    idam_plugin_interface.pluginList = plugin_list;
    idam_plugin_interface.userdefinedtypelist = userdefinedtypelist;
    idam_plugin_interface.logmalloclist = logmalloclist;

    // Redirect Output to temporary file if no file handles passed

    reset = 0;
    if ((err = idamServerRedirectStdStreams(reset)) != 0) {
        addIdamError(CODEERRORTYPE, "idamServerMetaDataPlugin", err,
                     "Error Redirecting Plugin Message Output");
        return err;
    }

// Call the plugin (Error handling is managed within)

    err = plugin_list->plugin[plugin_id].idamPlugin(&idam_plugin_interface);

// Reset Redirected Output 

    reset = 1;
    if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
        if (rc != 0) {
            addIdamError(CODEERRORTYPE, "idamServerMetaDataPlugin", rc,
                         "Error Resetting Redirected Plugin Message Output");
        }
        if (err != 0) return err;
        return rc;
    }

    return err;
}
