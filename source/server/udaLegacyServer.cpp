/*---------------------------------------------------------------
 * UDA Legacy Data Server (protocol versions <= 6)
 *
 *---------------------------------------------------------------------------------------------------------------------*/

#include "udaLegacyServer.h"

#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"
#include "clientserver/printStructs.h"
#include "clientserver/protocol.h"
#include "clientserver/xdrlib.h"
#include "logging/accessLog.h"
#include "logging/logging.h"
#include "server/serverPlugin.h"
#include "structures/struct.h"
#include "uda/structured.h"

#include "closeServerSockets.h"
#include "createXDRStream.h"
#include "getServerEnvironment.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "sleepServer.h"

#ifdef LEGACYSERVER
int idamLegacyServer(CLIENT_BLOCK client_block)
{
    return 0;
}
#else

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;

constexpr int server_version = 8;

// Legacy Server Entry point

int uda::server::legacyServer(CLIENT_BLOCK client_block, const uda::plugins::PluginList* pluginlist,
                              LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                              SOCKETLIST* socket_list, int protocolVersion, XDR* server_input, XDR* server_output,
                              unsigned int private_flags, int malloc_source)
{

    int rc, err = 0, depth, fatal = 0;
    int protocol_id, next_protocol;

    static unsigned short normalLegacyWait = 0;
    static unsigned int total_datablock_size = 0;

    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
    DATA_SOURCE data_source;
    SIGNAL signal_rec;
    SIGNAL_DESC signal_desc;

    DATA_BLOCK data_block;
    REQUEST_BLOCK request_block;
    SERVER_BLOCK server_block;

    ACTIONS actions_desc;
    ACTIONS actions_sig;

    LOGSTRUCTLIST log_struct_list;
    initLogStructList(&log_struct_list);

    int server_tot_block_time = 0;
    int server_timeout = TIMEOUT; // user specified Server Lifetime

    uda::server::IoData io_data = {};
    io_data.server_tot_block_time = &server_tot_block_time;
    io_data.server_timeout = &server_timeout;

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    init_error_stack();

    initServerBlock(&server_block, server_version);
    initDataBlock(&data_block);
    initActions(&actions_desc); // There may be a Sequence of Actions to Apply
    initActions(&actions_sig);

    USERDEFINEDTYPELIST parseduserdefinedtypelist;

    //----------------------------------------------------------------------------
    // Start of Server Wait Loop

    do {
        UDA_LOG(UDA_LOG_DEBUG, "Start of Server Wait Loop\n");

        //----------------------------------------------------------------------------
        // Start of Error Trap Loop #1

        do {
            UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #1 Loop\n");

            //----------------------------------------------------------------------------
            // Initialise the Client Structure - only if this is not the first time in the wait loop

            if (normalLegacyWait) {
                initClientBlock(&client_block, 0, "");
            }

            //----------------------------------------------------------------------------
            // Initialise the Request Structure

            initRequestBlock(&request_block);

            //----------------------------------------------------------------------------
            // Client and Server States
            //
            // Prior to this, client and server state blocks are exchanged. Control is not passed back.
            //
            // Errors: Fatal to Data Access: Return the Error Stack before stopping - at top of error trap #2
            //       Pass Back Server Block and Await Client Instruction

            if (normalLegacyWait) {
                rc = xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "Receiving Client Block\n");
                UDA_LOG(UDA_LOG_DEBUG, "XDR #AB xdrrec_eof ? %d\n", rc);

                protocol_id = UDA_PROTOCOL_CLIENT_BLOCK;

                if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                    &client_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Data Block\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 10 Error (Receiving Client Block)");
                    concat_error(&server_block.idamerrorstack);
                    close_error();

                    fatal = 1;
                    normalLegacyWait = 1;
                    break;
                }
            }

            server_timeout = client_block.timeout;            // User specified Server Lifetime
            private_flags = client_block.privateFlags;        // Server to Server flags
            uint32_t client_flags = client_block.clientFlags; // Client set flags
            int alt_rank = client_block.altRank;              // Rank of Alternative source

            // Protocol Version: Lower of the client and server version numbers
            // This defines the set of elements within data structures passed between client and server
            // Must be the same on both sides of the socket

            protocolVersion = server_version;
            if (client_block.version < server_version) {
                protocolVersion = client_block.version;
            }

            // The client request may originate from a server.
            // Is the Originating server an externally facing server? If so then switch to this mode: preserve local
            // access policy

            ENVIRONMENT* environment = getServerEnvironment();

            if (!environment->external_user && (private_flags & PRIVATEFLAG_EXTERNAL)) {
                environment->external_user = 1;
            }

            UDA_LOG(UDA_LOG_DEBUG, "client protocolVersion %d\n", protocolVersion);
            UDA_LOG(UDA_LOG_DEBUG, "private_flags %d\n", private_flags);
            UDA_LOG(UDA_LOG_DEBUG, "udaClientFlags  %d\n", client_flags);
            UDA_LOG(UDA_LOG_DEBUG, "altRank      %d\n", alt_rank);
            UDA_LOG(UDA_LOG_DEBUG, "external?    %d\n", environment->external_user);

            if (normalLegacyWait) {

                protocol_id = UDA_PROTOCOL_SERVER_BLOCK;

                UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block\n");

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &server_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 11 Error (Sending Server Block #1)");
                    concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack
                    close_error();
                    normalLegacyWait = 1;
                    fatal = 1;
                }

                if (fatal) {
                    if (server_block.idamerrorstack.nerrors > 0) {
                        err = server_block.idamerrorstack.idamerror[0].code;
                    } else {
                        err = 1;
                    }
                    break; // Manage the Fatal Server State
                }
            }

            normalLegacyWait = 1; // Enable client & server state block legacy exchange

            //-------------------------------------------------------------------------
            // Client Request
            //
            // Errors: Fatal to Data Access
            //       Pass Back and Await Client Instruction

            protocol_id = UDA_PROTOCOL_REQUEST_BLOCK;

            if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                &request_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Request Block\n");
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 1 Error (Receiving Client Request)");
                break;
            }

            rc = xdrrec_eof(server_input);
            UDA_LOG(UDA_LOG_DEBUG, "Request Block Received\n");
            UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? %d\n", rc);

            printClientBlock(client_block);
            printServerBlock(server_block);
            printRequestBlock(request_block);

            //------------------------------------------------------------------------------------------------------------------
            // Prepend Proxy Host to Source to redirect client request

            /*! On parallel clusters where nodes are connected together on a private network, only the master node may
            have access to external data sources. Cluster nodes can access these external sources via an UDA server
            running on the master node. This server acts as a proxy server. It simply redirects requests to other
            external UDA servers. To facilitate this redirection, each access request source string must be prepended
            with "UDA::host:port/" within the server. The host:port component is defined by the system administrator via
            an environment variable "UDA_PROXY". Client's don't need to specifiy redirection via a Proxy - it's
            automatic if the UDA_PROXY environment variable is defined. No prepending is done if the source is already a
            redirection, i.e. it begins "UDA::".
            */

