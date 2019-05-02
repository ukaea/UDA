//--------------------------------------------------------------------------------------------------------------------
// Generic Data Access Tool
// XML defined Derived Signals
// XML corrections to Data
//
// Return Codes:	0 => OK, otherwise Error
//
//--------------------------------------------------------------------------------------------------------------------
#include <errno.h>
#include <strings.h>

#include <clientserver/errorLog.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/printStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/udaErrors.h>
#include <logging/logging.h>
#include <structures/struct.h>
#include <clientserver/protocol.h>

#include "getServerEnvironment.h"
#include "serverGetData.h"
#include "serverPlugin.h"
#include "udaServer.h"

int udaGetData(REQUEST_BLOCK request_block, CLIENT_BLOCK client_block, DATA_BLOCK* data_block, DATA_SOURCE* data_source,
               SIGNAL* signal_rec, SIGNAL_DESC* signal_desc, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
               USERDEFINEDTYPELIST* userdefinedtypelist)
{
    // If err = 0 then standard signal data read
    // If err > 0 then an error occured
    // If err < 0 then unable to read signal because it is a derived type and details are in XML format

    printRequestBlock(request_block);

    //------------------------------------------------------------------------------
    // Identify the Signal Required from the Database if a Generic Signal Requested
    // Plugin sourced data (type 'P') will fail as there is no entry in the DATA_SOURCE table so ignore
    //------------------------------------------------------------------------------

    if (request_block.request == REQUEST_READ_GENERIC) {

        // Identify the required Plugin

        int plugin_id = idamServerMetaDataPluginId(pluginlist, getIdamServerEnvironment());
        if (plugin_id < 0) {
            // No plugin so not possible to identify the requested data item
            THROW_ERROR(778, "Unable to identify requested data item");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin ID = %d\nExecuting the plugin\n", plugin_id);

        // If the plugin is registered as a FILE or LIBRARY type then call the default method as no method will have been specified

        strcpy(request_block.function, pluginlist->plugin[plugin_id].method);

        // Execute the plugin to resolve the identity of the data requested

        int err = idamServerMetaDataPlugin(pluginlist, plugin_id, &request_block, signal_desc, signal_rec, data_source,
                                           getIdamServerEnvironment());

        if (err != 0) {
            THROW_ERROR(err, "No Record Found for this Generic Signal");
        }
        UDA_LOG(UDA_LOG_DEBUG, "Metadata Plugin Executed\n");
        UDA_LOG(UDA_LOG_DEBUG, "Signal Type: %c\n", signal_desc->type);

        // Plugin? Create a new Request Block to identify the request_id

        if (signal_desc->type == 'P') {
            strcpy(request_block.signal, signal_desc->signal_name);
            strcpy(request_block.source, data_source->path);
            makeRequestBlock(&request_block, *pluginlist, getIdamServerEnvironment()); // Includes placeholder substitution
        }
    } // end of REQUEST_READ_GENERIC

    ENVIRONMENT* environment = getIdamServerEnvironment();

    //------------------------------------------------------------------------------
    // Read Data via a Suitable Registered Plugin using a standard interface
    //------------------------------------------------------------------------------

    // Test for known File formats and Server protocols

    {
        IDAM_PLUGIN_INTERFACE idam_plugin_interface;

        UDA_LOG(UDA_LOG_DEBUG, "creating the plugin interface structure\n");

        // Initialise the Data Block

        initDataBlock(data_block);

        idam_plugin_interface.interfaceVersion = 1;
        idam_plugin_interface.pluginVersion = 0;
        idam_plugin_interface.data_block = data_block;
        idam_plugin_interface.client_block = &client_block;
        idam_plugin_interface.request_block = &request_block;
        idam_plugin_interface.data_source = data_source;
        idam_plugin_interface.signal_desc = signal_desc;
        idam_plugin_interface.environment = environment;
        idam_plugin_interface.verbose = 0;
        idam_plugin_interface.housekeeping = 0;
        idam_plugin_interface.changePlugin = 0;
        idam_plugin_interface.pluginList = pluginlist;
        idam_plugin_interface.userdefinedtypelist = userdefinedtypelist;
        idam_plugin_interface.logmalloclist = logmalloclist;

        if (request_block.request == REQUEST_READ_GENERIC) {
            // via Generic database query
            request_block.request = findPluginRequestByFormat(data_source->format, pluginlist);
            UDA_LOG(UDA_LOG_DEBUG, "findPluginRequestByFormat Plugin Request ID %d\n", request_block.request);
        }

        UDA_LOG(UDA_LOG_DEBUG, "(idamServerGetData) Number of PutData Blocks: %d\n",
                request_block.putDataBlockList.blockCount);

        if (request_block.request != REQUEST_READ_UNKNOWN) {

            int id = findPluginIdByRequest(request_block.request, pluginlist);

            if (id == -1) {
                UDA_LOG(UDA_LOG_DEBUG, "Error locating data plugin %d\n", request_block.request);
                THROW_ERROR(999, "Error locating data plugin");
            }

#ifndef ITERSERVER
            if (pluginlist->plugin[id].is_private == UDA_PLUGIN_PRIVATE && environment->external_user) {
                THROW_ERROR(999, "Access to this data class is not available.");
            }
#endif
            if (pluginlist->plugin[id].external == UDA_PLUGIN_EXTERNAL &&
                pluginlist->plugin[id].status == UDA_PLUGIN_OPERATIONAL &&
                pluginlist->plugin[id].pluginHandle != NULL &&
                pluginlist->plugin[id].idamPlugin != NULL) {

                UDA_LOG(UDA_LOG_DEBUG, "[%d] %s Plugin Selected\n", id, data_source->format);

                // Redirect Output to temporary file if no file handles passed

                int err;

#ifndef FATCLIENT
                int reset = 0;
                if ((err = idamServerRedirectStdStreams(reset)) != 0) {
                    THROW_ERROR(err, "Error Redirecting Plugin Message Output");
                }
#endif

                if (request_block.function[0] == '\0') {
                    strcpy(request_block.function, pluginlist->plugin[id].method);
                }

                if (request_block.nameValueList.listSize == 0
                    && pluginlist->plugin[id].plugin_class == UDA_PLUGIN_CLASS_FILE) {
                    char* pairs = FormatString("file=%s, signal=%s", request_block.path, signal_desc->signal_name);
                    nameValuePairs(pairs, &request_block.nameValueList, true);
                    free(pairs);
                }

                // Call the plugin

                err = pluginlist->plugin[id].idamPlugin(&idam_plugin_interface);

                // Reset Redirected Output

#ifndef FATCLIENT
                reset = 1;
                int rc;
                if ((rc = idamServerRedirectStdStreams(reset)) != 0 || err != 0) {
                    if (rc != 0) {
                        addIdamError(CODEERRORTYPE, __func__, rc, "Error Resetting Redirected Plugin Message Output");
                    }
                    if (err != 0) {
                        return err;
                    }
                    return rc;
                }
#else
                if (err != 0) {
                    return err;
                }
#endif

                UDA_LOG(UDA_LOG_DEBUG, "returned from plugin called\n");

                // Save Provenance with socket stream protection

                idamServerRedirectStdStreams(0);
                idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, pluginlist, NULL,
                                     getIdamServerEnvironment());
                idamServerRedirectStdStreams(1);

                // If no structures to pass back (only regular data) then free the user defined type list

                if (data_block->opaque_block == NULL) {
                    if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES && data_block->opaque_count > 0) {
                        THROW_ERROR(999, "Opaque Data Block is Null Pointer");
                    }
                }

                if (!idam_plugin_interface.changePlugin) {
                    // job done!
                    return 0;
                }

                request_block.request = REQUEST_READ_GENERIC;            // Use a different Plugin
            }
        }
    }

    int plugin_id = request_block.request;

    if (plugin_id == REQUEST_READ_UNKNOWN) {
        UDA_LOG(UDA_LOG_DEBUG, "No Plugin Selected\n");
    }
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source->pass);

    //----------------------------------------------------------------------------
    // Initialise the Data Block Structure

    initDataBlock(data_block);

    //----------------------------------------------------------------------------
    // Copy the Client Block into the Data Block to pass client requested properties into plugins

    data_block->client_block = client_block;

    //----------------------------------------------------------------------------
    // Save Provenance with socket stream protection

    idamServerRedirectStdStreams(0);
    idamProvenancePlugin(&client_block, &request_block, data_source, signal_desc, pluginlist, NULL,
                         getIdamServerEnvironment());
    idamServerRedirectStdStreams(1);

    return 0;
}