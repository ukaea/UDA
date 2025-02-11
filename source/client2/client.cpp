#include "client.hpp"

#include "client_xdr_stream.hpp"
#include "exceptions.hpp"
#include "make_request_block.hpp"

#include "cache/fileCache.h"
#include "clientserver/allocData.h"
#include "clientserver/errorLog.h"
#include "clientserver/initStructs.h"
#include "clientserver/printStructs.h"
#include "clientserver/protocol.h"
#include "common/stringUtils.h"
#include "clientserver/udaDefines.h"
#include "clientserver/udaErrors.h"
#include "clientserver/userid.h"
#include "clientserver/version.h"
#include "clientserver/xdrlib.h"
#include "logging/logging.h"
#include "uda/client.h"

#include <uda/version.h>
#include <algorithm>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;

using namespace std::string_literals;

namespace
{

constexpr int ClientVersion = UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);

void copy_data_block(DataBlock* str, DataBlock* in)
{
    *str = *in;
    memcpy(str->errparams, in->errparams, MaxErrParams);
    memcpy(str->data_units, in->data_units, StringLength);
    memcpy(str->data_label, in->data_label, StringLength);
    memcpy(str->data_desc, in->data_desc, StringLength);
    memcpy(str->error_msg, in->error_msg, StringLength);
    init_client_block(&str->client_block, 0, "");
}

void copy_client_block(ClientBlock* str, const uda::client::ClientFlags* client_flags)
{
    str->timeout = client_flags->user_timeout;
    str->clientFlags = client_flags->flags;
    str->altRank = client_flags->alt_rank;
    str->get_datadble = client_flags->get_datadble;
    str->get_dimdble = client_flags->get_dimdble;
    str->get_timedble = client_flags->get_timedble;
    str->get_scalar = client_flags->get_scalar;
    str->get_bytes = client_flags->get_bytes;
    str->get_bad = client_flags->get_bad;
    str->get_meta = client_flags->get_meta;
    str->get_asis = client_flags->get_asis;
    str->get_uncal = client_flags->get_uncal;
    str->get_notoff = client_flags->get_notoff;
    str->get_nodimdata = client_flags->get_nodimdata;
}

void update_client_block(ClientBlock& client_block, const uda::client::ClientFlags& client_flags,
                         unsigned int private_flags)
{
    client_block.timeout = client_flags.user_timeout;
    client_block.clientFlags = client_flags.flags;
    client_block.altRank = client_flags.alt_rank;
    client_block.get_datadble = client_flags.get_datadble;
    client_block.get_dimdble = client_flags.get_dimdble;
    client_block.get_timedble = client_flags.get_timedble;
    client_block.get_scalar = client_flags.get_scalar;
    client_block.get_bytes = client_flags.get_bytes;
    client_block.get_bad = client_flags.get_bad;
    client_block.get_meta = client_flags.get_meta;
    client_block.get_asis = client_flags.get_asis;
    client_block.get_uncal = client_flags.get_uncal;
    client_block.get_notoff = client_flags.get_notoff;
    client_block.get_nodimdata = client_flags.get_nodimdata;
    client_block.privateFlags = private_flags;

    // Operating System Name

    char* env;
    if ((env = getenv("OSTYPE")) != nullptr) {
        strcpy(client_block.OSName, env);
    }

    // Client's study DOI

    if ((env = getenv("UDA_CLIENT_DOI")) != nullptr) {
        strcpy(client_block.DOI, env);
    }
}

} // namespace

