#include "server.hpp"

#include "udaServer.h"

#include <string>
#include <unistd.h>

#include "clientserver/initStructs.h"
#include "server_environment.hpp"
#include "logging/logging.h"
#include "clientserver/errorLog.h"
#include "clientserver/udaErrors.h"
#include "clientserver/protocol.h"
#include "clientserver/xdrlib.h"
#include "clientserver/printStructs.h"
#include "logging/accessLog.h"
#include "server_plugin.h"
#include "server_processing.h"
#include "structures/struct.h"
#include "server_exceptions.h"

int udaServer(CLIENT_BLOCK client_block)
{
    try {
        uda::Server server;
        server.run();
    } catch (uda::server::Exception& ex) {
        return ex.code();
    }
    return 0;
}

void free_data_blocks(std::vector<DataBlock>& data_blocks)
{
    for (auto& data_block : data_blocks) {
        freeDataBlock(&data_block);
    }
    data_blocks.clear();
}

void close_sockets(std::vector<Sockets>& sockets)
{
    for (auto& socket : sockets) {
        if (socket.type == TYPE_UDA_SERVER) {
            close(socket.fh);
        }
    }
    sockets.clear();
}

void print_data_block_list(const std::vector<DATA_BLOCK>& data_blocks)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Blocks\n");
    UDA_LOG(UDA_LOG_DEBUG, "count        : %d\n", data_blocks.size());
    int i = 0;
    for (auto& data_block : data_blocks) {
        UDA_LOG(UDA_LOG_DEBUG, "block number : %d\n", i);
        printDataBlock(data_block);
        ++i;
    }
}

uda::Server::Server()
    : error_stack_{}
    , environment_{}
    , sockets_{}
{
    initServerBlock(&server_block_, ServerVersion);
    initActions(&actions_desc_);        // There may be a Sequence of Actions to Apply
    initActions(&actions_sig_);
    initRequestBlock(&request_block_);
    cache_ = cache::open_cache();
}

void uda::Server::start_logs()
{
    if (environment_->loglevel <= UDA_LOG_ACCESS) {
        char cmd[STRING_LENGTH];
        sprintf(cmd, "mkdir -p %s 2>/dev/null", environment_->logdir);
        if (system(cmd) != 0) {
            addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "mkdir command failed");
        }

        errno = 0;
        std::string log_file = std::string{ environment_->logdir } + "Access.log";
        FILE* accout = fopen(log_file.c_str(), environment_->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__ , errno, "Access Log: ");
            if (accout != nullptr) {
                fclose(accout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ACCESS, accout);
        }
    }

    if (environment_->loglevel <= UDA_LOG_ERROR) {
        errno = 0;
        std::string log_file = std::string{ environment_->logdir } + "Error.log";
        FILE* errout = fopen(log_file.c_str(), environment_->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "Error Log: ");
            if (errout != nullptr) {
                fclose(errout);
            }
        } else {
            udaSetLogFile(UDA_LOG_ERROR, errout);
        }
    }

    if (environment_->loglevel <= UDA_LOG_WARN) {
        errno = 0;
        std::string log_file = std::string{ environment_->logdir } + "DebugServer.log";
        FILE* dbgout = fopen(log_file.c_str(), environment_->logmode);

        if (errno != 0) {
            addIdamError(UDA_SYSTEM_ERROR_TYPE, __func__, errno, "Debug Log: ");
            if (dbgout != nullptr) {
                fclose(dbgout);
            }
        } else {
            udaSetLogFile(UDA_LOG_WARN, dbgout);
            udaSetLogFile(UDA_LOG_DEBUG, dbgout);
            udaSetLogFile(UDA_LOG_INFO, dbgout);
        }
    }
}