#  ifdef PROXYSERVER

            char work[STRING_LENGTH];

            if (request_block.api_delim[0] != '\0') {
                sprintf(work, "UDA%s", request_block.api_delim);
            } else {
                sprintf(work, "UDA%s", environment->api_delim);
            }

            if (environment->server_proxy[0] != '\0' && strncasecmp(request_block.source, work, strlen(work)) != 0) {

                // Check the Server Version is Compatible with the Originating client version ?

                if (client_block.version < 6) {
                    err = 999;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                              "PROXY redirection: Originating Client Version not compatible with the PROXY server "
                              "interface.");
                    break;
                }

                // Test for Proxy calling itself indirectly => potential infinite loop
                // The UDA Plugin strips out the host and port data from the source so the originating server details
                // are never passed.

                if (request_block.api_delim[0] != '\0') {
                    sprintf(work, "UDA%s%s", request_block.api_delim, environment->server_this);
                } else {
                    sprintf(work, "UDA%s%s", environment->api_delim, environment->server_this);
                }

                if (strstr(request_block.source, work) != nullptr) {
                    err = 999;
                    add_error(
                        UDA_CODE_ERROR_TYPE, __func__, err,
                        "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                    break;
                }

                // Check string length compatibility

                if (strlen(request_block.source) >=
                    (STRING_LENGTH - 1 - strlen(environment->server_proxy) - 4 + strlen(request_block.api_delim))) {
                    err = 999;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                              "PROXY redirection: The source argument string is too long!");
                    break;
                }

                // Prepend the redirection UDA server details

                if (request_block.api_delim[0] != '\0') {
                    sprintf(work, "UDA%s%s/%s", request_block.api_delim, environment->server_proxy,
                            request_block.source);
                } else {
                    sprintf(work, "UDA%s%s/%s", environment->api_delim, environment->server_proxy,
                            request_block.source);
                }

                strcpy(request_block.source, work);
                // strcpy(request_block.server, environment->server_proxy);

                if (debugon) {
                    UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s\n", environment->server_proxy);
                    UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request_block.source);
                    // UDA_LOG(UDA_LOG_DEBUG, "server: %s\n", request_block.server);
                }
            }

