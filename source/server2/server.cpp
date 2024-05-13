#include "server.hpp"

#include <fmt/format.h>
#include <string>
#include <uda/structured.h>
#include <unistd.h>
#include <filesystem>

#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/printStructs.h"
#include "clientserver/protocol.h"
#include "clientserver/xdrlib.h"
#include "logging/accessLog.h"
#include "logging/logging.h"
#include "server_exceptions.h"
#include "server_plugin.h"
#include "server_processing.h"
#include "structures/struct.h"
#include "config/config.h"

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
#  include "authentication/udaServerSSL.h"
using namespace uda::authentication;
#endif

using namespace std::string_literals;

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::config;

unsigned int count_data_block_size(const DataBlock& data_block, ClientBlock* client_block) {
    int factor;
    Dims dim;
    unsigned int count = sizeof(DataBlock);

    count += (unsigned int) (getSizeOf((UDA_TYPE) data_block.data_type) * data_block.data_n);

    if (data_block.error_type != UDA_TYPE_UNKNOWN) {
        count += (unsigned int) (getSizeOf((UDA_TYPE) data_block.error_type) * data_block.data_n);
    }
    if (data_block.errasymmetry) {
        count += (unsigned int) (getSizeOf((UDA_TYPE) data_block.error_type) * data_block.data_n);
    }

    if (data_block.rank > 0) {
        for (unsigned int k = 0; k < data_block.rank; k++) {
            count += sizeof(Dims);
            dim = data_block.dims[k];
            if (!dim.compressed) {
                count += (unsigned int) (getSizeOf((UDA_TYPE) dim.data_type) * dim.dim_n);
                factor = 1;
                if (dim.errasymmetry) {
                    factor = 2;
                }
                if (dim.error_type != UDA_TYPE_UNKNOWN) {
                    count += (unsigned int) (factor * getSizeOf((UDA_TYPE) dim.error_type) * dim.dim_n);
                }
            } else {
                switch (dim.method) {
                    case 0:
                        count += +2 * sizeof(double);
                        break;
                    case 1:
                        for (unsigned int i = 0; i < dim.udoms; i++) {
                            count += (unsigned int) (*((long*) dim.sams + i) * getSizeOf((UDA_TYPE) dim.data_type));
                        }
                        break;
                    case 2:
                        count += dim.udoms * getSizeOf((UDA_TYPE) dim.data_type);
                        break;
                    case 3:
                        count += dim.udoms * getSizeOf((UDA_TYPE) dim.data_type);
                        break;
                }
            }
        }
    }

    if (client_block->get_meta) {
        count += sizeof(DataSystem) + sizeof(SystemConfig) + sizeof(DataSource) + sizeof(Signal) + sizeof(SignalDesc);
    }

    return count;
}

unsigned int count_data_block_list_size(const std::vector<uda::client_server::DataBlock>& data_block_list, ClientBlock* client_block) {
    unsigned int total = 0;
    for (const auto& data_block : data_block_list) {
        total += count_data_block_size(data_block, client_block);
    }
    return total;
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

void print_data_block_list(const std::vector<DataBlock>& data_blocks)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Blocks");
    UDA_LOG(UDA_LOG_DEBUG, "count        : {}", data_blocks.size());
    int i = 0;
    for (auto& data_block : data_blocks) {
        UDA_LOG(UDA_LOG_DEBUG, "block number : {}", i);
        print_data_block(data_block);
        ++i;
    }
}

uda::server::Server::Server(Config config)
    : _config{std::move(config)}, _error_stack{}, _sockets{}, _plugins{_config}
{
    init_server_block(&_server_block, ServerVersion);
    init_actions(&_actions_desc); // There may be a Sequence of Actions to Apply
    init_actions(&_actions_sig);
    init_request_block(&_request_block);
    _cache = cache::open_cache();
}

