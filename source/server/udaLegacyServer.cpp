/*---------------------------------------------------------------
* IDAM Legacy Data Server (protocol versions <= 6)
*
*---------------------------------------------------------------------------------------------------------------------*/

#include "udaLegacyServer.h"

#include <clientserver/errorLog.h>
#include <clientserver/freeDataBlock.h>
#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>
#include <clientserver/udaTypes.h>
#include <clientserver/xdrlib.h>
#include <logging/accessLog.h>
#include <logging/logging.h>
#include <server/serverPlugin.h>
#include <structures/struct.h>

#include "closeServerSockets.h"
#include "freeIdamPut.h"
#include "getServerEnvironment.h"
#include "makeServerRequestBlock.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "sleepServer.h"
#include "udaServer.h"

#ifdef LEGACYSERVER
int idamLegacyServer(CLIENT_BLOCK client_block) {
    return 0;
}
#else

// Legacy Server Entry point

int idamLegacyServer(CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                     USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion)
{

    int rc, err = 0, depth, fatal = 0;
    int protocol_id, next_protocol;

    static unsigned short normalLegacyWait = 0;

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

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    initIdamErrorStack();

    initServerBlock(&server_block, serverVersion);
    initDataBlock(&data_block);
    initActions(&actions_desc);        // There may be a Sequence of Actions to Apply
    initActions(&actions_sig);

    //----------------------------------------------------------------------------
    // Start of Server Wait Loop

    do {
        UDA_LOG(UDA_LOG_DEBUG, "IdamLegacyServer: Start of Server Wait Loop\n");

        //----------------------------------------------------------------------------
        // Start of Error Trap Loop #1

        do {
            UDA_LOG(UDA_LOG_DEBUG, "IdamLegacyServer: Start of Server Error Trap #1 Loop\n");

            //----------------------------------------------------------------------------
            // Initialise the Client Structure - only if this is not the first time in the wait loop

            if (normalLegacyWait) initClientBlock(&client_block, 0, "");

            //----------------------------------------------------------------------------
            // Initialise the Request Structure

            initRequestBlock(&request_block);

            //----------------------------------------------------------------------------
            // Client and Server States
            //
            // Prior to this, client and server state blocks are exchanged. Control is not passed back.
            //
            // Errors: Fatal to Data Access: Return the Error Stack before stopping - at top of error trap #2
            //	   Pass Back Server Block and Await Client Instruction

            if (normalLegacyWait) {
                rc = xdrrec_eof(serverInput);
                UDA_LOG(UDA_LOG_DEBUG, "Receiving Client Block\n");
                UDA_LOG(UDA_LOG_DEBUG, "XDR #AB xdrrec_eof ? %d\n", rc);

                protocol_id = PROTOCOL_CLIENT_BLOCK;

                if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                    &client_block, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "IdamServer: Problem Receiving Client Data Block\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 10 Error (Receiving Client Block)");
                    concatIdamError(&server_block.idamerrorstack);
                    closeIdamError();

                    fatal = 1;
                    normalLegacyWait = 1;
                    break;
                }
            }

            server_timeout = client_block.timeout;        // User specified Server Lifetime
            privateFlags = client_block.privateFlags;    // Server to Server flags
            clientFlags = client_block.clientFlags;    // Client set flags
            altRank = client_block.altRank;            // Rank of Alternative source

            // Protocol Version: Lower of the client and server version numbers
            // This defines the set of elements within data structures passed between client and server
            // Must be the same on both sides of the socket

            protocolVersion = serverVersion;
            if (client_block.version < serverVersion) protocolVersion = client_block.version;

            // The client request may originate from a server.
            // Is the Originating server an externally facing server? If so then switch to this mode: preserve local access policy

            ENVIRONMENT* environment = getIdamServerEnvironment();

            if (!environment->external_user && (privateFlags & PRIVATEFLAG_EXTERNAL)) environment->external_user = 1;

            UDA_LOG(UDA_LOG_DEBUG, "client protocolVersion %d\n", protocolVersion);
            UDA_LOG(UDA_LOG_DEBUG, "privateFlags %d\n", privateFlags);
            UDA_LOG(UDA_LOG_DEBUG, "clientFlags  %d\n", clientFlags);
            UDA_LOG(UDA_LOG_DEBUG, "altRank      %d\n", altRank);
            UDA_LOG(UDA_LOG_DEBUG, "external?    %d\n", environment->external_user);

            if (normalLegacyWait) {

                protocol_id = PROTOCOL_SERVER_BLOCK;

                UDA_LOG(UDA_LOG_DEBUG, "Sending Server Block\n");

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &server_block, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 11 Error (Sending Server Block #1)");
                    concatIdamError(&server_block.idamerrorstack); // Update Server State with Error Stack
                    closeIdamError();
                    normalLegacyWait = 1;
                    fatal = 1;
                }

                if (fatal) {
                    if (server_block.idamerrorstack.nerrors > 0) {
                        err = server_block.idamerrorstack.idamerror[0].code;
                    } else {
                        err = 1;
                    }
                    break;                // Manage the Fatal Server State
                }
            }

            normalLegacyWait = 1;      // Enable client & server state block legacy exchange

            //-------------------------------------------------------------------------
            // Client Request
            //
            // Errors: Fatal to Data Access
            //	   Pass Back and Await Client Instruction

            protocol_id = PROTOCOL_REQUEST_BLOCK;

            if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                &request_block, protocolVersion)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Request Block\n");
                addIdamError(CODEERRORTYPE, "idamServer", err,
                             "Protocol 1 Error (Receiving Client Request)");
                break;
            }

            rc = xdrrec_eof(serverInput);
            UDA_LOG(UDA_LOG_DEBUG, "Request Block Received\n");
            UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? %d\n", rc);

            printClientBlock(client_block);
            printServerBlock(server_block);
            printRequestBlock(request_block);

            //------------------------------------------------------------------------------------------------------------------
            // Prepend Proxy Host to Source to redirect client request

            /*! On parallel clusters where nodes are connected together on a private network, only the master node may have access
            to external data sources. Cluster nodes can access these external sources via an IDAM server running on the master node.
            This server acts as a proxy server. It simply redirects requests to other external IDAM servers. To facilitate this redirection,
            each access request source string must be prepended with "IDAM::host:port/" within the server. The host:port component is defined
            by the system administrator via an environment variable "IDAM_PROXY". Client's don't need to specifiy redirection via a Proxy - it's
            automatic if the IDAM_PROXY environment variable is defined. No prepending is done if the source is already a redirection, i.e. it
            begins "IDAM::".
            */