uda::client::Client::Client() : version{ClientVersion}, _config{}, _connection{_config}, _protocol_version{ClientVersion}
{
    _host = DefaultHost;
    _port = DefaultPort;
    _flags = 0;

    _client_flags = {};
    _client_flags.alt_rank = 0;
    _client_flags.user_timeout = TimeOut;

    const char* timeout = getenv("UDA_TIMEOUT");
    if (timeout != nullptr) {
        _client_flags.user_timeout = (int)strtol(getenv("UDA_TIMEOUT"), nullptr, 10);
    }

    init_error_stack();

    _config.print();

    //----------------------------------------------------------------
    // Client set Property Flags (can be changed via property accessor functions)
    // Coded user properties changes have priority

    auto client_flags = _config.get("client.flags");
    auto alt_rank = _config.get("client.alt_rank");

    if (client_flags) {
        _flags |= client_flags.as<int>();
    }

    if (alt_rank) {
        _alt_rank = alt_rank.as<int>();
    }

    //----------------------------------------------------------------
    // X.509 Security Certification

    // if((rc = readIdamSecurityCert(environment->security_cert)) != 0){
    //    if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //    return(-1);
    // }

    _cache = uda::cache::open_cache();

    char username[StringLength];
    user_id(username);
    _client_username = username;

    init_client_block(&_client_block, ClientVersion, _client_username.c_str());

    //----------------------------------------------------------------
    // Check if Output Requested

    auto log_level = (LogLevel)_config.get("logging.level").as_or_default((int)UDA_LOG_NONE);

    set_log_level(log_level);
    if (log_level == UDA_LOG_NONE) {
        return;
    }

    //---------------------------------------------------------------
    // Open the Log File

    errno = 0;

    auto log_dir = _config.get("logging.path").as_or_default(""s);
    auto log_mode = _config.get("logging.mode").as_or_default("a"s);

    std::string file_name = log_dir + "Debug.dbg";

    set_log_file(UDA_LOG_WARN, file_name, log_mode);
    set_log_file(UDA_LOG_DEBUG, file_name, log_mode);
    set_log_file(UDA_LOG_INFO, file_name, log_mode);

    if (errno != 0) {
        add_error(ErrorType::System, __func__, errno, "failed to open debug log");
        close_logging();
        return;
    }

    if (get_log_level() <= UDA_LOG_ERROR) {
        file_name = log_dir + "Error.err";
        set_log_file(UDA_LOG_ERROR, file_name, log_mode);
    }

    if (errno != 0) {
        add_error(ErrorType::System, __func__, errno, "failed to open error log");
        close_logging();
        return;
    }
}