void uda::server::Server::start_logs()
{
    auto log_level = (LogLevel)_config.get("logging.level").as_or_default((int)UDA_LOG_NONE);
    auto log_dir = _config.get("logging.path").as_or_default(""s);
    auto log_mode = _config.get("logging.mode").as_or_default("a"s);

    if (log_level <= UDA_LOG_ACCESS) {
        if (!log_dir.empty()) {
            std::string cmd = fmt::format("mkdir -p {} 2>/dev/null", log_dir);
            if (system(cmd.c_str()) != 0) {
                add_error(UDA_CODE_ERROR_TYPE, __func__, 999, "mkdir command failed");
                throw uda::server::StartupException("mkdir command failed");
            }
        }

        errno = 0;
        auto log_file = std::filesystem::path{log_dir} / "Access.log";
        set_log_file(UDA_LOG_ACCESS, log_file, log_mode);
    }

    if (log_level <= UDA_LOG_ERROR) {
        errno = 0;
        auto log_file = std::filesystem::path{log_dir} / "Error.log";
        set_log_file(UDA_LOG_ERROR, log_file, log_mode);
    }

    if (log_level <= UDA_LOG_WARN) {
        errno = 0;
        auto log_file = std::filesystem::path{log_dir} / "DebugServer.log";
        set_log_file(UDA_LOG_WARN, log_file, log_mode);
        set_log_file(UDA_LOG_DEBUG, log_file, log_mode);
        set_log_file(UDA_LOG_INFO, log_file, log_mode);
    }
}

void uda::server::Server::startup()
{
    auto log_level = (LogLevel)_config.get("logging.level").as_or_default((int)UDA_LOG_NONE);

    init_logging();
    set_log_level((LogLevel) log_level);

    start_logs();

    _config.print();

    UDA_LOG(UDA_LOG_DEBUG, "New Server Instance");

    //-------------------------------------------------------------------------
    // Create the XDR Record Streams
    _protocol.create();

    //-------------------------------------------------------------------------
    // Initialise the plugins
    _plugins.init();

    //----------------------------------------------------------------------------
    // Server Information: Operating System Name - may limit types of data that can be received by the Client

    auto os = _config.get("server.os");
    if (os) {
        strcpy(_server_block.OSName, os.as<std::string>().c_str());
    }

    // Server Configuration and Environment DOI

    auto doi = _config.get("server.doi");
    if (doi) {
        strcpy(_server_block.DOI, doi.as<std::string>().c_str());
    }
}

void uda::server::Server::run()
{
    startup();
    handshake_client();
    if (!_server_closedown) {
        loop();
    }
    close();
}

void uda::server::Server::close()
{
    //----------------------------------------------------------------------------
    // Server Destruct.....

    UDA_LOG(UDA_LOG_DEBUG, "Server Shutting Down");
    if (_server_tot_block_time > 1000 * _server_timeout) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Timeout after {} secs", _server_timeout);
    }

    //----------------------------------------------------------------------------
    // Write the Error Log Record & Free Error Stack Heap

    error_log(_client_block, _request_block, nullptr);
    close_error();

    //----------------------------------------------------------------------------
    // Free Data Block Heap Memory in case by-passed

    free_data_blocks(_data_blocks);

    //----------------------------------------------------------------------------
    // Free Structure Definition List (don't free the structure as stack variable)

    // udaFreeUserDefinedTypeList(&parseduserdefinedtypelist);

    //----------------------------------------------------------------------------
    // Free Plugin List and Close all open library entries

    _plugins.close();

    //----------------------------------------------------------------------------
    // Close the Logs

    fflush(nullptr);

    close_logging();

    //----------------------------------------------------------------------------
    // Close the SSL binding and context

#if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
    closeUdaServerSSL();
#endif

    //----------------------------------------------------------------------------
    // Close the Socket Connections to Other Data Servers

    close_sockets(_sockets);
}