#ifdef PROXYSERVER

            char work[STRING_LENGTH];

            if(request_block.api_delim[0] != '\0')
                sprintf(work, "IDAM%s", request_block.api_delim);
            else
                sprintf(work, "IDAM%s", environment->api_delim);

            if(environment->server_proxy[0] != '\0' && strncasecmp(request_block.source, work, strlen(work)) != 0) {

// Check the Server Version is Compatible with the Originating client version ?

                if(client_block.version < 6) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
                    break;
                }

// Test for Proxy calling itself indirectly => potential infinite loop
// The IDAM Plugin strips out the host and port data from the source so the originating server details are never passed.

                if(request_block.api_delim[0] != '\0')
                    sprintf(work, "IDAM%s%s", request_block.api_delim, environment->server_this);
                else
                    sprintf(work, "IDAM%s%s", environment->api_delim, environment->server_this);

                if(strstr(request_block.source, work) != nullptr) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                    break;
                }

// Check string length compatibility

                if(strlen(request_block.source) >= (STRING_LENGTH-1 - strlen(environment->server_proxy) - 4+strlen(request_block.api_delim))) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "PROXY redirection: The source argument string is too long!");
                    break;
                }

// Prepend the redirection IDAM server details

                if(request_block.api_delim[0] != '\0')
                    sprintf(work, "IDAM%s%s/%s", request_block.api_delim, environment->server_proxy, request_block.source);
                else
                    sprintf(work, "IDAM%s%s/%s", environment->api_delim, environment->server_proxy, request_block.source);

                strcpy(request_block.source, work);
                //strcpy(request_block.server, environment->server_proxy);

                if(debugon) {
                    UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s\n", environment->server_proxy);
                    UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request_block.source);
                    //UDA_LOG(UDA_LOG_DEBUG, "server: %s\n", request_block.server);
                }

            }