void uda::Server::startup()
{
    udaSetLogLevel((LOG_LEVEL)environment_->loglevel);

    start_logs();

    environment_.print();

    UDA_LOG(UDA_LOG_DEBUG, "New Server Instance\n");

    //-------------------------------------------------------------------------
    // Create the XDR Record Streams
    protocol_.create();

    //-------------------------------------------------------------------------
    // Initialise the plugins
    plugins_.init();

    //----------------------------------------------------------------------------
    // Server Information: Operating System Name - may limit types of data that can be received by the Client

    char* env = nullptr;

    if ((env = getenv("OSTYPE")) != nullptr) {
        strcpy(server_block_.OSName, env);
    } else if ((env = getenv("UDA_SERVER_OS")) != nullptr) {
        strcpy(server_block_.OSName, env);
    }

    // Server Configuration and Environment DOI

    if ((env = getenv("UDA_SERVER_DOI")) != nullptr) {
        strcpy(server_block_.DOI, env);
    }
}

void uda::Server::run()
{
    startup();
    handshake_client();
    if (!server_closedown_) {
        loop();
    }
    close();
}

void uda::Server::close()
{
    //----------------------------------------------------------------------------
    // Server Destruct.....

    UDA_LOG(UDA_LOG_DEBUG, "Server Shutting Down\n");
    if (server_tot_block_time_ > 1000 * server_timeout_) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Timeout after %d secs\n", server_timeout_);
    }

    //----------------------------------------------------------------------------
    // Write the Error Log Record & Free Error Stack Heap

    udaErrorLog(client_block_, request_block_, nullptr);
    closeUdaError();

    //----------------------------------------------------------------------------
    // Free Data Block Heap Memory in case by-passed

    free_data_blocks(data_blocks_);

    //----------------------------------------------------------------------------
    // Free Structure Definition List (don't free the structure as stack variable)

    // freeUserDefinedTypeList(&parseduserdefinedtypelist);

    //----------------------------------------------------------------------------
    // Free Plugin List and Close all open library entries

    plugins_.close();

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

    close_sockets(sockets_);
}

void uda::Server::loop()
{
    int err = 0;

    int next_protocol;

    do {
        UDA_LOG(UDA_LOG_DEBUG, "Start of Server Wait Loop\n");

        // Create a new userdefinedtypelist for the request by copying the parseduserdefinedtypelist structure
        //copyUserDefinedTypeList(&userdefinedtypelist);

        getInitialUserDefinedTypeList(&user_defined_type_list_);
        parsed_user_defined_type_list_ = *user_defined_type_list_;
//        printUserDefinedTypeList(*user_defined_type_list_);

        log_malloc_list_ = (LOGMALLOCLIST*)malloc(sizeof(LOGMALLOCLIST));
        initLogMallocList(log_malloc_list_);

        server_closedown_ = false;

        err = handle_request();

        if (server_closedown_) {
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Handle Request Error: %d [%d]\n", err, (int)fatal_error_);

        err = report_to_client();

        UDA_LOG(UDA_LOG_DEBUG, "Data structures sent to client\n");
        UDA_LOG(UDA_LOG_DEBUG, "Report To Client Error: %d [%d]\n", err, (int)fatal_error_);

        udaAccessLog(FALSE, client_block_, request_block_, server_block_, total_datablock_size_);

        err = 0;
        next_protocol = UDA_PROTOCOL_SLEEP;
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol %d Received\n", next_protocol);

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "freeUserDefinedTypeList\n");
        freeUserDefinedTypeList(user_defined_type_list_);
        user_defined_type_list_ = nullptr;

        freeMallocLogList(log_malloc_list_);
        ::free(log_malloc_list_);
        log_malloc_list_ = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlockList\n");
        free_data_blocks(data_blocks_);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_desc_);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions\n");
        freeActions(&actions_sig_);

        freeRequestBlock(&request_block_);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concatUdaError\n");
        concatUdaError(&server_block_.idamerrorstack);        // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "closeUdaError\n");
        closeUdaError();

        UDA_LOG(UDA_LOG_DEBUG, "udaErrorLog\n");
        udaErrorLog(client_block_, request_block_, &server_block_.idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "closeUdaError\n");
        closeUdaError();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock\n");
        initServerBlock(&server_block_, ServerVersion);

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == UDA_PROTOCOL_SLEEP && !fatal_error_);
}