void uda::server::Server::loop()
{
    int err = 0;

    int next_protocol;

    do {
        UDA_LOG(UDA_LOG_DEBUG, "Start of Server Wait Loop");

        // Create a new userdefinedtypelist for the request by copying the parseduserdefinedtypelist structure
        // copy_user_defined_type_list(&userdefinedtypelist);

        get_initial_user_defined_type_list(&_user_defined_type_list);
        _parsed_user_defined_type_list = *_user_defined_type_list;
        //        print_user_defined_type_list(*user_defined_type_list_);

        _log_malloc_list = (LogMallocList*)malloc(sizeof(LogMallocList));
        init_log_malloc_list(_log_malloc_list);

        _server_closedown = false;

        err = handle_request();

        if (_server_closedown) {
            break;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Handle Request Error: {} [{}]", err, (int)_fatal_error);

        err = report_to_client();

        UDA_LOG(UDA_LOG_DEBUG, "Data structures sent to client");
        UDA_LOG(UDA_LOG_DEBUG, "Report To Client Error: {} [{}]", err, (int)_fatal_error);

        uda_access_log(FALSE, _client_block, _request_block, _server_block, _total_data_block_size);

        err = 0;
        next_protocol = UDA_PROTOCOL_SLEEP;
        UDA_LOG(UDA_LOG_DEBUG, "Next Protocol {} Received", next_protocol);

        //----------------------------------------------------------------------------
        // Free Data Block Heap Memory

        UDA_LOG(UDA_LOG_DEBUG, "udaFreeUserDefinedTypeList");
        udaFreeUserDefinedTypeList(_user_defined_type_list);
        _user_defined_type_list = nullptr;

        udaFreeMallocLogList(_log_malloc_list);
        ::free(_log_malloc_list);
        _log_malloc_list = nullptr;

        UDA_LOG(UDA_LOG_DEBUG, "freeDataBlockList");
        free_data_blocks(_data_blocks);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions");
        free_actions(&_actions_desc);

        UDA_LOG(UDA_LOG_DEBUG, "freeActions");
        free_actions(&_actions_sig);

        freeRequestBlock(&_request_block);

        //----------------------------------------------------------------------------
        // Write the Error Log Record & Free Error Stack Heap

        UDA_LOG(UDA_LOG_DEBUG, "concat_error");
        concat_error(&_server_block.idamerrorstack); // Update Server State with Error Stack

        UDA_LOG(UDA_LOG_DEBUG, "close_error");
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "error_log");
        error_log(_client_block, _request_block, &_server_block.idamerrorstack);

        UDA_LOG(UDA_LOG_DEBUG, "close_error");
        close_error();

        UDA_LOG(UDA_LOG_DEBUG, "initServerBlock");
        init_server_block(&_server_block, ServerVersion);

        //----------------------------------------------------------------------------
        // Server Wait Loop

    } while (err == 0 && next_protocol == UDA_PROTOCOL_SLEEP && !_fatal_error);
}