#endif

            //----------------------------------------------------------------------
            // Write to the Access Log

            idamAccessLog(TRUE, client_block, request_block, server_block, pluginlist, getIdamServerEnvironment());

            //----------------------------------------------------------------------
            // Initialise Data Structures

            initDataSource(&data_source);
            initSignalDesc(&signal_desc);
            initSignal(&signal_rec);

            //----------------------------------------------------------------------------------------------
            // If this is a PUT request then receive the putData structure

            initIdamPutDataBlockList(&(request_block.putDataBlockList));

            if (request_block.put) {

                protocol_id = PROTOCOL_PUTDATA_BLOCK_LIST;

                if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                                    &(request_block.putDataBlockList), protocolVersion)) !=
                    0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving putData Block List\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 1 Error (Receiving Client putDataBlockList)");
                    break;
                }

                rc = (int)xdrrec_eof(serverInput);
                UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received\n");
                UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", request_block.putDataBlockList.blockCount);
                UDA_LOG(UDA_LOG_DEBUG, "XDR #C xdrrec_eof ? %d\n", rc);
            }


            //----------------------------------------------------------------------------------------------
            // Decode the API Arguments: determine appropriate data plug-in to use
            // Decide on Authentication procedure

            if (protocolVersion >= 6) {
                if ((err = idamServerPlugin(&request_block, &data_source, &signal_desc, pluginlist,
                                            getIdamServerEnvironment())) != 0) {
                    break;
                }
            } else {
                if ((err = idamServerLegacyPlugin(&request_block, &data_source, &signal_desc)) != 0) break;
            }

            //------------------------------------------------------------------------------------------------
            // Query the Database: Internal or External Data Sources
            // Read the Data or Create the Composite/Derived Data
            // Apply XML Actions to Data

            depth = 0;

            err = udaGetData(&depth, &request_block, client_block, &data_block, &data_source, &signal_rec, &signal_desc,
                             &actions_desc, &actions_sig, pluginlist, logmalloclist, userdefinedtypelist, socket_list,
                             protocolVersion);

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
            printIdamErrorStack();
            UDA_LOG(UDA_LOG_DEBUG,
                    "======================== ******************** ==========================================\n");

            if (err != 0) break;

            //------------------------------------------------------------------------------------------------
            // Server-Side Data Processing

            if (client_block.get_dimdble || client_block.get_timedble || client_block.get_scalar) {
                if (serverProcessing(client_block, &data_block) != 0) {
                    err = 779;
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Server-Side Processing Error");
                    break;
                }
            }

            //----------------------------------------------------------------------------
            // Check the Client can receive the data type: Version dependent
            // Otherwise inform the client via the server state block

            if (protocolVersion < 6 && data_block.data_type == UDA_TYPE_STRING) data_block.data_type = UDA_TYPE_CHAR;

            if (data_block.data_n > 0 &&
                (protocolVersionTypeTest(protocolVersion, data_block.data_type) ||
                 protocolVersionTypeTest(protocolVersion, data_block.error_type))) {
                err = 999;
                addIdamError(CODEERRORTYPE, "idamServer", err,
                             "The Data has a type that cannot be passed to the Client: A newer client library version is required.");
                break;
            }

            if (data_block.rank > 0) {
                unsigned int i;
                DIMS dim;
                for (i = 0; i < data_block.rank; i++) {
                    dim = data_block.dims[i];
                    if (protocolVersionTypeTest(protocolVersion, dim.data_type) ||
                        protocolVersionTypeTest(protocolVersion, dim.error_type)) {
                        err = 999;
                        addIdamError(CODEERRORTYPE, "idamServer", err,
                                     "A Coordinate Data has a numerical type that cannot be passed to the Client: A newer client library version is required.");
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

            concatIdamError(&server_block.idamerrorstack); // Update Server State with Error Stack
            closeIdamError();

            printServerBlock(server_block);

            if (server_block.idamerrorstack.nerrors > 0) {
                server_block.error = server_block.idamerrorstack.idamerror[0].code;
                strcpy(server_block.msg, server_block.idamerrorstack.idamerror[0].msg);
            }

            protocol_id = PROTOCOL_SERVER_BLOCK;

            if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                &server_block, protocolVersion)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block #2\n");
                addIdamError(CODEERRORTYPE, "idamServer", err,
                             "Protocol 11 Error (Sending Server Block #2)");
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

                protocol_id = PROTOCOL_NEXT_PROTOCOL;

                if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                    userdefinedtypelist, nullptr, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #1 Receiving Next Protocol ID\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 3 (Next Protocol #1) Error");
                    break;
                }

                rc = xdrrec_eof(serverInput);
                UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
                UDA_LOG(UDA_LOG_DEBUG, "XDR #D xdrrec_eof ? %d\n", rc);

                if (next_protocol != PROTOCOL_DATA_SYSTEM) {
                    err = 998;
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 3 Error: Protocol Request Inconsistency");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data System Structure

                protocol_id = PROTOCOL_DATA_SYSTEM;

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_system, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 4 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the System Configuration Structure

                protocol_id = PROTOCOL_SYSTEM_CONFIG;

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &system_config, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 5 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Data Source Structure

                protocol_id = PROTOCOL_DATA_SOURCE;

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_source, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 6 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Structure

                protocol_id = PROTOCOL_SIGNAL;

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &signal_rec, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 7 Error");
                    break;
                }

                //----------------------------------------------------------------------------
                // Send the Signal Description Structure

                protocol_id = PROTOCOL_SIGNAL_DESC;

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &signal_desc, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 8 Error");
                    break;
                }

            } // End of Database Meta Data

            //----------------------------------------------------------------------------
            // Next Protocol id

            protocol_id = PROTOCOL_NEXT_PROTOCOL;

            if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                userdefinedtypelist, nullptr, protocolVersion)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem #2 Receiving Next Protocol ID\n");
                addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 3 (Next Protocol #2) Error");
                break;
            }

            rc = xdrrec_eof(serverInput);
            UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
            UDA_LOG(UDA_LOG_DEBUG, "XDR #E xdrrec_eof ? %d\n", rc);

            //----------------------------------------------------------------------------
            // Send the Data

            if (next_protocol != PROTOCOL_DATA_BLOCK) {
                err = 997;
                addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 3 Error: Incorrect Request");
                break;
            }

            printDataBlock(data_block);
            UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client\n");

            protocol_id = PROTOCOL_DATA_BLOCK;

            if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                &data_block, protocolVersion)) != 0) {
                UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure\n");
                addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 2 Error");
                break;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client\n");

            //------------------------------------------------------------------------------
            // Clear Output Buffer (check - it should be empty!) and receive Next Protocol

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

                protocol_id = PROTOCOL_NEXT_PROTOCOL;

                if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist,
                                    userdefinedtypelist, nullptr, protocolVersion)) != 0) {
                    UDA_LOG(UDA_LOG_DEBUG, "Problem #2a Receiving Next Protocol ID\n");
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Protocol 3 (Next Protocol #2) Error");
                    break;
                }

                if (next_protocol != PROTOCOL_STRUCTURES) {
                    err = 999;
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Incorrect Next Protocol received: (Structures)");
                    break;
                }
            }

            //------------------------------------------------------------------------------
            // Hierarchical Data Structures

            if (data_block.opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {
                if (data_block.opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
                    protocol_id = PROTOCOL_META;
                } else {
                    if (data_block.opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                        data_block.opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                        protocol_id = PROTOCOL_STRUCTURES;
                    } else {
                        protocol_id = PROTOCOL_EFIT;
                    }
                }

                UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client\n");

                if ((err = protocol(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                    &data_block, protocolVersion)) != 0) {
                    addIdamError(CODEERRORTYPE, "idamServer", err,
                                 "Server Side Protocol Error (Opaque Structure Type)");
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

        idamAccessLog(0, client_block, request_block, server_block, pluginlist, getIdamServerEnvironment());

        //----------------------------------------------------------------------------
        // Server Shutdown ? Next Instruction from Client
        //
        // Protocols:   13 => Die
        //		14 => Sleep
        //		15 => Wakeup

        // <========================== Client Server Code Only

        protocol_id = PROTOCOL_NEXT_PROTOCOL;
        next_protocol = 0;

        if ((err = protocol(serverInput, protocol_id, XDR_RECEIVE, &next_protocol, logmalloclist, userdefinedtypelist,
                            nullptr, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem #3 Receiving Next Protocol ID\n");
            addIdamError(CODEERRORTYPE, "idamServer", err, "Protocol 3 (Server Shutdown) Error");
            break;
        }

        rc = xdrrec_eof(serverInput);
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);
        UDA_LOG(UDA_LOG_DEBUG, "XDR #F xdrrec_eof ? %d\n", rc);
        UDA_LOG(UDA_LOG_DEBUG, "Current Error Value %d\n", err);

        UDA_LOG(UDA_LOG_DEBUG, "Client Request %d\n", next_protocol);
        if (next_protocol == PROTOCOL_CLOSEDOWN) UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Die\n");
        if (next_protocol == PROTOCOL_SLEEP) UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Sleep\n");
        if (next_protocol == PROTOCOL_WAKE_UP) UDA_LOG(UDA_LOG_DEBUG, "Client Requests Server Wake-up\n");

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlock\n");
        freeDataBlock(&data_block);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_desc);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_sig);

        //----------------------------------------------------------------------------
        // Free Name Value pair

        freeNameValueList(&request_block.nameValueList);

        //----------------------------------------------------------------------------
        // Free PutData Blocks

        freeIdamServerPutDataBlockList(&request_block.putDataBlockList);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concatIdamError\n");
        concatIdamError(&server_block.idamerrorstack);        // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "closeIdamError\n");
        closeIdamError();

        UDA_LOG(UDA_LOG_DEBUG, "idamErrorLog\n");
        idamErrorLog(client_block, request_block, &server_block.idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "closeIdamError\n");
        closeIdamError();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock\n");
        initServerBlock(&server_block, serverVersion);

        UDA_LOG(UDA_LOG_DEBUG, "At End of Error Trap\n");

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == PROTOCOL_SLEEP && sleepServer(logmalloclist, userdefinedtypelist, protocolVersion));

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

    freeUserDefinedTypeList(&parseduserdefinedtypelist);

    //----------------------------------------------------------------------------
    // Close the Socket Connections to Other Data Servers

    closeServerSockets(socket_list);

    //----------------------------------------------------------------------------
    // Write the Error Log Record & Free Error Stack Heap

    idamErrorLog(client_block, request_block, nullptr);
    closeIdamError();

    //----------------------------------------------------------------------------
    // Close the Logs

    fflush(nullptr);

    idamCloseLogging();

    return 0;
}

#endif
