#include <cstdio>
#if defined(__GNUC__)
#  include <strings.h>
#else
#  define strncasecmp _strnicmp
#endif

#include <clientserver/initStructs.h>
#include <clientserver/makeRequestBlock.h>
#include <clientserver/manageSockets.h>
#include <clientserver/printStructs.h>
#include <clientserver/protocol.h>
#include <clientserver/udaErrors.h>
#include <clientserver/xdrlib.h>
#include <logging/accessLog.h>
#include <server/serverPlugin.h>
#include <structures/parseIncludeFile.h>
#include <structures/struct.h>

#include "closeServerSockets.h"
#include "createXDRStream.h"
#include "getServerEnvironment.h"
#include "serverGetData.h"
#include "serverLegacyPlugin.h"
#include "serverProcessing.h"
#include "serverStartup.h"
#include "udaLegacyServer.h"

#ifdef SECURITYENABLED
#  include <security/serverAuthentication.h>
#endif

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include <authentication/udaServerSSL.h>
#endif

//--------------------------------------------------------------------------------------
// static globals

static XDR serverXDRInput;
static XDR serverXDROutput;

XDR* serverInput = &serverXDRInput;
XDR* serverOutput = &serverXDROutput;

int server_tot_block_time = 0;

int serverVersion = 8;
static int protocolVersion = 8;

//#if !defined(FATCLIENT)
static int legacyServerVersion = 6;
//#endif

NTREELIST NTreeList;
NTREE* fullNTree = nullptr;
LOGSTRUCTLIST logstructlist;

int server_timeout = TIMEOUT;        // user specified Server Lifetime

static USERDEFINEDTYPELIST* userdefinedtypelist = nullptr;            // User Defined Structure Types from Data Files & Plugins
static LOGMALLOCLIST* logmalloclist = nullptr;                        // List of all Heap Allocations for Data: Freed after data is dispatched
int malloc_source = MALLOCSOURCENONE;

USERDEFINEDTYPELIST parseduserdefinedtypelist;              // Initial set of User Defined Structure Types

unsigned int totalDataBlockSize = 0;                        // Total amount sent for the last data request
unsigned int clientFlags = 0;
int altRank = 0;
unsigned int privateFlags = 0;
unsigned int XDRstdioFlag = 0;                              // Flags the XDR stream is to stdio not a socket

static PLUGINLIST pluginList;                // List of all data reader plugins (internal and external shared libraries)
ENVIRONMENT environment;                // Holds local environment variable values

static SOCKETLIST socket_list;

typedef struct MetadataBlock {
    DATA_SOURCE data_source;
    SIGNAL signal_rec;
    SIGNAL_DESC signal_desc;
    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
} METADATA_BLOCK;

static int startupServer(SERVER_BLOCK* server_block);

static int handleRequest(REQUEST_BLOCK* request_block, CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block,
                         METADATA_BLOCK* metadata_block, ACTIONS* actions_desc, ACTIONS* actions_sig,
                         DATA_BLOCK_LIST* data_block_list, int* fatal, int* server_closedown);

static int doServerLoop(REQUEST_BLOCK* request_block, DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block,
                        SERVER_BLOCK* server_block, METADATA_BLOCK* metadata_block, ACTIONS* actions_desc,
                        ACTIONS* actions_sig, int* fatal);

static int reportToClient(SERVER_BLOCK* server_block, DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block,
                          int trap1Err, METADATA_BLOCK* metadata_block);

static int doServerClosedown(CLIENT_BLOCK* client_block, REQUEST_BLOCK* request_block, DATA_BLOCK_LIST* data_block_list);

#ifdef SECURITYENABLED
static int authenticateClient(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block);
#else
static int handshakeClient(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, int* server_closedown);
#endif

//--------------------------------------------------------------------------------------
// Server Entry point

int udaServer(CLIENT_BLOCK client_block)
{
    int err = 0;
    METADATA_BLOCK metadata_block;
    memset(&metadata_block, '\0', sizeof(METADATA_BLOCK));

    REQUEST_BLOCK request_block;
    SERVER_BLOCK server_block;

    ACTIONS actions_desc;
    ACTIONS actions_sig;

    //-------------------------------------------------------------------------
    // Initialise the Error Stack & the Server Status Structure
    // Reinitialised after each logging action

    initUdaErrorStack();
    initServerBlock(&server_block, serverVersion);
    initActions(&actions_desc);        // There may be a Sequence of Actions to Apply
    initActions(&actions_sig);
    initRequestBlock(&request_block);

    if ((err = startupServer(&server_block)) != 0) return err;

#ifdef SECURITYENABLED
    err = authenticateClient(&client_block, &server_block);
#else
    int server_closedown = 0;
    err = handshakeClient(&client_block, &server_block, &server_closedown);
#endif

    DATA_BLOCK_LIST data_block_list;
    data_block_list.count = 0;
    data_block_list.data = nullptr;

    if (!err && !server_closedown) {
        int fatal = 0;
        doServerLoop(&request_block, &data_block_list, &client_block, &server_block, &metadata_block, &actions_desc,
                     &actions_sig, &fatal);
    }

    err = doServerClosedown(&client_block, &request_block, &data_block_list);

    return err;
}