#  endif

            //----------------------------------------------------------------------
            // Write to the Access Log

            udaAccessLog(TRUE, client_block, request_block, server_block, total_datablock_size);

            //----------------------------------------------------------------------
            // Initialise Data Structures

            initDataSource(&data_source);
            initSignalDesc(&signal_desc);
            initSignal(&signal_rec);

            //----------------------------------------------------------------------------------------------
            // If this is a PUT request then receive the putData structure

            REQUEST_DATA* request_data = &request_block.requests[0];

            initPutDataBlockList(&(request_data->putDataBlockList));

            if (request_data->put) {

                protocol_id = UDA_PROTOCOL_PUTDATA_BLOCK_LIST;

                if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                    &(request_data->putDataBlockList), protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving putData Block List\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                              "Protocol 1 Error (Receiving Client putDataBlockList)");
                    break;
                }

                rc = (int)xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received\n");
                UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", request_data->putDataBlockList.blockCount);
                UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? %d\n", rc);
            }

            //----------------------------------------------------------------------------------------------
            // Decode the API Arguments: determine appropriate data plug-in to use
            // Decide on Authentication procedure

            for (int i = 0; i < request_block.num_requests; ++i) {
                auto request = &request_block.requests[i];
                if (protocolVersion >= 6) {
                    if ((err = uda::server::udaServerPlugin(request, &data_source, &signal_desc, pluginlist,
                                                            getServerEnvironment())) != 0) {
                        break;
                    }
                } else {
                    if ((err = udaServerLegacyPlugin(request, &data_source, &signal_desc)) != 0) {
                        break;
                    }
                }
            }

            //------------------------------------------------------------------------------------------------
            // Query the Database: Internal or External Data Sources
            // Read the Data or Create the Composite/Derived Data
            // Apply XML Actions to Data

            depth = 0;

            for (int i = 0; i < request_block.num_requests; ++i) {
                auto request = &request_block.requests[i];
                err = udaGetData(&depth, request, client_block, &data_block, &data_source, &signal_rec, &signal_desc,
                                 &actions_desc, &actions_sig, pluginlist, logmalloclist, userdefinedtypelist,
                                 socket_list, protocolVersion);
            }

            UDA_LOG(UDA_LOG_DEBUG,
                    "======================== ******************** ==========================================\n");
            UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source.archive);
            UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source.device_name);
            UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc.signal_name);
            UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source.path);
            UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source.filename);
            UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source.exp_number);
            UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source.pass);
            UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : %d \n", depth);
            printRequestBlock(request_block);
            printDataSource(data_source);
            printSignal(signal_rec);
            printSignalDesc(signal_desc);
            printDataBlock(data_block);
            print_error_stack();
            UDA_LOG(UDA_LOG_DEBUG,
                    "======================== ******************** ==========================================\n");

            if (err != 0) {
                break;
            }

            //------------------------------------------------------------------------------------------------
            // Server-Side Data Processing

            if (client_block.get_dimdble || client_block.get_timedble || client_block.get_scalar) {
                if (serverProcessing(client_block, &data_block) != 0) {
                    err = 779;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Server-Side Processing Error");
                    break;
                }
            }

            //----------------------------------------------------------------------------
            // Check the Client can receive the data type: Version dependent
            // Otherwise inform the client via the server state block

            if (protocolVersion < 6 && data_block.data_type == UDA_TYPE_STRING) {
                data_block.data_type = UDA_TYPE_CHAR;
            }

            if (data_block.data_n > 0 && (protocolVersionTypeTest(protocolVersion, data_block.data_type) ||
                                          protocolVersionTypeTest(protocolVersion, data_block.error_type))) {
                err = 999;
                add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                          "The Data has a type that cannot be passed to the Client: A newer client library version "
                          "is required.");
                break;
            }

            if (data_block.rank > 0) {
                DIMS dim;
                for (unsigned int i = 0; i < data_block.rank; i++) {
                    dim = data_block.dims[i];
                    if (protocolVersionTypeTest(protocolVersion, dim.data_type) ||
                        protocolVersionTypeTest(protocolVersion, dim.error_type)) {
                        err = 999;
                        add_error(UDA_CODE_ERROR_TYPE, __func__, err,
                                  "A Coordinate Data has a numerical type that cannot be passed to the Client: A "
                                  "newer client library version is required.");
                        break;
                    }
                }
            }

            //----------------------------------------------------------------------------
            // End of Error Trap #1

        } while (0);

        UDA_LOG(UDA_LOG_DEBUG, "Leaving Error Trap #1 Loop: %d\n", err);

        //----------------------------------------------------------------------------
        // Start of Error Trap Loop #2

        do {
            UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #2 Loop\n");

            //----------------------------------------------------------------------------
            // Send Server Error State

            concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack
            close_error();

            printServerBlock(server_block);

            if (server_block.idamerrorstack.nerrors > 0) {
                server_block.error = server_block.idamerrorstack.idamerror[0].code;
                strcpy(server_block.msg, server_block.idamerrorstack.idamerror[0].msg);
            }

            protocol_id = UDA_PROTOCOL_SERVER_BLOCK;

            if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                &server_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block #2\n");
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 11 Error (Sending Server Block #2)");
                break;
            }

            if (server_block.idamerrorstack.nerrors > 0) {
                err = server_block.idamerrorstack.idamerror[0].code;
            } else {
                err = 0;
            }

            if (err != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop\n");
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Server Block Sent to Client\n");

            //----------------------------------------------------------------------------
            // Return Database Meta Data if User Requests it

            if (client_block.get_meta) {

                // Next Protocol id

                protocol_id = UDA_PROTOCOL_NEXT_PROTOCOL;

                if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                    userdefinedtypelist, nullptr, protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #1 Receiving Next Protocol ID\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 (Next Protocol #1) Error");
                    break;
                }

                rc = xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
                UDA_LOG(UDA_LOG_DEBUG, "XDR #D xdrrec_eof ? %d\n", rc);

                if (next_protocol != UDA_PROTOCOL_DATA_SYSTEM) {
                    err = 998;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 Error: Protocol Request Inconsistency");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data System Structure

                protocol_id = UDA_PROTOCOL_DATA_SYSTEM;

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_system, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 4 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the System Configuration Structure

                protocol_id = UDA_PROTOCOL_SYSTEM_CONFIG;

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &system_config, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 5 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data Source Structure

                protocol_id = UDA_PROTOCOL_DATA_SOURCE;

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_source, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 6 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Structure

                protocol_id = UDA_PROTOCOL_SIGNAL;

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &signal_rec, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 7 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Description Structure

                protocol_id = UDA_PROTOCOL_SIGNAL_DESC;

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &signal_desc, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 8 Error");
                    break;
                }

            } // End of Database Meta Data

            //----------------------------------------------------------------------------
            // Next Protocol id

            protocol_id = UDA_PROTOCOL_NEXT_PROTOCOL;

            if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                userdefinedtypelist, nullptr, protocolVersion, &log_struct_list, &io_data,
                                private_flags, malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem #2 Receiving Next Protocol ID\n");
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 (Next Protocol #2) Error");
                break;
            }

            rc = xdrrec_eof(server_input);
            UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
            UDA_LOG(UDA_LOG_DEBUG, "XDR #E xdrrec_eof ? %d\n", rc);

            //----------------------------------------------------------------------------
            // Send the Data

            if (next_protocol != UDA_PROTOCOL_DATA_BLOCK_LIST) {
                err = 997;
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 Error: Incorrect Request");
                break;
            }

            printDataBlock(data_block);
            UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client\n");

            protocol_id = UDA_PROTOCOL_DATA_BLOCK_LIST;

            if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                &data_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure\n");
                add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 2 Error");
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client\n");

            //------------------------------------------------------------------------------
            // Clear Output Buffer (check - it should be empty!) and receive Next Protocol

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

                protocol_id = UDA_PROTOCOL_NEXT_PROTOCOL;

                if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                    userdefinedtypelist, nullptr, protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #2a Receiving Next Protocol ID\n");
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 (Next Protocol #2) Error");
                    break;
                }

                if (next_protocol != UDA_PROTOCOL_STRUCTURES) {
                    err = 999;
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Incorrect Next Protocol received: (Structures)");
                    break;
                }
            }

            //------------------------------------------------------------------------------
            // Hierarchical Data Structures

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {
                if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
                    protocol_id = UDA_PROTOCOL_META;
                } else {
                    if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                        data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                        protocol_id = UDA_PROTOCOL_STRUCTURES;
                    } else {
                        protocol_id = UDA_PROTOCOL_EFIT;
                    }
                }

                UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client\n");

                if ((err = protocol(server_output, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
                    break;
                }

                UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client\n");
            }

            //----------------------------------------------------------------------------
            // End of Error Trap #2

        } while (0);

        UDA_LOG(UDA_LOG_DEBUG, "Leaving Error Trap #2 Loop: %d\n", err);

        //----------------------------------------------------------------------
        // Complete & Write the Access Log Record

        udaAccessLog(0, client_block, request_block, server_block, total_datablock_size);

        //----------------------------------------------------------------------------
        // Server Shutdown ? Next Instruction from Client
        //
        // Protocols:   13 => Die
        //        14 => Sleep
        //        15 => Wakeup

        // <========================== Client Server Code Only

        protocol_id = UDA_PROTOCOL_NEXT_PROTOCOL;
        next_protocol = 0;

        if ((err = protocol(server_input, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist, userdefinedtypelist,
                            nullptr, protocolVersion, &log_struct_list, &io_data, private_flags, malloc_source)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem #3 Receiving Next Protocol ID\n");
            add_error(UDA_CODE_ERROR_TYPE, __func__, err, "Protocol 3 (Server Shutdown) Error");
            break;
        }

        rc = xdrrec_eof(server_input);
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
        UDA_LOG(UDA_LOG_DEBUG, "XDR #F xdrrec_eof ? %d\n", rc);
        UDA_LOG(UDA_LOG_DEBUG, "Current Error Value %d\n", err);

        UDA_LOG(UDA_LOG_DEBUG, "Client Request %d\n", next_protocol);
        if (next_protocol == UDA_PROTOCOL_CLOSEDOWN) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Die\n");
        }
        if (next_protocol == UDA_PROTOCOL_SLEEP) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Sleep\n");
        }
        if (next_protocol == UDA_PROTOCOL_WAKE_UP) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Wake-up\n");
        }

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock\n");
        freeDataBlock(&data_block);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_desc);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_sig);

        UDA_LOG(UDA_LOG_DEBUG, "freeRequestBlock\n");
        freeRequestBlock(&request_block);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concat_error\n");
        concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "close_error\n");
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "error_log\n");
        error_log(client_block, request_block, &server_block.idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "close_error\n");
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock\n");
        initServerBlock(&server_block, server_version);

        UDA_LOG(UDA_LOG_DEBUG, "At End of Error Trap\n");

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == UDA_PROTOCOL_SLEEP &&
             sleepServer(server_input, server_output, logmalloclist, userdefinedtypelist, protocolVersion,
                         &log_struct_list, server_tot_block_time, server_timeout, &io_data, private_flags,
                         malloc_source));

    //----------------------------------------------------------------------------
    // Server Destruct.....

    UDA_LOG(UDA_LOG_DEBUG, "Server Shuting Down\n");

    if (server_tot_block_time > 1000 * server_timeout) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Timeout after %d secs\n", server_timeout);
    }

    //----------------------------------------------------------------------------
    // Free Data Block Heap Memory in case by-passed

    freeDataBlock(&data_block);

    //----------------------------------------------------------------------------
    // Free Structure Definition List (don't free the structure as stack variable)

    udaFreeUserDefinedTypeList(&parseduserdefinedtypelist);

    //----------------------------------------------------------------------------
    // Close the Socket Connections to Other Data Servers

    closeServerSockets(socket_list);

    //----------------------------------------------------------------------------
    // Write the Error Log Record & Free Error Stack Heap

    error_log(client_block, request_block, nullptr);
    close_error();

    //----------------------------------------------------------------------------
    // Close the Logs

    fflush(nullptr);

    udaCloseLogging();

    return 0;
}

#endif