int uda::Server::handle_request()
{
    UDA_LOG(UDA_LOG_DEBUG, "Beginning request handling\n");

    //----------------------------------------------------------------------------
    // Client and Server States
    //
    // Errors: Fatal to Data Access
    //       Pass Back and Await Client Instruction

    int err = 0;

    initClientBlock(&client_block_, 0, "");

    err = protocol_.recv_client_block(server_block_, &client_block_, &fatal_error_, server_tot_block_time_, &server_timeout_,
                                log_malloc_list_, user_defined_type_list_);
    if (err != 0) {
        return err;
    }

    server_timeout_ = client_block_.timeout;                    // User specified Server Lifetime
    unsigned int private_flags = client_block_.privateFlags;    // Server to Server flags
    unsigned int clientFlags = client_block_.clientFlags;       // Client set flags
    int altRank = client_block_.altRank;                        // Rank of Alternative source

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket

    int protocol_version = ServerVersion;
    if (client_block_.version < ServerVersion) {
        protocol_version = client_block_.version;
    }
    protocol_.set_version(protocol_version);

    // The client request may originate from a server.
    // Is the Originating server an externally facing server? If so then switch to this mode: preserve local access policy

    if (!environment_->external_user && (private_flags & PRIVATEFLAG_EXTERNAL)) {
        environment_->external_user = 1;
    }

    UDA_LOG(UDA_LOG_DEBUG, "client version  %d\n", client_block_.version);
    UDA_LOG(UDA_LOG_DEBUG, "private_flags   %d\n", private_flags);
    UDA_LOG(UDA_LOG_DEBUG, "udaClientFlags  %d\n", clientFlags);
    UDA_LOG(UDA_LOG_DEBUG, "altRank         %d\n", altRank);
    UDA_LOG(UDA_LOG_DEBUG, "external?       %d\n", environment_->external_user);

    if (server_block_.idamerrorstack.nerrors > 0) {
        server_block_.error = server_block_.idamerrorstack.idamerror[0].code;
        strcpy(server_block_.msg, server_block_.idamerrorstack.idamerror[0].msg);
    }

    // Test the client version is compatible with this server version

    if (protocol_version > ServerVersion) {
        UDA_THROW_ERROR(999, "Protocol Error: Client API Version is Newer than the Server Version");
    }

    if (fatal_error_) {
        if (server_block_.idamerrorstack.nerrors > 0) {
            err = server_block_.idamerrorstack.idamerror[0].code;
        } else {
            err = 1;
        }
        return err;                // Manage the Fatal Server State
    }

    // Test for an immediate CLOSEDOWN instruction

    if (client_block_.timeout == 0 || (client_block_.clientFlags & CLIENTFLAG_CLOSEDOWN)) {
        server_closedown_ = true;
        return err;
    }

    //-------------------------------------------------------------------------
    // Client Request
    //
    // Errors: Fatal to Data Access
    //       Pass Back and Await Client Instruction

    err = protocol_.recv_request_block(&request_block_, log_malloc_list_, user_defined_type_list_);
    if (err != 0) {
        return err;
    }

    printClientBlock(client_block_);
    printServerBlock(server_block_);
    printRequestBlock(request_block_);

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

    if (strlen(request_block_.source) >=
        (STRING_LENGTH - 1 - strlen(proxyName) - strlen(environment_->server_proxy) - strlen(request_block_.api_delim))) {
        THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
    }

    // Prepend the client request and test for a redirection request via the proxy's plugin

    if (request_block_.api_delim[0] != '\0') {
        sprintf(work, "%s%s", proxyName, request_block_.api_delim);
    } else {
        sprintf(work, "%s%s", proxyName, environment_->api_delim);
    }

    if (strncasecmp(request_block_.source, work, strlen(work)) != 0) {
        // Not a recognised redirection so prepending is necessary

        // Has a proxy host been specified in the server startup script? If not assume the plugin has a default host and port

        if (environment_->server_proxy[0] == '\0') {
            if (request_block_.api_delim[0] != '\0') {
                sprintf(work, "%s%s%s", proxyName, request_block_.api_delim, request_block_.source);
            } else {
                sprintf(work, "%s%s%s", proxyName, environment_->api_delim, request_block_.source);
            }

            strcpy(request_block_.source, work);

        } else {                                                // UDA::host.port/source

            // Check the Server Version is Compatible with the Originating client version ?

            if (client_block->version < 6) {
                THROW_ERROR(999, "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are never passed.
            // Primitive test as the same IP address can be mapped to different names!
            // Should pass on the number of redirections and cap the limit!

            if (environment_->server_this[0] != '\0') {
                if (request_block_.api_delim[0] != '\0') {
                    sprintf(work, "%s%s%s", proxyName, request_block_.api_delim, environment_->server_this);
                } else {
                    sprintf(work, "%s%s%s", proxyName, environment_->api_delim, environment_->server_this);
                }

                if (strstr(request_block_.source, work) != nullptr) {
                    THROW_ERROR(999, "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                }
            }

            // Prepend the redirection UDA server details and replace the original

            if (request_block_.source[0] == '/') {
                if (request_block_.api_delim[0] != '\0')
                    sprintf(work, "%s%s%s%s", proxyName, request_block_.api_delim, environment_->server_proxy, request_block_.source);
                else
                    sprintf(work, "%s%s%s%s", proxyName, environment_->api_delim, environment_->server_proxy, request_block_.source);
            } else {
                if (request_block_.api_delim[0] != '\0')
                    sprintf(work, "%s%s%s/%s", proxyName, request_block_.api_delim, environment_->server_proxy, request_block_.source);
                else
                    sprintf(work, "%s%s%s/%s", proxyName, environment_->api_delim, environment_->server_proxy, request_block_.source);
            }
            strcpy(request_block_.source, work);
        }

        UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s avoiding %s\n", environment_->server_proxy, environment_->server_this);
        UDA_LOG(UDA_LOG_DEBUG, "plugin: %s\n", proxyName);
        UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request_block_.source);
    }

# else

    for (int i = 0; i < request_block_.num_requests; ++i) {
        REQUEST_DATA* request = &request_block_.requests[0];

        char work[1024];
        if (request->api_delim[0] != '\0') {
            sprintf(work, "UDA%s", request->api_delim);
        } else {
            sprintf(work, "UDA%s", environment_->api_delim);
        }

        if (environment_->server_proxy[0] != '\0' && strncasecmp(request->source, work, strlen(work)) != 0) {

            // Check the Server Version is Compatible with the Originating client version ?

            if (client_block_.version < 6) {
                UDA_THROW_ERROR(999,
                                "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are never passed.

            if (request->api_delim[0] != '\0') {
                sprintf(work, "UDA%s%s", request->api_delim, environment_->server_this);
            } else {
                sprintf(work, "UDA%s%s", environment_->api_delim, environment_->server_this);
            }

            if (strstr(request->source, work) != nullptr) {
                UDA_THROW_ERROR(999,
                                "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
            }

            // Check string length compatibility

            if (strlen(request->source) >=
                (STRING_LENGTH - 1 - strlen(environment_->server_proxy) - 4 + strlen(request->api_delim))) {
                UDA_THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
            }

            // Prepend the redirection UDA server details

            if (request->api_delim[0] != '\0') {
                sprintf(work, "UDA%s%s/%s", request->api_delim, environment_->server_proxy, request->source);
            } else {
                sprintf(work, "UDA%s%s/%s", environment_->api_delim, environment_->server_proxy, request->source);
            }

            strcpy(request->source, work);

            UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to %s\n", environment_->server_proxy);
            UDA_LOG(UDA_LOG_DEBUG, "source: %s\n", request->source);
        }
    }
# endif

    //----------------------------------------------------------------------
    // Write to the Access Log

    udaAccessLog(TRUE, client_block_, request_block_, server_block_, total_datablock_size_);

    //----------------------------------------------------------------------
    // Initialise Data Structures

    initDataSource(&metadata_block_.data_source);
    initSignalDesc(&metadata_block_.signal_desc);
    initSignal(&metadata_block_.signal_rec);

    //----------------------------------------------------------------------------------------------
    // If this is a PUT request then receive the putData structure

    for (int i = 0; i < request_block_.num_requests; ++i) {
        REQUEST_DATA* request = &request_block_.requests[0];

        initPutDataBlockList(&request->putDataBlockList);

        if (request->put) {
            err = protocol_.recv_putdata_block_list(&request->putDataBlockList, log_malloc_list_, user_defined_type_list_);
            if (err != 0) {
                return err;
            }
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Accessing data.\n");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data plug-in to use
    // Decide on Authentication procedure

    for (int i = 0; i < request_block_.num_requests; ++i) {
        auto request = &request_block_.requests[i];
        if (protocol_version >= 6) {
            if ((err = uda::serverPlugin(request, &metadata_block_.data_source, &metadata_block_.signal_desc,
                                         plugins_, environment_.p_env())) != 0) {
                return err;
            }
        } else {
            throw uda::server::ProtocolVersionError{"Unsupported protocol version"};
        }
    }

    //------------------------------------------------------------------------------------------------
    // Query the Database: Internal or External Data Sources
    // Read the Data or Create the Composite/Derived Data
    // Apply XML Actions to Data

    int depth = 0;

    for (int i = 0; i < request_block_.num_requests; ++i) {
        auto request = &request_block_.requests[i];

        auto cache_block = protocol_.read_from_cache(cache_, request, environment_, log_malloc_list_, user_defined_type_list_);
        if (cache_block != nullptr) {
            data_blocks_.push_back(*cache_block);
            continue;
        }

        data_blocks_.push_back({});
        DATA_BLOCK* data_block = &data_blocks_.back();

        err = get_data(&depth, request, data_block, protocol_version);

        protocol_.write_to_cache(cache_, request, environment_, data_block, log_malloc_list_, user_defined_type_list_);
    }

    for (int i = 0; i < request_block_.num_requests; ++i) {
        request_block_.requests[i].function[0] = '\0';
    }

    DATA_SOURCE* data_source = &metadata_block_.data_source;
    SIGNAL_DESC* signal_desc = &metadata_block_.signal_desc;
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : %s \n", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : %s \n", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : %s \n", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : %s \n", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : %s \n", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : %d \n", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : %d \n", data_source->pass);
    UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : %d \n", depth);
    printRequestBlock(request_block_);
    printDataSource(*data_source);
    printSignal(metadata_block_.signal_rec);
    printSignalDesc(*signal_desc);
    print_data_block_list(data_blocks_);
    printIdamErrorStack();
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");

    if (err != 0) return err;

    //------------------------------------------------------------------------------------------------
    // Server-Side Data Processing

    if (client_block_.get_dimdble || client_block_.get_timedble || client_block_.get_scalar) {
        for (auto& data_block : data_blocks_) {
            if (uda::serverProcessing(client_block_, &data_block) != 0) {
                UDA_THROW_ERROR(779, "Server-Side Processing Error");
            }
        }
    }

    //----------------------------------------------------------------------------
    // Check the Client can receive the data type: Version dependent
    // Otherwise inform the client via the server state block

    for (auto& data_block : data_blocks_) {

        if (protocol_version < 6 && data_block.data_type == UDA_TYPE_STRING) {
            data_block.data_type = UDA_TYPE_CHAR;
        }

        if (data_block.data_n > 0 &&
            (protocolVersionTypeTest(protocol_version, data_block.data_type) ||
             protocolVersionTypeTest(protocol_version, data_block.error_type))) {
            UDA_THROW_ERROR(999,
                            "The Data has a type that cannot be passed to the Client: A newer client library version is required.");
        }

        if (data_block.rank > 0) {
            DIMS dim;
            for (unsigned int j = 0; j < data_block.rank; j++) {
                dim = data_block.dims[j];
                if (protocolVersionTypeTest(protocol_version, dim.data_type) ||
                    protocolVersionTypeTest(protocol_version, dim.error_type)) {
                    UDA_THROW_ERROR(999,
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

unsigned int count_data_block_list_size(const std::vector<DataBlock>& data_blocks, ClientBlock* client_block)
{
    unsigned int total = 0;
    for (const auto& data_block : data_blocks) {
        total += countDataBlockSize(&data_block, client_block);
    }
    return total;
}

void print_data_blocks(const std::vector<DataBlock>& data_blocks)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Blocks\n");
    UDA_LOG(UDA_LOG_DEBUG, "count        : %d\n", data_blocks.size());
    int i = 0;
    for (const auto& data_block : data_blocks) {
        UDA_LOG(UDA_LOG_DEBUG, "block number : %d\n", i);
        printDataBlock(data_block);
        ++i;
    }
}

int uda::Server::report_to_client()
{
    //----------------------------------------------------------------------------
    // Gather Server Error State

    // Update Server State with Error Stack
    concatUdaError(&server_block_.idamerrorstack);
    closeUdaError();

    int err = 0;

    if (server_block_.idamerrorstack.nerrors > 0) {
        server_block_.error = server_block_.idamerrorstack.idamerror[0].code;
        strcpy(server_block_.msg, server_block_.idamerrorstack.idamerror[0].msg);
    }

    //------------------------------------------------------------------------------------------------
    // How much data to be sent?

    total_datablock_size_ = count_data_block_list_size(data_blocks_, &client_block_);

    printServerBlock(server_block_);

    //------------------------------------------------------------------------------------------------
    // Send the server block and all data in a single (minimal number) tcp packet

    err = protocol_.send_server_block(server_block_, log_malloc_list_, user_defined_type_list_);

    if (server_block_.idamerrorstack.nerrors > 0) {
        err = server_block_.idamerrorstack.idamerror[0].code;
    }

    if (err != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop\n");

        // Send the Server Block and bypass sending data (there are none!)
        int rc = protocol_.flush();
        if (rc != 0) {
            return rc;
        }

        return err;
    }

    if (client_block_.get_meta) {
        total_datablock_size_ +=
                sizeof(DATA_SYSTEM) + sizeof(SYSTEM_CONFIG) + sizeof(DATA_SOURCE) + sizeof(SIGNAL) +
                sizeof(SIGNAL_DESC);

        err = protocol_.send_meta_data(metadata_block_, log_malloc_list_, user_defined_type_list_);
        if (err != 0) {
            return err;
        }
    } // End of Database Meta Data

    //----------------------------------------------------------------------------
    // Send the Data

    print_data_blocks(data_blocks_);
    err = protocol_.send_data_blocks(data_blocks_, log_malloc_list_, user_defined_type_list_);
    if (err != 0) {
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the data in a single full TCP packet

    protocol_.flush();

    //------------------------------------------------------------------------------
    // Legacy Hierarchical Data Structures

    for (const auto& data_block : data_blocks_) {
        err = protocol_.send_hierachical_data(data_block, log_malloc_list_, user_defined_type_list_);
        if (err != 0) {
            return err;
        }
    }

    return err;
}

void uda::Server::handshake_client()
{
    // Exchange version details - once only

    initClientBlock(&client_block_, 0, "");

    // Receive the client block, respecting earlier protocol versions

    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Initial Client Block\n");

    int err = 0;

    err = protocol_.read_client_block(&client_block_, log_malloc_list_, user_defined_type_list_);

    if (client_block_.timeout == 0 || client_block_.clientFlags & CLIENTFLAG_CLOSEDOWN) {
        server_closedown_ = true;
        return;
    }

    if (err != 0) {
        return;
    }

    // Flush (mark as at EOF) the input socket buffer (not all client state data may have been read - version dependent)

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    // set in xdr_client

    //protocolVersion = serverVersion;
    //if(client_block.version < serverVersion) protocolVersion = client_block.version;
    //if(client_block.version < server_block.version) protocolVersion = client_block.version;

    // Send the server block

    err = protocol_.send_server_block(server_block_, log_malloc_list_, user_defined_type_list_);
    if (err != 0) {
        throw server::ProtocolError{"Failed to send server block"};
    }

    err = protocol_.flush();
    if (err != 0) {
        throw server::ProtocolError{"Failed to flush protocol"};
    }

    // If the protocol version is legacy (<=6), then divert full control to a legacy server

    if (client_block_.version <= LegacyServerVersion) {
        throw server::ProtocolVersionError{"Unsupported protocol version"};
    }
}