int reportToClient(SERVER_BLOCK* server_block, DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block, int trap1Err,
                   METADATA_BLOCK* metadata_block)
{
    //----------------------------------------------------------------------------
    // Gather Server Error State

    // Update Server State with Error Stack
    concatUdaError(&server_block->idamerrorstack);
    closeUdaError();

    int err = 0;

    if (server_block->idamerrorstack.nerrors > 0) {
        server_block->error = server_block->idamerrorstack.idamerror[0].code;
        strcpy(server_block->msg, server_block->idamerrorstack.idamerror[0].msg);
    }

    //------------------------------------------------------------------------------------------------
    // How much data to be sent?

    totalDataBlockSize = countDataBlockListSize(data_block_list, client_block);

    printServerBlock(*server_block);

    //------------------------------------------------------------------------------------------------
    // Send the server block and all data in a single (minimal number) tcp packet

    int protocol_id = PROTOCOL_SERVER_BLOCK;

    if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                         server_block, protocolVersion)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block #2\n");
        addIdamError(CODEERRORTYPE, __func__, err, "Protocol 11 Error (Sending Server Block #2)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Sent to Client without error\n");

    if (server_block->idamerrorstack.nerrors > 0) {
        err = server_block->idamerrorstack.idamerror[0].code;
    } else {
        err = trap1Err;
    }

    if (err != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop\n");

        // Send the Server Block and byepass sending data (there are none!)

        if (!xdrrec_endofrecord(serverOutput, 1)) {
            err = PROTOCOL_ERROR_7;
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Server Data Block #2\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 7 Error (Server Block)");
            return err;
        }

        return err;
    }

    if (client_block->get_meta) {

        totalDataBlockSize +=
                sizeof(DATA_SYSTEM) + sizeof(SYSTEM_CONFIG) + sizeof(DATA_SOURCE) + sizeof(SIGNAL) +
                sizeof(SIGNAL_DESC);

        //----------------------------------------------------------------------------
        // Send the Data System Structure

        protocol_id = PROTOCOL_DATA_SYSTEM;

        if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                             &metadata_block->data_system, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data System Structure\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 4 Error");
            return err;
        }

        //----------------------------------------------------------------------------
        // Send the System Configuration Structure

        protocol_id = PROTOCOL_SYSTEM_CONFIG;

        if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                             &metadata_block->system_config, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending System Configuration Structure\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 5 Error");
            return err;
        }

        //----------------------------------------------------------------------------
        // Send the Data Source Structure

        protocol_id = PROTOCOL_DATA_SOURCE;

        if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                             &metadata_block->data_source, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Source Structure\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 6 Error");
            return err;
        }

        //----------------------------------------------------------------------------
        // Send the Signal Structure

        protocol_id = PROTOCOL_SIGNAL;

        if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                             &metadata_block->signal_rec, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Structure\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 7 Error");
            return err;
        }

        //----------------------------------------------------------------------------
        // Send the Signal Description Structure

        protocol_id = PROTOCOL_SIGNAL_DESC;

        if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                             &metadata_block->signal_desc, protocolVersion)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Signal Description Structure\n");
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 8 Error");
            return err;
        }

    } // End of Database Meta Data

    //----------------------------------------------------------------------------
    // Send the Data

    printDataBlockList(*data_block_list);
    UDA_LOG(UDA_LOG_DEBUG, "Sending Data Block Structure to Client\n");

    if ((err = protocol2(serverOutput, PROTOCOL_DATA_BLOCK_LIST, XDR_SEND, nullptr, logmalloclist,
                         userdefinedtypelist, data_block_list, protocolVersion)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Problem Sending Data Structure\n");
        addIdamError(CODEERRORTYPE, __func__, err, "Protocol 2 Error");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Data Block Sent to Client\n");

    //----------------------------------------------------------------------------
    // Send the data in a single full TCP packet

    // ******* is this an extra XDR EOF ?????

    if (!xdrrec_endofrecord(serverOutput, 1)) {
        err = PROTOCOL_ERROR_7;
        addIdamError(CODEERRORTYPE, "idamClient", err, "Protocol 7 Error (Server Block)");
        return err;
    }

    //------------------------------------------------------------------------------
    // Legacy Hierarchical Data Structures

    for (int i = 0; i < data_block_list->count; ++i) {
        DATA_BLOCK* data_block = &data_block_list->data[i];

        if (protocolVersion < 9 && data_block->data_type == UDA_TYPE_COMPOUND &&
            data_block->opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {
            if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
                protocol_id = PROTOCOL_META;
            } else {
                if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                    data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE) {
                    protocol_id = PROTOCOL_STRUCTURES;
                } else {
                    protocol_id = PROTOCOL_EFIT;
                }
            }

            UDA_LOG(UDA_LOG_DEBUG, "Sending Hierarchical Data Structure to Client\n");

            if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                                 data_block, protocolVersion)) != 0) {
                addIdamError(CODEERRORTYPE, __func__, err, "Server Side Protocol Error (Opaque Structure Type)");
                return err;
            }

            UDA_LOG(UDA_LOG_DEBUG, "Hierarchical Data Structure sent to Client\n");
        }
    }

    return err;
}