int uda::server::Server::handle_request()
{
    UDA_LOG(UDA_LOG_DEBUG, "Beginning request handling");

    //----------------------------------------------------------------------------
    // Client and Server States
    //
    // Errors: Fatal to Data Access
    //       Pass Back and Await Client Instruction

    int err = 0;

    init_client_block(&_client_block, 0, "");

    err = _protocol.recv_client_block(_server_block, &_client_block, &_fatal_error, _server_tot_block_time,
                                      &_server_timeout, _log_malloc_list, _user_defined_type_list);
    if (err != 0) {
        return err;
    }

    _server_timeout = _client_block.timeout;                 // User specified Server Lifetime
    unsigned int private_flags = _client_block.privateFlags; // Server to Server flags
    unsigned int clientFlags = _client_block.clientFlags;    // Client set flags
    int altRank = _client_block.altRank;                     // Rank of Alternative source

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket

    int protocol_version = ServerVersion;
    if (_client_block.version < ServerVersion) {
        protocol_version = _client_block.version;
    }
    _protocol.set_version(protocol_version);

    // The client request may originate from a server.
    // Is the Originating server an externally facing server? If so then switch to this mode: preserve local access
    // policy

    auto external_user = bool(_config.get("server.external_user"));
    if (!external_user && (private_flags & PRIVATEFLAG_EXTERNAL)) {
        _config.set("server.external_user", true);
        external_user = true;
    }

    UDA_LOG(UDA_LOG_DEBUG, "client version  {}", _client_block.version);
    UDA_LOG(UDA_LOG_DEBUG, "private_flags   {}", private_flags);
    UDA_LOG(UDA_LOG_DEBUG, "udaClientFlags  {}", clientFlags);
    UDA_LOG(UDA_LOG_DEBUG, "altRank         {}", altRank);
    UDA_LOG(UDA_LOG_DEBUG, "external?       {}", external_user);

    if (_server_block.idamerrorstack.nerrors > 0) {
        _server_block.error = _server_block.idamerrorstack.idamerror[0].code;
        strcpy(_server_block.msg, _server_block.idamerrorstack.idamerror[0].msg);
    }

    // Test the client version is compatible with this server version

    if (protocol_version > ServerVersion) {
        UDA_THROW_ERROR(999, "Protocol Error: Client API Version is Newer than the Server Version");
    }

    if (_fatal_error) {
        if (_server_block.idamerrorstack.nerrors > 0) {
            err = _server_block.idamerrorstack.idamerror[0].code;
        } else {
            err = 1;
        }
        return err; // Manage the Fatal Server State
    }

    // Test for an immediate CLOSEDOWN instruction

    if (_client_block.timeout == 0 || (_client_block.clientFlags & CLIENTFLAG_CLOSEDOWN)) {
        _server_closedown = true;
        return err;
    }

    //-------------------------------------------------------------------------
    // Client Request
    //
    // Errors: Fatal to Data Access
    //       Pass Back and Await Client Instruction

    err = _protocol.recv_request_block(&_request_block, _log_malloc_list, _user_defined_type_list);
    if (err != 0) {
        return err;
    }

    print_client_block(_client_block);
    print_server_block(_server_block);
    print_request_block(_request_block);

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

#ifdef PROXYSERVER

    // Name of the Proxy plugin
    
    std::string proxy_name = "UDA";

    auto proxy_plugin = _config.get("server.proxy_plugin");
    if (proxy_plugin) {
        proxy_name = proxy_plugin.as<std::string>();
    }

    // Check string length compatibility

    if (strlen(request_block_.source) >= (STRING_LENGTH - 1 - proxy_name.size() - strlen(environment_->server_proxy) -
                                          strlen(request_block_.api_delim))) {
        UDA_THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
    }

    // Prepend the client request and test for a redirection request via the proxy's plugin

    char work[STRING_LENGTH];

    if (request_block_.api_delim[0] != '\0') {
        sprintf(work, "%s%s", proxy_name.c_str(), request_block_.api_delim);
    } else {
        sprintf(work, "%s%s", proxy_name.c_str(), environment_->api_delim);
    }

    if (strncasecmp(request_block_.source, work, strlen(work)) != 0) {
        // Not a recognised redirection so prepending is necessary

        // Has a proxy host been specified in the server startup script? If not assume the plugin has a default host and
        // port

        if (environment_->server_proxy[0] == '\0') {
            if (request_block_.api_delim[0] != '\0') {
                sprintf(work, "%s%s%s", proxy_name.c_str(), request_block_.api_delim, request_block_.source);
            } else {
                sprintf(work, "%s%s%s", proxy_name.c_str(), environment_->api_delim, request_block_.source);
            }

            strcpy(request_block_.source, work);

        } else { // UDA::host.port/source

            // Check the Server Version is Compatible with the Originating client version ?

            if (client_block->version < 6) {
                UDA_THROW_ERROR(
                    999,
                    "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are
            // never passed. Primitive test as the same IP address can be mapped to different names! Should pass on the
            // number of redirections and cap the limit!

            if (environment_->server_this[0] != '\0') {
                if (request_block_.api_delim[0] != '\0') {
                    sprintf(work, "%s%s%s", proxy_name.c_str(), request_block_.api_delim, environment_->server_this);
                } else {
                    sprintf(work, "%s%s%s", proxy_name.c_str(), environment_->api_delim, environment_->server_this);
                }

                if (strstr(request_block_.source, work) != nullptr) {
                    UDA_THROW_ERROR(
                        999,
                        "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
                }
            }

            // Prepend the redirection UDA server details and replace the original

            if (request_block_.source[0] == '/') {
                if (request_block_.api_delim[0] != '\0') {
                    sprintf(work, "%s%s%s%s", proxy_name.c_str(), request_block_.api_delim, environment_->server_proxy,
                            request_block_.source);
                } else {
                    sprintf(work, "%s%s%s%s", proxy_name.c_str(), environment_->api_delim, environment_->server_proxy,
                            request_block_.source);
                }
            } else {
                if (request_block_.api_delim[0] != '\0') {
                    sprintf(work, "%s%s%s/%s", proxy_name.c_str(), request_block_.api_delim, environment_->server_proxy,
                            request_block_.source);
                } else {
                    sprintf(work, "%s%s%s/%s", proxy_name.c_str(), environment_->api_delim, environment_->server_proxy,
                            request_block_.source);
                }
            }
            strcpy(request_block_.source, work);
        }

        UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to {} avoiding {}", environment_->server_proxy,
                environment_->server_this);
        UDA_LOG(UDA_LOG_DEBUG, "plugin: {}", proxy_name.c_str());
        UDA_LOG(UDA_LOG_DEBUG, "source: {}", request_block_.source);
    }

#else

    auto delim = _config.get("request.delim").as_or_default(""s);
    auto proxy_target = _config.get("server.proxy_target").as_or_default(""s);
    auto server = _config.get("server.address").as_or_default(""s);

    for (int i = 0; i < _request_block.num_requests; ++i) {
        RequestData* request = &_request_block.requests[0];

        char work[STRING_LENGTH];
        if (request->api_delim[0] != '\0') {
            snprintf(work, STRING_LENGTH, "UDA%s", request->api_delim);
        } else {
            snprintf(work, STRING_LENGTH, "UDA%s", delim.c_str());
        }

        if (!proxy_target.empty() && strncasecmp(request->source, work, strlen(work)) != 0) {

            // Check the Server Version is Compatible with the Originating client version ?

            if (_client_block.version < 6) {
                UDA_THROW_ERROR(
                    999,
                    "PROXY redirection: Originating Client Version not compatible with the PROXY server interface.");
            }

            // Test for Proxy calling itself indirectly => potential infinite loop
            // The UDA Plugin strips out the host and port data from the source so the originating server details are
            // never passed.

            if (request->api_delim[0] != '\0') {
                snprintf(work, STRING_LENGTH, "UDA%s%s", request->api_delim, server.c_str());
            } else {
                snprintf(work, STRING_LENGTH, "UDA%s%s", delim.c_str(), server.c_str());
            }

            if (strstr(request->source, work) != nullptr) {
                UDA_THROW_ERROR(
                    999, "PROXY redirection: The PROXY is calling itself - Recursive server calls are not advisable!");
            }

            // Check string length compatibility

            if (strlen(request->source) >=
                (STRING_LENGTH - 1 - proxy_target.size() - 4 + strlen(request->api_delim))) {
                UDA_THROW_ERROR(999, "PROXY redirection: The source argument string is too long!");
            }

            // Prepend the redirection UDA server details

            if (request->api_delim[0] != '\0') {
                snprintf(work, STRING_LENGTH, "UDA%s%s/%s", request->api_delim, proxy_target.c_str(),
                         request->source);
            } else {
                snprintf(work, STRING_LENGTH, "UDA%s%s/%s", delim.c_str(), proxy_target.c_str(),
                         request->source);
            }

            strcpy(request->source, work);

            UDA_LOG(UDA_LOG_DEBUG, "PROXY Redirection to {}", proxy_target.c_str());
            UDA_LOG(UDA_LOG_DEBUG, "source: {}", request->source);
        }
    }
#endif

    //----------------------------------------------------------------------
    // Write to the Access Log

    uda_access_log(TRUE, _client_block, _request_block, _server_block, _total_data_block_size);

    //----------------------------------------------------------------------
    // Initialise Data Structures

    init_data_source(&_metadata_block.data_source);
    init_signal_desc(&_metadata_block.signal_desc);
    init_signal(&_metadata_block.signal_rec);

    //----------------------------------------------------------------------------------------------
    // If this is a PUT request then receive the putData structure

    for (int i = 0; i < _request_block.num_requests; ++i) {
        RequestData* request = &_request_block.requests[0];

        init_put_data_block_list(&request->putDataBlockList);

        if (request->put) {
            err = _protocol.recv_putdata_block_list(&request->putDataBlockList, _log_malloc_list,
                                                    _user_defined_type_list);
            if (err != 0) {
                return err;
            }
        }
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Accessing data.");

    //----------------------------------------------------------------------------------------------
    // Decode the API Arguments: determine appropriate data plug-in to use
    // Decide on Authentication procedure

    for (int i = 0; i < _request_block.num_requests; ++i) {
        auto request = &_request_block.requests[i];
        if (protocol_version >= 6) {
            if ((err = server_plugin(_config, request, &_metadata_block.data_source, &_metadata_block.signal_desc, _plugins)) != 0) {
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

    for (int i = 0; i < _request_block.num_requests; ++i) {
        auto request = &_request_block.requests[i];

        auto cache_block =
            _protocol.read_from_cache(_config, _cache, request, _log_malloc_list, _user_defined_type_list);
        if (cache_block != nullptr) {
            _data_blocks.push_back(*cache_block);
            continue;
        }

        _data_blocks.push_back({});
        DataBlock* data_block = &_data_blocks.back();

        err = get_data(&depth, request, data_block, protocol_version);

        _protocol.write_to_cache(_config, _cache, request, data_block, _log_malloc_list, _user_defined_type_list);
    }

    for (int i = 0; i < _request_block.num_requests; ++i) {
        _request_block.requests[i].function[0] = '\0';
    }

    DataSource* data_source = &_metadata_block.data_source;
    SignalDesc* signal_desc = &_metadata_block.signal_desc;
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");
    UDA_LOG(UDA_LOG_DEBUG, "Archive      : {} ", data_source->archive);
    UDA_LOG(UDA_LOG_DEBUG, "Device Name  : {} ", data_source->device_name);
    UDA_LOG(UDA_LOG_DEBUG, "Signal Name  : {} ", signal_desc->signal_name);
    UDA_LOG(UDA_LOG_DEBUG, "File Path    : {} ", data_source->path);
    UDA_LOG(UDA_LOG_DEBUG, "File Name    : {} ", data_source->filename);
    UDA_LOG(UDA_LOG_DEBUG, "Pulse Number : {} ", data_source->exp_number);
    UDA_LOG(UDA_LOG_DEBUG, "Pass Number  : {} ", data_source->pass);
    UDA_LOG(UDA_LOG_DEBUG, "Recursive #  : {} ", depth);
    print_request_block(_request_block);
    print_data_source(*data_source);
    print_signal(_metadata_block.signal_rec);
    print_signal_desc(*signal_desc);
    print_data_block_list(_data_blocks);
    print_error_stack();
    UDA_LOG(UDA_LOG_DEBUG,
            "======================== ******************** ==========================================\n");

    if (err != 0) {
        return err;
    }

    //------------------------------------------------------------------------------------------------
    // Server-Side Data Processing

    if (_client_block.get_dimdble || _client_block.get_timedble || _client_block.get_scalar) {
        for (auto& data_block : _data_blocks) {
            if (server_processing(_client_block, &data_block) != 0) {
                UDA_THROW_ERROR(779, "Server-Side Processing Error");
            }
        }
    }

    //----------------------------------------------------------------------------
    // Check the Client can receive the data type: Version dependent
    // Otherwise inform the client via the server state block

    for (auto& data_block : _data_blocks) {

        if (protocol_version < 6 && data_block.data_type == UDA_TYPE_STRING) {
            data_block.data_type = UDA_TYPE_CHAR;
        }

        if (data_block.data_n > 0 && (protocol_version_type_test(protocol_version, data_block.data_type) ||
                                      protocol_version_type_test(protocol_version, data_block.error_type))) {
            UDA_THROW_ERROR(
                999,
                "The Data has a type that cannot be passed to the Client: A newer client library version is required.");
        }

        if (data_block.rank > 0) {
            Dims dim;
            for (unsigned int j = 0; j < data_block.rank; j++) {
                dim = data_block.dims[j];
                if (protocol_version_type_test(protocol_version, dim.data_type) ||
                    protocol_version_type_test(protocol_version, dim.error_type)) {
                    UDA_THROW_ERROR(999, "A Coordinate Data has a numerical type that cannot be passed to the Client: "
                                         "A newer client library version is required.");
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

void print_data_blocks(const std::vector<DataBlock>& data_blocks)
{
    UDA_LOG(UDA_LOG_DEBUG, "Data Blocks");
    UDA_LOG(UDA_LOG_DEBUG, "count        : {}", data_blocks.size());
    int i = 0;
    for (const auto& data_block : data_blocks) {
        UDA_LOG(UDA_LOG_DEBUG, "block number : {}", i);
        print_data_block(data_block);
        ++i;
    }
}

int uda::server::Server::report_to_client()
{
    //----------------------------------------------------------------------------
    // Gather Server Error State

    // Update Server State with Error Stack
    concat_error(&_server_block.idamerrorstack);
    close_error();

    int err = 0;

    if (_server_block.idamerrorstack.nerrors > 0) {
        _server_block.error = _server_block.idamerrorstack.idamerror[0].code;
        strcpy(_server_block.msg, _server_block.idamerrorstack.idamerror[0].msg);
    }

    //------------------------------------------------------------------------------------------------
    // How much data to be sent?

    _total_data_block_size = count_data_block_list_size(_data_blocks, &_client_block);

    print_server_block(_server_block);

    //------------------------------------------------------------------------------------------------
    // Send the server block and all data in a single (minimal number) tcp packet

    err = _protocol.send_server_block(_server_block, _log_malloc_list, _user_defined_type_list);

    if (_server_block.idamerrorstack.nerrors > 0) {
        err = _server_block.idamerrorstack.idamerror[0].code;
    }

    if (err != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Error Forces Exiting of Server Error Trap #2 Loop");

        // Send the Server Block and bypass sending data (there are none!)
        int rc = _protocol.flush();
        if (rc != 0) {
            return rc;
        }

        return err;
    }

    if (_client_block.get_meta) {
        _total_data_block_size +=
            sizeof(DataSystem) + sizeof(SystemConfig) + sizeof(DataSource) + sizeof(Signal) + sizeof(SignalDesc);

        err = _protocol.send_meta_data(_metadata_block, _log_malloc_list, _user_defined_type_list);
        if (err != 0) {
            return err;
        }
    } // End of Database Meta Data

    //----------------------------------------------------------------------------
    // Send the Data

    print_data_blocks(_data_blocks);
    err = _protocol.send_data_blocks(_data_blocks, _log_malloc_list, _user_defined_type_list);
    if (err != 0) {
        return err;
    }

    //----------------------------------------------------------------------------
    // Send the data in a single full TCP packet

    _protocol.flush();

    //------------------------------------------------------------------------------
    // Legacy Hierarchical Data Structures

    for (const auto& data_block : _data_blocks) {
        err = _protocol.send_hierachical_data(data_block, _log_malloc_list, _user_defined_type_list);
        if (err != 0) {
            return err;
        }
    }

    return err;
}

void uda::server::Server::handshake_client()
{
    // Exchange version details - once only

    init_client_block(&_client_block, 0, "");

    // Receive the client block, respecting earlier protocol versions

    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Initial Client Block");

    int err = 0;

    err = _protocol.read_client_block(&_client_block, _log_malloc_list, _user_defined_type_list);

    if (_client_block.timeout == 0 || _client_block.clientFlags & CLIENTFLAG_CLOSEDOWN) {
        _server_closedown = true;
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

    // protocolVersion = serverVersion;
    // if(client_block.version < serverVersion) protocolVersion = client_block.version;
    // if(client_block.version < server_block.version) protocolVersion = client_block.version;

    // Send the server block

    err = _protocol.send_server_block(_server_block, _log_malloc_list, _user_defined_type_list);
    if (err != 0) {
        throw server::ProtocolError{"Failed to send server block"};
    }

    err = _protocol.flush();
    if (err != 0) {
        throw server::ProtocolError{"Failed to flush protocol"};
    }

    // If the protocol version is legacy (<=6), then divert full control to a legacy server

    if (_client_block.version <= LegacyServerVersion) {
        throw server::ProtocolVersionError{"Unsupported protocol version"};
    }
}
