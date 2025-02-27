/*---------------------------------------------------------------
 * UDA Legacy Data Server (protocol versions <= 6)
 *
 *---------------------------------------------------------------------------------------------------------------------*/

#include "udaLegacyServer.h"

#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/printStructs.h"
#include "protocol/protocol.h"
#include "protocol/xdr_lib.h"
#include "logging/accessLog.h"
#include "logging/logging.h"
#include "server/serverPlugin.h"
#include "structures/struct.h"
#include "uda/structured.h"
#include "config/config.h"

#include "closeServerSockets.h"
#include "createXDRStream.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "sleepServer.h"

#ifdef LEGACYSERVER
int idamLegacyServer(ClientBlock client_block)
{
    return 0;
}
#else

using namespace uda::client_server;
using namespace uda::server;
using namespace uda::logging;
using namespace uda::structures;

constexpr int ServerVersion = 8;

// Legacy Server Entry point

int uda::server::legacyServer(config::Config& config, ClientBlock client_block, const std::vector<client_server::PluginData>& plugin_list,
                              LogMallocList* log_malloc_list, UserDefinedTypeList* user_defined_type_list,
                              SOCKETLIST* socket_list, int protocolVersion, XDR* server_input, XDR* server_output,
                              unsigned int private_flags, int malloc_source)
{

    int rc, err = 0, depth, fatal = 0;
    ProtocolId protocol_id, next_protocol;

    static unsigned short normal_legacy_wait = 0;
    static unsigned int total_datablock_size = 0;

    SystemConfig system_config;
    DataSystem data_system;
    DataSource data_source;
    Signal signal_rec;
    SignalDesc signal_desc;

    DataBlock data_block;
    RequestBlock request_block;
    ServerBlock server_block;

    Actions actions_desc;
    Actions actions_sig;

    LogStructList log_struct_list;
    init_log_struct_list(&log_struct_list);

    int server_tot_block_time = 0;
    int server_timeout = TimeOut; // user specified Server Lifetime

    uda::server::IoData io_data = {};
    io_data.server_tot_block_time = &server_tot_block_time;
    io_data.server_timeout = &server_timeout;

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    init_error_stack();

    init_server_block(&server_block, ServerVersion);
    init_data_block(&data_block);
    init_actions(&actions_desc); // There may be a Sequence of Actions to Apply
    init_actions(&actions_sig);

    UserDefinedTypeList parseduser_defined_type_list;

    //----------------------------------------------------------------------------
    // Start of Server Wait Loop

    do {
        UDA_LOG(UDA_LOG_DEBUG, "Start of Server Wait Loop")

        //----------------------------------------------------------------------------
        // Start of Error Trap Loop #1

        do {
            UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #1 Loop")

            //----------------------------------------------------------------------------
            // Initialise the Client Structure - only if this is not the first time in the wait loop

            if (normal_legacy_wait) {
                init_client_block(&client_block, 0, "");
            }

            //----------------------------------------------------------------------------
            // Initialise the Request Structure

            init_request_block(&request_block);

            //----------------------------------------------------------------------------
            // Client and Server States
            //
            // Prior to this, client and server state blocks are exchanged. Control is not passed back.
            //
            // Errors: Fatal to Data Access: Return the Error Stack before stopping - at top of error trap #2
            //       Pass Back Server Block and Await Client Instruction

            if (normal_legacy_wait) {
                rc = xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "Receiving Client Block")
                UDA_LOG(UDA_LOG_DEBUG, "XDR #AB xdrrec_eof ? {}", rc)

                protocol_id = ProtocolId::ClientBlock;

                if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                                    &client_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Data Block")
                    add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Receiving Client Block)");
                    concat_error(&server_block.idamerrorstack);
                    close_error();

                    fatal = 1;
                    normal_legacy_wait = 1;
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

            protocolVersion = ServerVersion;
            if (client_block.version < ServerVersion) {
                protocolVersion = client_block.version;
            }

            // The client request may originate from a server.
            // Is the Originating server an externally facing server? If so then switch to this mode: preserve local
            // access policy

            bool external_user = config.get("server.external_user").as_or_default(false);

            if (!external_user && (private_flags & private_flags::External)) {
                config.set("server.external_user", true);
            }

            UDA_LOG(UDA_LOG_DEBUG, "client protocolVersion {}", protocolVersion)
            UDA_LOG(UDA_LOG_DEBUG, "private_flags {}", private_flags)
            UDA_LOG(UDA_LOG_DEBUG, "udaClientFlags  {}", client_flags)
            UDA_LOG(UDA_LOG_DEBUG, "altRank      {}", alt_rank)
            UDA_LOG(UDA_LOG_DEBUG, "external?    {}", external_user)

            if (normal_legacy_wait) {

                protocol_id = ProtocolId::ServerBlock;

                UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block")

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &server_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block")
                    add_error(ErrorType::Code, __func__, err, "Protocol 11 Error (Sending Server Block #1)");
                    concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack
                    close_error();
                    normal_legacy_wait = 1;
                    fatal = 1;
                }

                if (fatal) {
                    if (server_block.idamerrorstack.nerrors > 0) {
                        err = server_block.error_stack[0].code;
                    } else {
                        err = 1;
                    }
                    break; // Manage the Fatal Server State
                }
            }

            normal_legacy_wait = 1; // Enable client & server state block legacy exchange

            //-------------------------------------------------------------------------
            // Client Request
            //
            // Errors: Fatal to Data Access
            //       Pass Back and Await Client Instruction

            protocol_id = ProtocolId::RequestBlock;

            if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                                &request_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Request Block")
                add_error(ErrorType::Code, __func__, err, "Protocol 1 Error (Receiving Client Request)");
                break;
            }

            rc = xdrrec_eof(server_input);
            UDA_LOG(UDA_LOG_DEBUG, "Request Block Received")
            UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? {}", rc)

            print_client_block(client_block);
            print_server_block(server_block);
            print_request_block(request_block);

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

            char work[StringLength];

            if (request_block.api_delim[0] != '\0') {
                sprintf(work, "UDA%s", request_block.api_delim);
            } else {
                sprintf(work, "UDA%s", environment->api_delim);
            }

            if (environment->server_proxy[0] != '\0' && strncasecmp(request_block.source, work, strlen(work)) != 0) {

                // Check the Server Version is Compatible with the Originating client version ?

                if (client_block.version < 6) {
                    err = 999;
                    add_error(ErrorType::Code, __func__, err,
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
                        ErrorType::Code, __func__, err,
                        "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                    break;
                }

                // Check string length compatibility

                if (strlen(request_block.source) >=
                    (StringLength - 1 - strlen(environment->server_proxy) - 4 + strlen(request_block.api_delim))) {
                    err = 999;
                    add_error(ErrorType::Code, __func__, err,
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
                    UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to {}", environment->server_proxy);
                    UDA_LOG(UDA_LOG_DEBUG, "source: {}", request_block.source);
                    // UDA_LOG(UDA_LOG_DEBUG, "server: {}", request_block.server);
                }
            }

#  endif

            //----------------------------------------------------------------------
            // Write to the Access Log

            uda_access_log(TRUE, client_block, request_block, server_block, total_datablock_size);

            //----------------------------------------------------------------------
            // Initialise Data Structures

            init_data_source(&data_source);
            init_signal_desc(&signal_desc);
            init_signal(&signal_rec);

            //----------------------------------------------------------------------------------------------
            // If this is a PUT request then receive the putData structure

            RequestData* request_data = &request_block.requests[0];

            init_put_data_block_list(&(request_data->putDataBlockList));

            if (request_data->put) {

                protocol_id = ProtocolId::PutdataBlockList;

                if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, nullptr, log_malloc_list, user_defined_type_list,
                                    &(request_data->putDataBlockList), protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving putData Block List")
                    add_error(ErrorType::Code, __func__, err,
                              "Protocol 1 Error (Receiving Client putDataBlockList)");
                    break;
                }

                rc = (int)xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received")
                UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", request_data->putDataBlockList.blockCount)
                UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? {}", rc)
            }

            //----------------------------------------------------------------------------------------------
            // Decode the API Arguments: determine appropriate data plug-in to use
            // Decide on Authentication procedure

            for (int i = 0; i < request_block.num_requests; ++i) {
                auto request = &request_block.requests[i];
                if (protocolVersion >= 6) {
                    if ((err = uda::server::udaServerPlugin(config, request, &data_source, &signal_desc, plugin_list)) != 0) {
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
                err = get_data(config, &depth, request, client_block, &data_block, &data_source, &signal_rec, &signal_desc,
                               &actions_desc, &actions_sig, plugin_list, log_malloc_list, user_defined_type_list, socket_list,
                               protocolVersion);
            }

            UDA_LOG(UDA_LOG_DEBUG,
                    "======================== ******************** ==========================================\n")
            UDA_LOG(UDA_LOG_DEBUG, "Archive      : {} ", data_source.archive)
            UDA_LOG(UDA_LOG_DEBUG, "Device Name  : {} ", data_source.device_name)
            UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : {} ", signal_desc.signal_name)
            UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", data_source.path)
            UDA_LOG(UDA_LOG_DEBUG, "File Name    : {} ", data_source.filename)
            UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", data_source.exp_number)
            UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", data_source.pass)
            UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : {} ", depth)
            print_request_block(request_block);
            print_data_source(data_source);
            print_signal(signal_rec);
            print_signal_desc(signal_desc);
            print_data_block(data_block);
            print_error_stack();
            UDA_LOG(UDA_LOG_DEBUG,
                    "======================== ******************** ==========================================\n")

            if (err != 0) {
                break;
            }

            //------------------------------------------------------------------------------------------------
            // Server-Side Data Processing

            if (client_block.get_dimdble || client_block.get_timedble || client_block.get_scalar) {
                if (serverProcessing(client_block, &data_block) != 0) {
                    err = 779;
                    add_error(ErrorType::Code, __func__, err, "Server-Side Processing Error");
                    break;
                }
            }

            //----------------------------------------------------------------------------
            // Check the Client can receive the data type: Version dependent
            // Otherwise inform the client via the server state block

            if (protocolVersion < 6 && data_block.data_type == UDA_TYPE_STRING) {
                data_block.data_type = UDA_TYPE_CHAR;
            }

            if (data_block.data_n > 0 && (protocol_version_type_test(protocolVersion, data_block.data_type) ||
                                          protocol_version_type_test(protocolVersion, data_block.error_type))) {
                err = 999;
                add_error(ErrorType::Code, __func__, err,
                          "The Data has a type that cannot be passed to the Client: A newer client library version "
                          "is required.");
                break;
            }

            if (data_block.rank > 0) {
                Dims dim;
                for (unsigned int i = 0; i < data_block.rank; i++) {
                    dim = data_block.dims[i];
                    if (protocol_version_type_test(protocolVersion, dim.data_type) ||
                        protocol_version_type_test(protocolVersion, dim.error_type)) {
                        err = 999;
                        add_error(ErrorType::Code, __func__, err,
                                  "A Coordinate Data has a numerical type that cannot be passed to the Client: A "
                                  "newer client library version is required.");
                        break;
                    }
                }
            }

            //----------------------------------------------------------------------------
            // End of Error Trap #1

        } while (0);

        UDA_LOG(UDA_LOG_DEBUG, "Leaving Error Trap #1 Loop: {}", err)

        //----------------------------------------------------------------------------
        // Start of Error Trap Loop #2

        do {
            UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #2 Loop")

            //----------------------------------------------------------------------------
            // Send Server Error State

            concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack
            close_error();

            print_server_block(server_block);

            if (server_block.idamerrorstack.nerrors > 0) {
                server_block.error = server_block.error_stack[0].code;
                strcpy(server_block.msg, server_block.error_stack[0].msg);
            }

            protocol_id = ProtocolId::ServerBlock;

            if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                &server_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block #2")
                add_error(ErrorType::Code, __func__, err, "Protocol 11 Error (Sending Server Block #2)");
                break;
            }

            if (server_block.idamerrorstack.nerrors > 0) {
                err = server_block.error_stack[0].code;
            } else {
                err = 0;
            }

            if (err != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop")
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Server Block Sent to Client")

            //----------------------------------------------------------------------------
            // Return Database Metadata if User Requests it

            if (client_block.get_meta) {

                // Next Protocol id

                protocol_id = ProtocolId::NextProtocol;

                if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, &next_protocol, log_malloc_list,
                                    user_defined_type_list, nullptr, protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #1 Receiving Next Protocol ID")
                    add_error(ErrorType::Code, __func__, err, "Protocol 3 (Next Protocol #1) Error");
                    break;
                }

                rc = xdrrec_eof(server_input);
                UDA_LOG(UDA_LOG_DEBUG, "Next Protocol {} Received", next_protocol)
                UDA_LOG(UDA_LOG_DEBUG, "XDR #D xdrrec_eof ? {}", rc)

                if (next_protocol != ProtocolId::DataSystem) {
                    err = 998;
                    add_error(ErrorType::Code, __func__, err, "Protocol 3 Error: Protocol Request Inconsistency");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data System Structure

                protocol_id = ProtocolId::DataSystem;

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &data_system, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure")
                    add_error(ErrorType::Code, __func__, err, "Protocol 4 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the System Configuration Structure

                protocol_id = ProtocolId::SystemConfig;

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &system_config, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure")
                    add_error(ErrorType::Code, __func__, err, "Protocol 5 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data Source Structure

                protocol_id = ProtocolId::DataSource;

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &data_source, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure")
                    add_error(ErrorType::Code, __func__, err, "Protocol 6 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Structure

                protocol_id = ProtocolId::Signal;

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &signal_rec, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure")
                    add_error(ErrorType::Code, __func__, err, "Protocol 7 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Description Structure

                protocol_id = ProtocolId::SignalDesc;

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &signal_desc, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure")
                    add_error(ErrorType::Code, __func__, err, "Protocol 8 Error");
                    break;
                }

            } // End of Database Meta Data

            //----------------------------------------------------------------------------
            // Next Protocol id

            protocol_id = ProtocolId::NextProtocol;

            if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, &next_protocol, log_malloc_list,
                                user_defined_type_list, nullptr, protocolVersion, &log_struct_list, &io_data,
                                private_flags, malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem #2 Receiving Next Protocol ID")
                add_error(ErrorType::Code, __func__, err, "Protocol 3 (Next Protocol #2) Error");
                break;
            }

            rc = xdrrec_eof(server_input);
            UDA_LOG(UDA_LOG_DEBUG, "Next Protocol {} Received", next_protocol)
            UDA_LOG(UDA_LOG_DEBUG, "XDR #E xdrrec_eof ? {}", rc)

            //----------------------------------------------------------------------------
            // Send the Data

            if (next_protocol != ProtocolId::DataBlockList) {
                err = 997;
                add_error(ErrorType::Code, __func__, err, "Protocol 3 Error: Incorrect Request");
                break;
            }

            print_data_block(data_block);
            UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client")

            protocol_id = ProtocolId::DataBlockList;

            if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                &data_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                malloc_source)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure")
                add_error(ErrorType::Code, __func__, err, "Protocol 2 Error");
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client")

            //------------------------------------------------------------------------------
            // Clear Output Buffer (check - it should be empty!) and receive Next Protocol

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

                protocol_id = ProtocolId::NextProtocol;

                if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, &next_protocol, log_malloc_list,
                                    user_defined_type_list, nullptr, protocolVersion, &log_struct_list, &io_data,
                                    private_flags, malloc_source)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #2a Receiving Next Protocol ID")
                    add_error(ErrorType::Code, __func__, err, "Protocol 3 (Next Protocol #2) Error");
                    break;
                }

                if (next_protocol != ProtocolId::Structures) {
                    err = 999;
                    add_error(ErrorType::Code, __func__, err, "Incorrect Next Protocol received: (Structures)");
                    break;
                }
            }

            //------------------------------------------------------------------------------
            // Hierarchical Data Structures

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {
                if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
                    protocol_id = ProtocolId::Meta;
                } else if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                    data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                    protocol_id = ProtocolId::Structures;
                }

                UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client")

                if ((err = protocol(server_output, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list, user_defined_type_list,
                                    &data_block, protocolVersion, &log_struct_list, &io_data, private_flags,
                                    malloc_source)) != 0) {
                    add_error(ErrorType::Code, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
                    break;
                }

                UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client")
            }

            //----------------------------------------------------------------------------
            // End of Error Trap #2

        } while (0);

        UDA_LOG(UDA_LOG_DEBUG, "Leaving Error Trap #2 Loop: {}", err)

        //----------------------------------------------------------------------
        // Complete & Write the Access Log Record

        uda_access_log(0, client_block, request_block, server_block, total_datablock_size);

        //----------------------------------------------------------------------------
        // Server Shutdown ? Next Instruction from Client
        //
        // Protocols:   13 => Die
        //        14 => Sleep
        //        15 => Wakeup

        // <========================== Client Server Code Only

        protocol_id = ProtocolId::NextProtocol;
        next_protocol = ProtocolId::Start;

        if ((err = protocol(server_input, protocol_id, XDRStreamDirection::Receive, &next_protocol, log_malloc_list, user_defined_type_list,
                            nullptr, protocolVersion, &log_struct_list, &io_data, private_flags, malloc_source)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem #3 Receiving Next Protocol ID")
            add_error(ErrorType::Code, __func__, err, "Protocol 3 (Server Shutdown) Error");
            break;
        }

        rc = xdrrec_eof(server_input);
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol {} Received", next_protocol)
        UDA_LOG(UDA_LOG_DEBUG, "XDR #F xdrrec_eof ? {}", rc)
        UDA_LOG(UDA_LOG_DEBUG, "Current Error Value {}", err)

        UDA_LOG(UDA_LOG_DEBUG, "Client Request {}", next_protocol)
        if (next_protocol == ProtocolId::CloseDown) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Die")
        }
        if (next_protocol == ProtocolId::Sleep) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Sleep")
        }
        if (next_protocol == ProtocolId::WakeUp) {
            UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Wake-up")
        }

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "free_data_block")
        free_data_block(&data_block);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions")
        free_actions(&actions_desc);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions")
        free_actions(&actions_sig);

        UDA_LOG(UDA_LOG_DEBUG, "free_request_block")
        free_request_block(&request_block);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concat_error")
        concat_error(&server_block.idamerrorstack); // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "close_error")
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "error_log")
        error_log(client_block, request_block, &server_block.idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "close_error")
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock")
        init_server_block(&server_block, ServerVersion);

        UDA_LOG(UDA_LOG_DEBUG, "At End of Error Trap")

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == ProtocolId::Sleep &&
             sleepServer(server_input, server_output, log_malloc_list, user_defined_type_list, protocolVersion,
                         &log_struct_list, server_tot_block_time, server_timeout, &io_data, private_flags,
                         malloc_source));

    //----------------------------------------------------------------------------
    // Server Destruct.....

    UDA_LOG(UDA_LOG_DEBUG, "Server Shutting Down")

    if (server_tot_block_time > 1000 * server_timeout) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Timeout after {} secs", server_timeout)
    }

    //----------------------------------------------------------------------------
    // Free Data Block Heap Memory in case by-passed

    free_data_block(&data_block);

    //----------------------------------------------------------------------------
    // Free Structure Definition List (don't free the structure as stack variable)

    udaFreeUserDefinedTypeList(&parseduser_defined_type_list);

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

    close_logging();

    return 0;
}

#endif