int handleRequest(REQUEST_BLOCK* request_block, CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block,
                  METADATA_BLOCK* metadata_block, ACTIONS* actions_desc, ACTIONS* actions_sig,
                  DATA_BLOCK_LIST* data_block_list, int* fatal, int* server_closedown)
{
    UDA_LOG(UDA_LOG_DEBUG, "Start of Server Error Trap #1 Loop\n");

    //----------------------------------------------------------------------------
    // Client and Server States
    //
    // Errors: Fatal to Data Access
    //	   Pass Back and Await Client Instruction

    int err = 0;

    initClientBlock(client_block, 0, "");

    UDA_LOG(UDA_LOG_DEBUG, "Waiting to receive Client Block\n");

    // Receive the Client Block, request block and putData block

    if (!xdrrec_skiprecord(serverInput)) {
        *fatal = 1;
        THROW_ERROR(PROTOCOL_ERROR_5, "Protocol 5 Error (Client Block)");
    }

    int protocol_id = PROTOCOL_CLIENT_BLOCK;

    if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                         client_block, protocolVersion)) != 0) {
        if (server_tot_block_time >= 1000 * server_timeout) {
            *fatal = 1;
            THROW_ERROR(999, "Server Time Out");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Problem Receiving Client Data Block\n");

        addIdamError(CODEERRORTYPE, __func__, err, "Protocol 10 Error (Receiving Client Block)");
        concatUdaError(&server_block->idamerrorstack); // Update Server State with Error Stack
        closeUdaError();

        *fatal = 1;
        return err;
    }

    server_timeout = client_block->timeout;         // User specified Server Lifetime
    privateFlags = client_block->privateFlags;      // Server to Server flags
    clientFlags = client_block->clientFlags;        // Client set flags
    altRank = client_block->altRank;                // Rank of Alternative source

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket

    protocolVersion = serverVersion;
    if (client_block->version < serverVersion) {
        protocolVersion = client_block->version;
    }

    // The client request may originate from a server.
    // Is the Originating server an externally facing server? If so then switch to this mode: preserve local access policy

    if (!environment.external_user && (privateFlags & PRIVATEFLAG_EXTERNAL)) {
        environment.external_user = 1;
    }

    UDA_LOG(UDA_LOG_DEBUG, "client protocolVersion %d\n", protocolVersion);
    UDA_LOG(UDA_LOG_DEBUG, "privateFlags %d\n", privateFlags);
    UDA_LOG(UDA_LOG_DEBUG, "clientFlags  %d\n", clientFlags);
    UDA_LOG(UDA_LOG_DEBUG, "altRank      %d\n", altRank);
    UDA_LOG(UDA_LOG_DEBUG, "external?    %d\n", environment.external_user);

    if (server_block->idamerrorstack.nerrors > 0) {
        server_block->error = server_block->idamerrorstack.idamerror[0].code;
        strcpy(server_block->msg, server_block->idamerrorstack.idamerror[0].msg);
    }

    // Test the client version is compatible with this server version

    if (protocolVersion > serverVersion) {
        THROW_ERROR(999, "Protocol Error: Client API Version is Newer than the Server Version");
    }

    if (*fatal) {
        if (server_block->idamerrorstack.nerrors > 0) {
            err = server_block->idamerrorstack.idamerror[0].code;
        } else {
            err = 1;
        }
        return err;                // Manage the Fatal Server State
    }

    // Test for an immediate CLOSEDOWN instruction

    if (client_block->timeout == 0 || (client_block->clientFlags & CLIENTFLAG_CLOSEDOWN)) {
        *server_closedown = 1;
        return err;
    }

    //-------------------------------------------------------------------------
    // Client Request
    //
    // Errors: Fatal to Data Access
    //	   Pass Back and Await Client Instruction

    if ((err = protocol2(serverInput, PROTOCOL_REQUEST_BLOCK, XDR_RECEIVE, nullptr, logmalloclist,
                         userdefinedtypelist, request_block, protocolVersion)) != 0) {
        THROW_ERROR(err, "Protocol 1 Error (Receiving Client Request)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Request Block Received\n");

    printClientBlock(*client_block);
    printServerBlock(*server_block);
    printRequestBlock(*request_block);

    data_block_list->count = request_block->num_requests;
    data_block_list->data = (DATA_BLOCK*)calloc(data_block_list->count, sizeof(DATA_BLOCK));

    //------------------------------------------------------------------------------------------------------------------
    // Prepend Proxy Host to Source to redirect client request

    /*! On parallel clusters where nodes are connected together on a private network, only the master node may have
     * access to external data sources. Cluster nodes can access these external sources via an UDA server running on
     * the master node.
     * This server acts as a proxy server. It simply redirects requests to other external UDA servers. To facilitate
     * this redirection, each access request source string must be prepended with "UDA::host:port/" within the proxy
     * server.
     * The host:port component is defined by the system administrator via an environment variable "UDA_PROXY".
     * Clients don't need to specifiy redirection via a Proxy - it's automatic if the UDA_PROXY environment variable
     * is defined.
     * No prepending is done if the source is already a redirection, i.e. it begins "UDA::".
     *
     * The name of the proxy reirection plugin is UDA by default but may be changed using the environment variable
     * UDA_PROXYPLUGINNAME
    */

# ifdef PROXYSERVER

    // Name of the Proxy plugin

    char* proxyNameDefault = "UDA";
    char* proxyName = nullptr;

    char work[STRING_LENGTH];

    if ((proxyName = getenv("UDA_PROXYPLUGINNAME")) == nullptr) proxyName = proxyNameDefault;

    // Check string length compatibility

    if (strlen(request_block->source) >=
        (STRING_LENGTH - 1 - strlen(proxyName) - strlen(environment.server_proxy) - strlen(request_block->api_delim))) {
        THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
    }

    // Prepend the client request and test for a redirection request via the proxy's plugin

    if (request_block->api_delim[0] != '\0') {
        snprintf(work, STRING_LENGTH, "%s%s", proxyName, request_block->api_delim);
    } else {
        snprintf(work, STRING_LENGTH, "%s%s", proxyName, environment.api_delim);
    }

    if (strncasecmp(request_block->source, work, strlen(work)) != 0) {
        // Not a recognised redirection so prepending is necessary

        // Has a proxy host been specified in the server startup script? If not assume the plugin has a default host and port

        if (environment.server_proxy[0] == '\0') {
            if (request_block->api_delim[0] != '\0') {
                snprintf(work, STRING_LENGTH, "%s%s%s", proxyName, request_block->api_delim, request_block->source);
            } else {
                snprintf(work, STRING_LENGTH, "%s%s%s", proxyName, environment.api_delim, request_block->source);
            }

            strcpy(request_block->source, work);

        } else {                                                // UDA::host.port/source

            // Check the Server Version is Compatible with the Originating client version ?

            if (client_block->version < 6) {
                THROW_ERROR(999, "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are never passed.
            // Primitive test as the same IP address can be mapped to different names!
            // Should pass on the number of redirections and cap the limit!

            if (environment.server_this[0] != '\0') {
                if (request_block->api_delim[0] != '\0') {
                    snprintf(work, STRING_LENGTH, "%s%s%s", proxyName, request_block->api_delim, environment.server_this);
                } else {
                    snprintf(work, STRING_LENGTH, "%s%s%s", proxyName, environment.api_delim, environment.server_this);
                }

                if (strstr(request_block->source, work) != nullptr) {
                    THROW_ERROR(999, "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                }
            }

            // Prepend the redirection UDA server details and replace the original

            if (request_block->source[0] == '/') {
                if (request_block->api_delim[0] != '\0')
                    snprintf(work, STRING_LENGTH, "%s%s%s%s", proxyName, request_block->api_delim, environment.server_proxy, request_block->source);
                else
                    snprintf(work, STRING_LENGTH, "%s%s%s%s", proxyName, environment.api_delim, environment.server_proxy, request_block->source);
            } else {
                if (request_block->api_delim[0] != '\0')
                    snprintf(work, STRING_LENGTH, "%s%s%s/%s", proxyName, request_block->api_delim, environment.server_proxy, request_block->source);
                else
                    snprintf(work, STRING_LENGTH, "%s%s%s/%s", proxyName, environment.api_delim, environment.server_proxy, request_block->source);
            }
            strcpy(request_block->source, work);
        }

        UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s avoiding %s\n", environment.server_proxy, environment.server_this);
        UDA_LOG(UDA_LOG_DEBUG, "plugin: %s\n", proxyName);
        UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request_block->source);
    }

# else

    for (int i = 0; i < request_block->num_requests; ++i) {
        REQUEST_DATA* request = &request_block->requests[0];

        char work[1024];
        if (request->api_delim[0] != '\0') {
            snprintf(work, 1024, "UDA%s", request->api_delim);
        } else {
            snprintf(work, 1024, "UDA%s", environment.api_delim);
        }

        if (environment.server_proxy[0] != '\0' && strncasecmp(request->source, work, strlen(work)) != 0) {

            // Check the Server Version is Compatible with the Originating client version ?

            if (client_block->version < 6) {
                THROW_ERROR(999,
                            "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are never passed.

            if (request->api_delim[0] != '\0') {
                snprintf(work, 1024, "UDA%s%s", request->api_delim, environment.server_this);
            } else {
                snprintf(work, 1024, "UDA%s%s", environment.api_delim, environment.server_this);
            }

            if (strstr(request->source, work) != nullptr) {
                THROW_ERROR(999,
                            "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
            }

            // Check string length compatibility

            if (strlen(request->source) >=
                (STRING_LENGTH - 1 - strlen(environment.server_proxy) - 4 + strlen(request->api_delim))) {
                THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
            }

            // Prepend the redirection UDA server details

            if (request->api_delim[0] != '\0') {
                snprintf(work, 1024, "UDA%s%s/%s", request->api_delim, environment.server_proxy, request->source);
            } else {
                snprintf(work, 1024, "UDA%s%s/%s", environment.api_delim, environment.server_proxy, request->source);
            }

            strcpy(request->source, work);

            UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s\n", environment.server_proxy);
            UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request->source);
        }
    }
# endif

    //----------------------------------------------------------------------
    // Write to the Access Log

    udaAccessLog(TRUE, *client_block, *request_block, *server_block, &pluginList, getServerEnvironment());

    //----------------------------------------------------------------------
    // Initialise Data Structures

    initDataSource(&metadata_block->data_source);
    initSignalDesc(&metadata_block->signal_desc);
    initSignal(&metadata_block->signal_rec);

    //----------------------------------------------------------------------------------------------
    // If this is a PUT request then receive the putData structure

    for (int i = 0; i < request_block->num_requests; ++i) {
        REQUEST_DATA* request = &request_block->requests[0];

        initPutDataBlockList(&request->putDataBlockList);

        if (request->put) {
            if ((err = protocol2(serverInput, PROTOCOL_PUTDATA_BLOCK_LIST, XDR_RECEIVE, nullptr, logmalloclist,
                                 userdefinedtypelist, &request->putDataBlockList, protocolVersion)) != 0) {
                THROW_ERROR(err, "Protocol 1 Error (Receiving Client putDataBlockList)");
            }

            UDA_LOG(UDA_LOG_DEBUG, "putData Block List Received\n");
            UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", request->putDataBlockList.blockCount);
        }
    }

    // Flush (mark as at EOF) the input socket buffer: no more data should be read from this point

    xdrrec_eof(serverInput);

    UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Accessing data.\n");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data plug-in to use
    // Decide on Authentication procedure

    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[i];
        if (protocolVersion >= 6) {
            if ((err = udaServerPlugin(request, &metadata_block->data_source, &metadata_block->signal_desc,
                                       &pluginList, getServerEnvironment())) != 0) {
                return err;
            }
        } else {
            if ((err = udaServerLegacyPlugin(request, &metadata_block->data_source, &metadata_block->signal_desc)) !=
                0) {
                return err;
            }
        }
    }

    //------------------------------------------------------------------------------------------------
    // Query the Database: Internal or External Data Sources
    // Read the Data or Create the Composite/Derived Data
    // Apply XML Actions to Data

    for (int i = 0; i < request_block->num_requests; ++i) {
        auto request = &request_block->requests[i];
        DATA_BLOCK* data_block = &data_block_list->data[i];
        int depth = 0;
        err = udaGetData(&depth, request, *client_block, data_block, &metadata_block->data_source,
                         &metadata_block->signal_rec, &metadata_block->signal_desc, actions_desc, actions_sig,
                         &pluginList, logmalloclist, userdefinedtypelist, &socket_list, protocolVersion);
    }

    for (int i = 0; i < request_block->num_requests; ++i) {
        request_block->requests[i].function[0] = '\0';
    }

    DATA_SOURCE* data_source = &metadata_block->data_source;
    SIGNAL_DESC* signal_desc = &metadata_block->signal_desc;
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source->pass);
//    UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : %d \n", depth);
    printRequestBlock(*request_block);
    printDataSource(*data_source);
    printSignal(metadata_block->signal_rec);
    printSignalDesc(*signal_desc);
    printDataBlockList(*data_block_list);
    printIdamErrorStack();
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");

    if (err != 0) return err;

    //------------------------------------------------------------------------------------------------
    // Server-Side Data Processing

    if (client_block->get_dimdble || client_block->get_timedble || client_block->get_scalar) {
        for (int i = 0; i < data_block_list->count; ++i) {
            DATA_BLOCK* data_block = &data_block_list->data[i];
            if (serverProcessing(*client_block, data_block) != 0) {
                THROW_ERROR(779, "Server-Side Processing Error");
            }
        }
    }

    //----------------------------------------------------------------------------
    // Check the Client can receive the data type: Version dependent
    // Otherwise inform the client via the server state block

    for (int i = 0; i < data_block_list->count; ++i) {
        DATA_BLOCK* data_block = &data_block_list->data[i];

        if (protocolVersion < 6 && data_block->data_type == UDA_TYPE_STRING) {
            data_block->data_type = UDA_TYPE_CHAR;
        }

        if (data_block->data_n > 0 &&
            (protocolVersionTypeTest(protocolVersion, data_block->data_type) ||
             protocolVersionTypeTest(protocolVersion, data_block->error_type))) {
            THROW_ERROR(999,
                        "The Data has a type that cannot be passed to the Client: A newer client library version is required.");
        }

        if (data_block->rank > 0) {
            DIMS dim;
            for (unsigned int j = 0; j < data_block->rank; j++) {
                dim = data_block->dims[j];
                if (protocolVersionTypeTest(protocolVersion, dim.data_type) ||
                    protocolVersionTypeTest(protocolVersion, dim.error_type)) {
                    THROW_ERROR(999,
                                "A Coordinate Data has a numerical type that cannot be passed to the Client: A newer client library version is required.");
                }
            }
        }
    }

    //----------------------------------------------------------------------------
    // End of Error Trap #1
    // If an error has occued within this trap, then a problem occured accessing data
    // The server block should be returned with the error stack

    return err;
}

int doServerLoop(REQUEST_BLOCK* request_block, DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block,
                 SERVER_BLOCK* server_block, METADATA_BLOCK* metadata_block, ACTIONS* actions_desc,
                 ACTIONS* actions_sig, int* fatal)
{
    int err = 0;

    int next_protocol;

    do {
        UDA_LOG(UDA_LOG_DEBUG, "Start of Server Wait Loop\n");

        // Create a new userdefinedtypelist for the request by copying the parseduserdefinedtypelist structure
        //copyUserDefinedTypeList(&userdefinedtypelist);

        getInitialUserDefinedTypeList(&userdefinedtypelist);
        parseduserdefinedtypelist = *userdefinedtypelist;
        printUserDefinedTypeList(*userdefinedtypelist);

        logmalloclist = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
        initLogMallocList(logmalloclist);

        int server_closedown = 0;
        err = handleRequest(request_block, client_block, server_block, metadata_block, actions_desc, actions_sig,
                            data_block_list, fatal, &server_closedown);

        if (server_closedown) {
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Handle Request Error: %d [%d]\n", err, *fatal);

        err = reportToClient(server_block, data_block_list, client_block, err, metadata_block);

        UDA_LOG(UDA_LOG_DEBUG, "Data structures sent to client\n");
        UDA_LOG(UDA_LOG_DEBUG, "Report To Client Error: %d [%d]\n", err, *fatal);

        udaAccessLog(FALSE, *client_block, *request_block, *server_block, &pluginList, getServerEnvironment());

        err = 0;
        next_protocol = PROTOCOL_SLEEP;
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "freeUserDefinedTypeList\n");
        freeUserDefinedTypeList(userdefinedtypelist);
        userdefinedtypelist = nullptr;

        freeMallocLogList(logmalloclist);
        free(logmalloclist);
        logmalloclist = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlockList\n");
        freeDataBlockList(data_block_list);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(actions_desc);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(actions_sig);

        freeRequestBlock(request_block);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concatUdaError\n");
        concatUdaError(&server_block->idamerrorstack);        // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "closeUdaError\n");
        closeUdaError();

        UDA_LOG(UDA_LOG_DEBUG, "udaErrorLog\n");
        udaErrorLog(*client_block, *request_block, &server_block->idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "closeUdaError\n");
        closeUdaError();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock\n");
        initServerBlock(server_block, serverVersion);

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == PROTOCOL_SLEEP && !*fatal);

    return err;
}

int doServerClosedown(CLIENT_BLOCK* client_block, REQUEST_BLOCK* request_block, DATA_BLOCK_LIST* data_block_list)
{
    //----------------------------------------------------------------------------
    // Server Destruct.....

    UDA_LOG(UDA_LOG_DEBUG, "Server Shuting Down\n");
    if (server_tot_block_time > 1000 * server_timeout) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Timeout after %d secs\n", server_timeout);
    }

    //----------------------------------------------------------------------------
    // Write the Error Log Record & Free Error Stack Heap

    udaErrorLog(*client_block, *request_block, nullptr);
    closeUdaError();

    //----------------------------------------------------------------------------
    // Free Data Block Heap Memory in case by-passed

    freeDataBlockList(data_block_list);

    //----------------------------------------------------------------------------
    // Free Structure Definition List (don't free the structure as stack variable)

    //freeUserDefinedTypeList(&parseduserdefinedtypelist);

    //----------------------------------------------------------------------------
    // Free Plugin List and Close all open library entries

    freePluginList(&pluginList);

    freeMallocLogList(logmalloclist);
    free(logmalloclist);
    logmalloclist = nullptr;

    //----------------------------------------------------------------------------
    // Close the Logs

    fflush(nullptr);

    udaCloseLogging();

    //----------------------------------------------------------------------------
    // Close the SSL binding and context

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    closeUdaServerSSL();
#endif

    //----------------------------------------------------------------------------
    // Close the Socket Connections to Other Data Servers

    closeServerSockets(&socket_list);

    //----------------------------------------------------------------------------
    // Wait for client to receive returned server state

    return 0;
}

#ifdef SECURITYENABLED
int authenticateClient(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block)
{
    static int authenticationNeeded = 1;            // No data access until this is set TRUE

    initClientBlock(client_block, 0, "");

    if (authenticationNeeded && protocolVersion >= UDA_SECURITY_VERSION) {
        // User or intermediate server Must Authenticate
        // If the request is passed on through a chain of servers, user#2 authentication occurs within the external server.
        // An authentication structure is passed back from the server to the client

        // Receive the client_block
        // Test data validity of certificate
        // Test certificate signature						=> has a valid certificate (not proof of authentication)
        // Decrypt token A with the server private key
        // Encrypt token A with the client public key
        // Generate new token B and encrypt with the client public key

        int err = 0;
        if ((err = serverAuthentication(client_block, server_block, SERVER_DECRYPT_CLIENT_TOKEN)) != 0) {
            THROW_ERROR(err, "Client or Server Authentication Failed #2");
        }

        if ((err = serverAuthentication(client_block, server_block, SERVER_ENCRYPT_CLIENT_TOKEN)) != 0) {
            THROW_ERROR(err, "Client or Server Authentication Failed #3");
        }

        // Send the server_block

        if ((err = serverAuthentication(client_block, server_block, SERVER_ISSUE_TOKEN)) != 0) {
            THROW_ERROR(err, "Client or Server Authentication Failed #4");
        }

        // Receive the client_block
        // Decrypt token B with the server private key				=> Proof Client has valid private key == client authenticated
        // Test token B identical to that sent in step 4
        // Generate a new token B and encrypt with the client public key	=> maintain mutual authentication
        // Send the server_block

        if ((err = serverAuthentication(client_block, server_block, SERVER_VERIFY_TOKEN)) != 0) {
            THROW_ERROR(err, "Client or Server Authentication Failed #7");
        }

        authenticationNeeded = 0;
    }

    return 0;
}
#endif

int handshakeClient(CLIENT_BLOCK* client_block, SERVER_BLOCK* server_block, int* server_closedown)
{
    // Exchange version details - once only

    initClientBlock(client_block, 0, "");

    // Receive the client block, respecting earlier protocol versions

    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Initial Client Block\n");

    int err = 0;

    if (!xdrrec_skiprecord(serverInput)) {
        err = PROTOCOL_ERROR_5;
        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_skiprecord error!\n");
        addIdamError(CODEERRORTYPE, __func__, err, "Protocol 5 Error (Client Block)");
    } else {

        int protocol_id = PROTOCOL_CLIENT_BLOCK;        // Recieve Client Block

        if ((err = protocol2(serverInput, protocol_id, XDR_RECEIVE, nullptr, logmalloclist, userdefinedtypelist,
                             client_block, protocolVersion)) != 0) {
            addIdamError(CODEERRORTYPE, __func__, err, "Protocol 10 Error (Client Block)");
            UDA_LOG(UDA_LOG_DEBUG, "protocol error! Client Block not received!\n");
        }

        if (err == 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Initial Client Block received\n");
            printClientBlock(*client_block);
        }

        // Test for an immediate CLOSEDOWN instruction

        if (client_block->timeout == 0 || client_block->clientFlags & CLIENTFLAG_CLOSEDOWN) {
            *server_closedown = 1;
            return err;
        }
    }

    if (err != 0) return err;

    // Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    // set in xdr_client

    //protocolVersion = serverVersion;
    //if(client_block.version < serverVersion) protocolVersion = client_block.version;
    //if(client_block.version < server_block.version) protocolVersion = client_block.version;

    // Send the server block

    UDA_LOG(UDA_LOG_DEBUG, "Sending Initial Server Block \n");
    printServerBlock(*server_block);

    int protocol_id = PROTOCOL_SERVER_BLOCK;        // Receive Server Block: Server Aknowledgement

    if ((err = protocol2(serverOutput, protocol_id, XDR_SEND, nullptr, logmalloclist, userdefinedtypelist,
                         server_block, protocolVersion)) != 0) {
        THROW_ERROR(err, "Protocol 11 Error (Server Block #1)");
    }

    if (!xdrrec_endofrecord(serverOutput, 1)) {    // Send data now
        THROW_ERROR(PROTOCOL_ERROR_7, "Protocol 7 Error (Server Block)");
    }

    UDA_LOG(UDA_LOG_DEBUG, "Initial Server Block sent without error\n");

    // If the protocol version is legacy (<=6), then divert full control to a legacy server

    if (client_block->version <= legacyServerVersion) {
        UDA_LOG(UDA_LOG_DEBUG, "Diverting to the Legacy Server\n");
        UDA_LOG(UDA_LOG_DEBUG, "Client protocol %d\n", client_block->version);
        return legacyServer(*client_block, &pluginList, logmalloclist, userdefinedtypelist, &socket_list,
                            protocolVersion);
    }

    return err;
}

int startupServer(SERVER_BLOCK* server_block)
{
    static int socket_list_initialised = 0;
    static int plugin_list_initialised = 0;

    //-------------------------------------------------------------------------
    // Create the Server Log Directory: Fatal Error if any Problem Opening a Log?

    if (startup() != 0) {
        int err = FATAL_ERROR_LOGS;
        addIdamError(CODEERRORTYPE, __func__, err, "Fatal Error Opening the Server Logs");
        concatUdaError(&server_block->idamerrorstack);
        initUdaErrorStack();
    }

    UDA_LOG(UDA_LOG_DEBUG, "New Server Instance\n");

    //-------------------------------------------------------------------------
    // Connect to the client with SSL (X509) authentication

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)

    // Create the SSL binding (on socket #0), the SSL context, and verify the client certificate
    // Identify the authenticated user for service authorisation

    putUdaServerSSLSocket(0);
    
    int err = 0;
    if ((err = startUdaServerSSL()) != 0) {
        return err;
    }

#endif

    //-------------------------------------------------------------------------
    // Create the XDR Record Streams

    CreateXDRStream();

    UDA_LOG(UDA_LOG_DEBUG, "XDR Streams Created\n");

    //-------------------------------------------------------------------------
    // Open and Initialise the Socket List (Once Only)

    if (!socket_list_initialised) {
        initSocketList(&socket_list);
        socket_list_initialised = 1;
    }

    //----------------------------------------------------------------------
    // Initialise General Structure Passing

    // this step needs doing once only - the first time a generalised user defined structure is encountered.
    // For FAT clients use a static state variable to prevent multiple parsing

/*
    if (!fileParsed) {
        fileParsed = 1;
        initUserDefinedTypeList(&parseduserdefinedtypelist);

        char* token = nullptr;
        if ((token = getenv("UDA_SARRAY_CONFIG")) == nullptr) {
            THROW_ERROR(999, "No Environment variable UDA_SARRAY_CONFIG");
        }

        UDA_LOG(UDA_LOG_DEBUG, "Parsing structure definition file: %s\n", token);
        parseIncludeFile(&parseduserdefinedtypelist, token); // file containing the SARRAY structure definition
        printUserDefinedTypeList(parseduserdefinedtypelist);
    }
*/

    userdefinedtypelist = nullptr;                                     // Startup State

    //----------------------------------------------------------------------
    // Initialise the Data Reader Plugin list

    if (!plugin_list_initialised) {
        pluginList.count = 0;
        initPluginList(&pluginList, getServerEnvironment());
        plugin_list_initialised = 1;

        UDA_LOG(UDA_LOG_INFO, "List of Plugins available\n");
        for (int i = 0; i < pluginList.count; i++) {
            UDA_LOG(UDA_LOG_INFO, "[%d] %d %s\n", i, pluginList.plugin[i].request, pluginList.plugin[i].format);
        }
    }

    //----------------------------------------------------------------------------
    // Server Information: Operating System Name - may limit types of data that can be received by the Client

    char* env = nullptr;

    if ((env = getenv("OSTYPE")) != nullptr) {
        strcpy(server_block->OSName, env);
    } else if ((env = getenv("UDA_SERVER_OS")) != nullptr) {
        strcpy(server_block->OSName, env);
    }

    // Server Configuration and Environment DOI

    if ((env = getenv("UDA_SERVER_DOI")) != nullptr) {
        strcpy(server_block->DOI, env);
    }

    return 0;
}