int uda::client::Client::fetch_meta()
{
    int err = 0;

#ifndef FATCLIENT // <========================== Client Server Code Only

    if ((err = protocol2(_client_input, ProtocolId::MetaData, XDRStreamDirection::Receive, nullptr, _logmalloclist,
                         _userdefinedtypelist, &_metadata, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 4 Error (Data System)");
        return err;
    }
    print_meta_data(_metadata);

#endif

    return err;
}

int uda::client::Client::fetch_hierarchical_data(DataBlock* data_block)
{
    if (data_block->data_type == UDA_TYPE_COMPOUND && data_block->opaque_type != UDA_OPAQUE_TYPE_UNKNOWN) {

        ProtocolId protocol_id = ProtocolId::Start;
        if (data_block->opaque_type == UDA_OPAQUE_TYPE_XML_DOCUMENT) {
            protocol_id = ProtocolId::Meta;
        } else if (data_block->opaque_type == UDA_OPAQUE_TYPE_STRUCTURES ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDRFILE ||
                   data_block->opaque_type == UDA_OPAQUE_TYPE_XDROBJECT) {
            protocol_id = ProtocolId::Structures;
        }

        UDA_LOG(UDA_LOG_DEBUG, "Receiving Hierarchical Data Structure from Server");

        int err = 0;
        if ((err = protocol2(_client_input, protocol_id, XDRStreamDirection::Receive, nullptr, _logmalloclist, _userdefinedtypelist,
                             data_block, _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
            add_error(ErrorType::Code, __func__, err, "Client Side Protocol Error (Opaque Structure Type)");
            return err;
        }
    }

    return 0;
}

const char* uda::client::Client::get_server_error_stack_record_msg(int record)
{
    UDA_LOG(UDA_LOG_DEBUG, "record {}", record);
    UDA_LOG(UDA_LOG_DEBUG, "count  {}", _server_block.idamerrorstack.nerrors);
    if (record < 0 || (unsigned int)record >= _server_block.idamerrorstack.nerrors) {
        return nullptr;
    }
    return _server_block.idamerrorstack.idamerror[record].msg; // Server Error Stack Record Message
}

int uda::client::Client::get_server_error_stack_record_code(int record)
{
    if (record < 0 || (unsigned int)record >= _server_block.idamerrorstack.nerrors) {
        return 0;
    }
    return _server_block.idamerrorstack.idamerror[record].code; // Server Error Stack Record Code
}

namespace
{
int get_signal_status(DataBlock* data_block)
{
    // Signal Status
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->signal_status;
}

int get_data_status(DataBlock* data_block)
{
    // Data Status based on Standard Rule
    if (data_block == nullptr) {
        return 0;
    }
    if (get_signal_status(data_block) == DefaultStatus) {
        // Signal Status Not Changed from Default - use Data Source Value
        return data_block->source_status;
    } else {
        return data_block->signal_status;
    }
}
} // namespace

int uda::client::Client::get_requests(RequestBlock& request_block, int* indices)
{
    init_server_block(&_server_block, 0);
    init_error_stack();

    time_t tv_server_start = 0;
    time_t tv_server_end = 0;

    // auto server_reconnect = _config.get("client.server_reconnect").as_or_default(false);
    // auto server_change_socket = _config.get("client.server_change_socket").as_or_default(false);

    if (_server_reconnect || _server_change_sockets) {
        int err = _connection.reconnect(&_client_input, &_client_output, &tv_server_start, &_client_flags.user_timeout);
        if (err) {
            return err;
        }
    }

    time(&tv_server_end);
    long age = (long)tv_server_end - (long)tv_server_start;

    UDA_LOG(UDA_LOG_DEBUG, "Start: {}    End: {}", (long)tv_server_start, (long)tv_server_end);
    UDA_LOG(UDA_LOG_DEBUG, "Server Age: {}", age);

    bool init_server = true;
    if (age >= _client_flags.user_timeout - 2) {
        // Assume the Server has Self-Destructed so Instantiate a New Server
        UDA_LOG(UDA_LOG_DEBUG, "Server Age Limit Reached {}", (long)age);
        UDA_LOG(UDA_LOG_DEBUG, "Server Closed and New Instance Started");

        // Close the Existing Socket and XDR Stream: Reopening will Instance a New Server
        closedown(ClosedownType::CLOSE_SOCKETS, &_connection, _client_input, _client_output, &_reopen_logs, &_env_host,
                  &_env_port);
    } else if (_connection.open()) {
        // Assume the Server is Still Alive
        if (_client_output->x_ops == nullptr || _client_input->x_ops == nullptr) {
            add_error(ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
            closedown(ClosedownType::CLOSE_SOCKETS, &_connection, _client_input, _client_output, &_reopen_logs,
                      &_env_host, &_env_port);
        } else {
            init_server = false;
            xdrrec_eof(_client_input); // Flush input socket
        }
    }

    // bool authentication_needed = false;
    bool startup_states = false;
    if (init_server) {
        // authentication_needed = true;
        startup_states = true;
        if (_connection.create(_client_input, _client_output, _host_list) != 0) {
            int err = NO_SOCKET_CONNECTION;
            add_error(ErrorType::Code, __func__, err, "No Socket Connection to Server");
            return err;
        }

        _io_data = _connection.io_data();
        std::tie(_client_input, _client_output) = create_xdr_stream(&_io_data);
        time(&tv_server_start); // Start the Clock again: Age of Server
    }

    char* env = nullptr;

    if ((env = getenv("UDA_PRIVATEFLAGS")) != nullptr) {
        _private_flags |= atoi(env);
    }

    update_client_block(_client_block, _client_flags, _private_flags);
    print_client_block(_client_block);

    //-------------------------------------------------------------------------
    // Client and Server States at Startup only (1 RTT)
    // Will be passed during mutual authentication step

    if (startup_states) {
        perform_handshake();
        startup_states = false;
    } // startup_states

    UDA_LOG(UDA_LOG_DEBUG, "Protocol Version {}", _protocol_version);
    UDA_LOG(UDA_LOG_DEBUG, "Client Version   {}", _client_block.version);
    UDA_LOG(UDA_LOG_DEBUG, "Server Version   {}", _server_block.version);

    int err = test_connection();

    err = send_client_block();
    err = send_request_block(request_block);
    err = send_putdata(request_block);

    err = flush_sockets();

    err = receive_server_block();

    // bool server_side = false;

    if (_server_block.idamerrorstack.nerrors > 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        err = _server_block.idamerrorstack.idamerror[0].code; // Problem on the Server Side!
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        // server_side = true;        // Most Server Side errors are benign so don't close the server
        return 0;
    }

    if (_client_block.get_meta) {
        if ((err = fetch_meta()) != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Fetch the data Block

    DataBlockList recv_data_block_list;

    if ((err = protocol2(_client_input, ProtocolId::DataBlockList, XDRStreamDirection::Receive, nullptr, _logmalloclist,
                         _userdefinedtypelist, &recv_data_block_list, _protocol_version, &_log_struct_list,
                         _private_flags, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");

        add_error(ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
        throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
    }

    print_data_block_list(recv_data_block_list);

    bool data_received = false;
    std::vector<int> data_block_indices(request_block.num_requests);

    for (int i = 0; i < request_block.num_requests; ++i) {
        _data_blocks.emplace_back(DataBlock{});
        auto data_block_idx = _data_blocks.size() - 1;
        DataBlock* data_block = &_data_blocks.back();

        copy_data_block(data_block, &recv_data_block_list.data[i]);
        copy_client_block(&data_block->client_block, &_client_flags);

        if (_client_block.get_meta) {
            data_block->meta_data = _metadata;
        }

        fetch_hierarchical_data(data_block);

        //------------------------------------------------------------------------------
        // Cache the data if the server has passed permission and the application (client) has enabled caching
        if (_client_flags.flags & client_flags::FileCache) {
            udaFileCacheWrite(data_block, &request_block, _logmalloclist, _userdefinedtypelist, _protocol_version,
                              &_log_struct_list, _private_flags, _malloc_source);
        }

        if (_cache != nullptr && _client_flags.flags & client_flags::Cache) {
            cache_write(_config, _cache, &request_block.requests[i], data_block, _logmalloclist, _userdefinedtypelist,
                        _protocol_version, _client_flags.flags, &_log_struct_list, _private_flags,
                        _malloc_source);
        }

        data_block_indices[i] = data_block_idx;
        data_received = true;
    }

    free(recv_data_block_list.data);

    if (data_received) {
        if (err != 0) {
            // Close Socket & XDR Streams but Not Files
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, _client_input, _client_output, &_reopen_logs, &_env_host,
                      &_env_port);
        }

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &_data_blocks[data_block_idx];

            if (err == 0 && (get_data_status(data_block) == MIN_STATUS) && !_client_flags.get_bad) {
                // If Data are not usable, flag the client
                add_error(ErrorType::Code, __func__, DATA_STATUS_BAD,
                          "Data Status is BAD ... Data are Not Usable!");

                if (data_block->errcode == 0) {
                    // Don't over-rule a server side error
                    data_block->errcode = DATA_STATUS_BAD;
                    strcpy(data_block->error_msg, "Data Status is BAD ... Data are Not Usable!");
                }
            }
        }

        //------------------------------------------------------------------------------
        // Concatenate Error Message Stacks & Write to the Error Log

        concat_errors(&_server_block.idamerrorstack);
        close_error();
        error_log(_client_block, request_block, &_server_block.idamerrorstack);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &_data_blocks[data_block_idx];

            if (data_block->errcode == 0 && _server_block.idamerrorstack.nerrors > 0) {
                data_block->errcode = get_server_error_stack_record_code(0);
                strcpy(data_block->error_msg, get_server_error_stack_record_msg(0));
            }
        }

        //------------------------------------------------------------------------------
        // Normal Exit: Return to Client

        std::copy(data_block_indices.begin(), data_block_indices.end(), indices);
        return 0;

        //------------------------------------------------------------------------------
        // Abnormal Exit: Return to Client

    } else {
        UDA_LOG(UDA_LOG_DEBUG, "Returning Error {}", err);

        if (err != 0) {
            closedown(ClosedownType::CLOSE_SOCKETS, nullptr, _client_input, _client_output, &_reopen_logs, &_env_host,
                      &_env_port);
        }

        concat_errors(&_server_block.idamerrorstack);
        close_error();
        error_log(_client_block, request_block, &_server_block.idamerrorstack);

        if (err == 0) {
            return ERROR_CONDITION_UNKNOWN;
        }

        return -abs(err); // Abnormal Exit
    }

    return 0;
}

int uda::client::Client::send_putdata(const RequestBlock& request_block)
{
    for (int i = 0; i < request_block.num_requests; ++i) {
        RequestData* request = &request_block.requests[i];

        if (request->put) {
            ProtocolId protocol_id = ProtocolId::PutdataBlockList;
            int err = 0;
            if ((err = protocol2(_client_output, protocol_id, XDRStreamDirection::Send, nullptr, _logmalloclist, _userdefinedtypelist,
                                 &(request->putDataBlockList), _protocol_version, &_log_struct_list, _private_flags,
                                 _malloc_source)) != 0) {
                add_error(ErrorType::Code, __func__, err,
                          "Protocol 1 Error (sending putDataBlockList from Request Block)");
                throw uda::exceptions::ClientError("Protocol 1 Error (sending putDataBlockList from Request Block)");
            }
        }
    }

    return 0;
}

int uda::client::Client::get(std::string_view data_signal, std::string_view data_source)
{
    RequestBlock request_block;
    init_request_block(&request_block);

    auto signal_ptr = data_signal.data();
    auto source_ptr = data_source.data();

    if (make_request_block(_config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [{}]", data_source);
            add_error(ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source [{}]", data_source);
    }

    print_request_block(request_block);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices[0];
}

std::vector<int> uda::client::Client::get(std::vector<std::pair<std::string, std::string>>& requests)
{
    RequestBlock request_block;
    init_request_block(&request_block);

    std::vector<const char*> signals;
    std::transform(requests.begin(), requests.end(), std::back_inserter(signals),
                   [](const std::pair<std::string, std::string>& p) { return p.first.c_str(); });

    std::vector<const char*> sources;
    std::transform(requests.begin(), requests.end(), std::back_inserter(sources),
                   [](const std::pair<std::string, std::string>& p) { return p.second.c_str(); });

    if (make_request_block(_config, signals.data(), sources.data(), requests.size(), &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices;
}

void uda::client::Client::set_host(std::string_view host)
{
    _host = host;
    _server_reconnect = true;
}

void uda::client::Client::set_port(int port)
{
    _port = port;
    _server_reconnect = true;
}

int uda::client::Client::test_connection()
{
    int rc = 0;
    if (!(rc = xdrrec_eof(_client_input))) { // Test for an EOF

        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} => more input when none expected!", rc);

        int count = 0;
        char temp;

        do {
            rc = xdr_char(_client_input, &temp); // Flush the input (limit to 64 bytes)

            if (rc) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] [{}]", count++, temp);
            }
        } while (rc && count < 64);

        if (count > 0) { // Error if data is waiting
            add_error(
                ErrorType::Code, __func__, 999,
                "Data waiting in the input data buffer when none expected! Please contact the system administrator.");
            UDA_LOG(UDA_LOG_DEBUG, "[{}] excess data bytes waiting in input buffer!", count++);
            throw uda::exceptions::ClientError(
                "Data waiting in the input data buffer when none expected! Please contact the system administrator.");
        }

        rc = xdrrec_eof(_client_input); // Test for an EOF

        if (!rc) {
            rc = xdrrec_skiprecord(_client_input); // Flush the input buffer (Zero data waiting but not an EOF!)
        }

        rc = xdrrec_eof(_client_input); // Test for an EOF

        if (!rc) {
            int err = 999;
            add_error(ErrorType::Code, __func__, err,
                      "Corrupted input data stream! Please contact the system administrator.");
            UDA_LOG(UDA_LOG_DEBUG, "Unable to flush input buffer!!!");
            throw uda::exceptions::ClientError("Corrupted input data stream! Please contact the system administrator.");
        }

        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = 1 => no more input, buffer flushed.");
    }

    return 0;
}

int uda::client::Client::send_request_block(RequestBlock& request_block)
{
    ProtocolId protocol_id = ProtocolId::RequestBlock; // This is what the Client Wants
    int err = 0;
    if ((err = protocol2(_client_output, protocol_id, XDRStreamDirection::Send, nullptr, _logmalloclist, _userdefinedtypelist,
                         &request_block, _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 1 Error (Request Block)");
        throw uda::exceptions::ClientError("Protocol 1 Error (Request Block)");
    }

    return 0;
}

int uda::client::Client::send_client_block()
{
    ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block
    int err = 0;
    if ((err = protocol2(_client_output, protocol_id, XDRStreamDirection::Send, nullptr, _logmalloclist, _userdefinedtypelist,
                         &_client_block, _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    return 0;
}

int uda::client::Client::perform_handshake()
{
    // Flush (mark as at EOF) the input socket buffer (before the exchange begins)

    ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block (proxy for authenticationStep = 6)

    int err = 0;
    if ((err = protocol2(_client_output, protocol_id, XDRStreamDirection::Send, nullptr, _logmalloclist, _userdefinedtypelist,
                         &_client_block, _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error Sending Client Block");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    if (!(xdrrec_endofrecord(_client_output, 1))) { // Send data now
        err = (int)ProtocolError::Error7;
        add_error(ErrorType::Code, __func__, err, "Protocol 7 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_endofrecord after Client Block");
        throw uda::exceptions::ClientError("Protocol 7 Error (Client Block)");
    }

    // Flush (mark as at EOF) the input socket buffer (start of wait for data)

    // Wait for data, then position buffer reader to the start of a new record
    if (!(xdrrec_skiprecord(_client_input))) {
        err = (int)ProtocolError::Error5;
        add_error(ErrorType::Code, __func__, err, "Protocol 5 Error (Server Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_skiprecord prior to Server Block");
        throw uda::exceptions::ClientError("Protocol 5 Error (Server Block)");
    }

    protocol_id =
        ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement (proxy for authenticationStep = 8)

    if ((err = protocol2(_client_input, protocol_id, XDRStreamDirection::Receive, nullptr, _logmalloclist, _userdefinedtypelist,
                         &_server_block, _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        add_error(ErrorType::Code, __func__, err, "Protocol 11 Error (Server Block #1)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid concatonation problems
        _server_block.idamerrorstack.nerrors = 0;
        UDA_LOG(UDA_LOG_DEBUG, "Error receiving Server Block");
        throw uda::exceptions::ClientError("Protocol 11 Error (Server Block #1)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

    int rc = xdrrec_eof(_client_input);

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
    UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} [1 => no more input]", rc);
    print_server_block(_server_block);

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    _protocol_version = std::min(get_protocol_version(_client_block.version), get_protocol_version(_server_block.version));

    if (_server_block.idamerrorstack.nerrors > 0) {
        err = _server_block.idamerrorstack.idamerror[0].code; // Problem on the Server Side!
        throw uda::exceptions::ServerError(_server_block.idamerrorstack.idamerror[0].msg);
    }

    return 0;
}

int uda::client::Client::flush_sockets()
{
    //------------------------------------------------------------------------------
    // Send the Full TCP packet and wait for the returned data

    int rc = 0;
    if (!(rc = xdrrec_endofrecord(_client_output, 1))) {
        int err = (int)ProtocolError::Error7;
        add_error(ErrorType::Code, __func__, err, "Protocol 7 Error (Request Block & putDataBlockList)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Outgoing tcp packet sent without error. Waiting for data.");

    if (!xdrrec_skiprecord(_client_input)) {
        int err = (int)ProtocolError::Error5;
        add_error(ErrorType::Code, __func__, err, " Protocol 5 Error (Server & Data Structures)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Incoming tcp packet received without error. Reading...");
    return 0;
}

int uda::client::Client::receive_server_block()
{
    //------------------------------------------------------------------------------
    // Receive the Server State/Aknowledgement that the Data has been Accessed
    // Just in case the Server has crashed!

    UDA_LOG(UDA_LOG_DEBUG, "Waiting for Server Status Block");

    int err = 0;
    if ((err = protocol2(_client_input, ProtocolId::ServerBlock, XDRStreamDirection::Receive, nullptr, _logmalloclist,
                         _userdefinedtypelist, &_server_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 11 Error (Server Block #2) = {}", err);
        add_error(ErrorType::Code, __func__, err, " Protocol 11 Error (Server Block #2)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid future concatonation problems
        _server_block.idamerrorstack.nerrors = 0;
        throw uda::exceptions::ClientError("Protocol 11 Error (Server Block #2) = {}", err);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
    print_server_block(_server_block);

    return 0;
}

void uda::client::Client::clear()
{
    _data_blocks.clear();
}

int uda::client::Client::new_handle()
{
    _data_blocks.emplace_back(DataBlock{});
    return static_cast<int>(_data_blocks.size() - 1);
}

void uda::client::Client::free_handle(int handle_idx) {
    if (handle_idx < 0 || handle_idx >= static_cast<int>(_data_blocks.size())) {
        UDA_LOG(UDA_LOG_ERROR, "Invalid handle index");
        return;
    }
    _data_blocks.at(handle_idx) = {};
}

void uda::client::Client::free_all() {
    _data_blocks.clear();
}

void uda::client::Client::set_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        _private_flags |= flag;
    } else {
        _client_flags.flags |= flag;
    }
}

void uda::client::Client::reset_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        _private_flags &= !flag;
    } else {
        _client_flags.flags &= !flag;
    }
}

void uda::client::Client::set_property(const char* property)
{
    // User settings for Client and Server behaviour

    char name[56];
    char* value;

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            _client_flags.get_datadble = 1;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            _client_flags.get_dimdble = 1;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            _client_flags.get_timedble = 1;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            _client_flags.get_bytes = 1;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            _client_flags.get_bad = 1;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            _client_flags.get_meta = 1;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            _client_flags.get_asis = 1;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            _client_flags.get_uncal = 1;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            _client_flags.get_notoff = 1;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            _client_flags.get_synthetic = 1;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            _client_flags.get_scalar = 1;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            _client_flags.get_nodimdata = 1;
        }
    } else {
        if (property[0] == 't') {
            copy_string(property, name, 56);
            trim_string(name);
            left_trim_string(name);
            mid_trim_string(name);
            strlwr(name);
            if ((value = strstr(name, "timeout=")) != nullptr) {
                value = name + 8;
                if (is_number(value)) {
                    _client_flags.user_timeout = atoi(value);
                }
            }
        } else {
            if (STR_IEQUALS(property, "verbose")) {
                set_log_level(UDA_LOG_INFO);
            }
            if (STR_IEQUALS(property, "debug")) {
                set_log_level(UDA_LOG_DEBUG);
            }
            if (STR_IEQUALS(property, "altData")) {
                _client_flags.flags = _client_flags.flags | client_flags::AltData;
            }
            if (!strncasecmp(property, "altRank", 7)) {
                copy_string(property, name, 56);
                trim_string(name);
                left_trim_string(name);
                mid_trim_string(name);
                strlwr(name);
                if ((value = strcasestr(name, "altRank=")) != nullptr) {
                    value = name + 8;
                    if (is_number(value)) {
                        _client_flags.alt_rank = atoi(value);
                    }
                }
            }
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            _client_flags.flags = _client_flags.flags | client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            _client_flags.flags = _client_flags.flags | client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            _client_flags.flags = _client_flags.flags | client_flags::FileCache;
        }
    }
}

int uda::client::Client::get_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            return _client_flags.get_datadble;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            return _client_flags.get_dimdble;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            return _client_flags.get_timedble;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            return _client_flags.get_bytes;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            return _client_flags.get_bad;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            return _client_flags.get_meta;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            return _client_flags.get_asis;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            return _client_flags.get_uncal;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            return _client_flags.get_notoff;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            return _client_flags.get_synthetic;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            return _client_flags.get_scalar;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            return _client_flags.get_nodimdata;
        }
    } else {
        if (STR_IEQUALS(property, "timeout")) {
            return _client_flags.user_timeout;
        }
        if (STR_IEQUALS(property, "altRank")) {
            return _alt_rank;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            return (int)(_client_flags.flags & client_flags::ReuseLastHandle);
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            return (int)(_client_flags.flags & client_flags::FreeReuseLastHandle);
        }
        if (STR_IEQUALS(property, "verbose")) {
            return get_log_level() == UDA_LOG_INFO;
        }
        if (STR_IEQUALS(property, "debug")) {
            return get_log_level() == UDA_LOG_DEBUG;
        }
        if (STR_IEQUALS(property, "altData")) {
            return (int)(_client_flags.flags & client_flags::AltData);
        }
        if (STR_IEQUALS(property, "fileCache")) {
            return (int)(_client_flags.flags & client_flags::FileCache);
        }
    }
    return 0;
}

void uda::client::Client::reset_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            _client_flags.get_datadble = 0;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            _client_flags.get_dimdble = 0;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            _client_flags.get_timedble = 0;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            _client_flags.get_bytes = 0;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            _client_flags.get_bad = 0;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            _client_flags.get_meta = 0;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            _client_flags.get_asis = 0;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            _client_flags.get_uncal = 0;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            _client_flags.get_notoff = 0;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            _client_flags.get_synthetic = 0;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            _client_flags.get_scalar = 0;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            _client_flags.get_nodimdata = 0;
        }
    } else {
        if (STR_IEQUALS(property, "verbose")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "debug")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "altData")) {
            _client_flags.flags &= !client_flags::AltData;
        }
        if (STR_IEQUALS(property, "altRank")) {
            _client_flags.alt_rank = 0;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            _client_flags.flags &= !client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            _client_flags.flags &= !client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            _client_flags.flags &= !client_flags::FileCache;
        }
    }
}

void uda::client::Client::reset_properties()
{
    // Reset on Both Client and Server

    _client_flags.get_datadble = 0;
    _client_flags.get_dimdble = 0;
    _client_flags.get_timedble = 0;
    _client_flags.get_bad = 0;
    _client_flags.get_meta = 0;
    _client_flags.get_asis = 0;
    _client_flags.get_uncal = 0;
    _client_flags.get_notoff = 0;
    _client_flags.get_synthetic = 0;
    _client_flags.get_scalar = 0;
    _client_flags.get_bytes = 0;
    _client_flags.get_nodimdata = 0;
    set_log_level(UDA_LOG_NONE);
    _client_flags.user_timeout = TimeOut;
    if (getenv("UDA_TIMEOUT")) {
        _client_flags.user_timeout = atoi(getenv("UDA_TIMEOUT"));
    }
    _client_flags.flags = 0;
    _client_flags.alt_rank = 0;
}

const DataBlock* uda::client::Client::data_block(int handle) const
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < _data_blocks.size()) {
        return &_data_blocks[idx];
    } else {
        return nullptr;
    }
}

DataBlock* uda::client::Client::data_block(int handle)
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < _data_blocks.size()) {
        return &_data_blocks[idx];
    } else {
        return nullptr;
    }
}

const ClientBlock* uda::client::Client::client_block(int handle) const
{
    return &_client_block;
}

void uda::client::Client::concat_errors(uda::client_server::ErrorStack* error_stack) const
{
    if (_error_stack.empty()) {
        return;
    }

    unsigned int iold = error_stack->nerrors;
    unsigned int inew = _error_stack.size() + error_stack->nerrors;

    error_stack->idamerror = (UdaError*)realloc((void*)error_stack->idamerror, (inew * sizeof(UdaError)));

    for (unsigned int i = iold; i < inew; i++) {
        error_stack->idamerror[i] = _error_stack[i - iold];
    }
    error_stack->nerrors = inew;
}

const uda::client::ClientFlags* uda::client::Client::client_flags() const
{
    return &_client_flags;
}

const DataBlock* uda::client::Client::current_data_block() const
{
    if (!_data_blocks.empty()) {
        return &_data_blocks.back();
    } else {
        return nullptr;
    }
}

const ServerBlock* uda::client::Client::server_block() const
{
    return &_server_block;
}

void uda::client::Client::set_user_defined_type_list(UserDefinedTypeList* userdefinedtypelist)
{
    _userdefinedtypelist = userdefinedtypelist;
}

void uda::client::Client::set_log_malloc_list(LogMallocList* logmalloclist)
{
    _logmalloclist = logmalloclist;
}

void uda::client::Client::set_full_ntree(NTREE* full_ntree)
{
    _full_ntree = static_cast<NTree*>(full_ntree);
}

int uda::client::Client::put(std::string_view put_instruction, uda::client_server::PutDataBlock* putdata_block)
{
    RequestBlock request_block;
    init_request_block(&request_block);

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(_config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)
    add_put_data_block_list(putdata_block, &request_block.requests[0].putDataBlockList);

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    free_client_put_data_block_list(&request_block.requests[0].putDataBlockList);

    return indices[0];
}

int uda::client::Client::put(std::string_view put_instruction, uda::client_server::PutDataBlockList* putdata_block_list)
{
    RequestBlock request_block;
    init_request_block(&request_block);

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(_config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (udaNumErrors() == 0) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    request_block.requests[0].put = 1; // flags the direction of data (0 is default => get operation)
    request_block.requests[0].putDataBlockList = *putdata_block_list;

    std::vector<int> indices(request_block.num_requests);
    get_requests(request_block, indices.data());

    return indices[0];
}
