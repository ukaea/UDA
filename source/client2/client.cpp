#include "client.hpp"

#include "connection.hpp"
#include "exceptions.hpp"
#include "make_request_block.hpp"

#include "cache/fileCache.h"
#include "clientserver/error_log.h"
#include "clientserver/init_structs.h"
#include "clientserver/print_structs.h"
#include "clientserver/uda_defines.h"
#include "clientserver/uda_errors.h"
#include "clientserver/userid.h"
#include "clientserver/version.h"
#include "common/string_utils.h"
#include "logging/logging.h"
#include "protocol/protocol.h"
#include "protocol/xdr_lib.h"
#include "uda/client.h"

#include <algorithm>
#include <filesystem>
#include <uda/version.h>
#include <iostream>
#include <authentication/client_ssl.h>

using namespace uda::client_server;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;
using namespace uda::protocol;

using namespace std::string_literals;

namespace
{

constexpr int ClientVersion =
    UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);

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
                         const unsigned int private_flags)
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
    const char* env = getenv("OSTYPE");
    if (env != nullptr) {
        strcpy(client_block.OSName, env);
    }
}

} // namespace

uda::client::Client::Client() noexcept
    : _version{ClientVersion}
    , _protocol_version{ClientVersion}
{
    try {
        _cache = cache::open_cache();
        _client_flags = {};
        _client_flags.alt_rank = 0;
        _client_flags.user_timeout = TimeOut;

        std::array<char, StringLength> username{};
        user_id(username);
        _client_username = username.data();

        init_client_block(&_client_block, ClientVersion, _client_username.c_str());

#  if defined(SSLAUTHENTICATION) && !defined(FATCLIENT)
        // Create the SSL binding and context, and verify the server certificate
        authentication::init_client_ssl(_config, _error_stack);
#  endif
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }
}

uda::client::Client::Client(const std::string_view config_path) : Client()
{
    // load config throws on error, no need to check _config has read a valid file
    load_config(config_path);
    // _connection = Connection(_error_stack, _config);
    _connection.load_config(_config);

    // TODO: this will throw if the config path is illegal
    // what behaviour do we want? ignore silently and fallback to default constructed client
    // or let it crash here?
    // _config.load(config_path);
    // _connection = Connection(_config);
    //
    // // TODO:
    // // - timeout from config
    // // - parse client flags fromn config
    // // - initialise other any structs from config?
    //
    // //----------------------------------------------------------------
    // // Client set Property Flags (can be changed via property accessor functions)
    // // Coded user properties changes have priority
    //
    // auto client_flags = _config.get("client.flags");
    // auto alt_rank = _config.get("client.alt_rank");
    //
    // if (client_flags) {
    //     _flags |= client_flags.as<int>();
    // }
    //
    // if (alt_rank) {
    //     _alt_rank = alt_rank.as<int>();
    // }

    //----------------------------------------------------------------
    // X.509 Security Certification

    // if((rc = readIdamSecurityCert(environment->security_cert)) != 0){
    //    if(verbose) fprintf(stderr, "Idam: Problem Locating the Security Certificate [%d]\n",  rc);
    //    return(-1);
    // }

    //     //----------------------------------------------------------------
    //     // Check if Output Requested
    //
    //     constexpr int default_log_level = static_cast<int>(UDA_LOG_NONE);
    //     auto log_level = static_cast<LogLevel>(_config.get("logging.level")
    //             .as_or_default(default_log_level));
    //
    //     if (log_level == UDA_LOG_NONE) {
    //         return;
    //     }
    //
    //     //---------------------------------------------------------------
    //     // Open the Log File
    //
    //     const auto log_dir = _config.get("logging.path").as_or_default(""s);
    //     const auto log_mode = _config.get("logging.mode").as_or_default("a"s);
    //     initialise_logging(log_dir, log_level, log_mode);
    //
    //     errno = 0;
    //     if (errno == 0)
    //     {
    //         _config.print();
    //     }
}

void uda::client::Client::load_config(std::string_view path)
{
    // load step throws on error
    _config.load(path);
    set_client_flags_from_config();
    initialise_logging_from_config();
    _connection.load_config(_config);
}

void uda::client::Client::set_client_flags_from_config()
{
    if (!_config) {
        return;
    }

    for (const auto& [key, value] : _config.get_section_as_map("client_flags")) {
        if (key == "timeout" and value) {
            _client_flags.user_timeout = value.as_or_default<int>(DefaultTimeout);
        } else if (value and value.is<bool>()) {
            set_property(key.c_str());
        }
    }
}

void uda::client::Client::Client::initialise_logging_from_config()
{
    //----------------------------------------------------------------
    // Check if Output Requested

    constexpr int default_log_level = static_cast<int>(UDA_LOG_NONE);
    auto log_level = static_cast<LogLevel>(_config.get("logging.level").as_or_default(default_log_level));

    if (log_level == UDA_LOG_NONE) {
        return;
    }
    _logging_options.log_level = log_level;

    //---------------------------------------------------------------
    // Open the Log File

    const auto log_dir = _config.get("logging.path").as_or_default<std::string>(""s);
    if (!log_dir.empty()) {
        _logging_options.log_dir = log_dir;
    }
    const auto log_mode = _config.get("logging.mode").as_or_default<std::string>("a"s);
    if (log_mode != "a"s) {
        _logging_options.open_mode = LogOpenMode::CLEAR;
    }

    errno = 0;

    initialise_logging(_logging_options.log_dir, _logging_options.log_level, _logging_options.open_mode);

    if (errno == 0) {
        _config.print();
    }
}

void uda::client::Client::initialise_logging(const std::string& log_dir, LogLevel log_level, LogOpenMode open_mode)
{
    if (log_level == UDA_LOG_NONE) {
        return;
    }

    init_logging();
    set_log_level(log_level);

    errno = 0;

    std::string log_mode = (open_mode == LogOpenMode::APPEND) ? "a"s : "";
    auto file_name = (std::filesystem::path(log_dir) / "Debug.dbg").string();
    set_log_file(UDA_LOG_WARN, file_name, log_mode);
    set_log_file(UDA_LOG_DEBUG, file_name, log_mode);
    set_log_file(UDA_LOG_INFO, file_name, log_mode);

    // TODO: should these throw?
    if (errno != 0) {
        add_error(_error_stack, ErrorType::System, __func__, errno, "failed to open debug log");
        close_logging();
        return;
    }

    if (get_log_level() <= UDA_LOG_ERROR) {
        file_name = (std::filesystem::path(log_dir) / "Error.err").string();
        set_log_file(UDA_LOG_ERROR, file_name, log_mode);
    }

    if (errno != 0) {
        add_error(_error_stack, ErrorType::System, __func__, errno, "failed to open error log");
        close_logging();
        return;
    }
}

void uda::client::Client::initialise_logging()
{
    initialise_logging(_logging_options.log_dir, _logging_options.log_level, _logging_options.open_mode);
}

int uda::client::Client::fetch_meta()
{
    int err = 0;

#ifndef FATCLIENT // <========================== Client Server Code Only

    if ((err = protocol2(_error_stack, _client_input, ProtocolId::MetaData, XDRStreamDirection::Receive, nullptr,
                         _log_malloc_list, _user_defined_type_list, &_metadata, _protocol_version, &_log_struct_list,
                         _private_flags, _malloc_source)) != 0) {
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 4 Error (Data System)");
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
        if ((err = protocol2(_error_stack, _client_input, protocol_id, XDRStreamDirection::Receive, nullptr,
                             _log_malloc_list, _user_defined_type_list, data_block, _protocol_version,
                             &_log_struct_list, _private_flags, _malloc_source)) != 0) {
            add_error(_error_stack, ErrorType::Code, __func__, err,
                      "Client Side Protocol Error (Opaque Structure Type)");
            return err;
        }
    }

    return 0;
}

const char* uda::client::Client::get_server_error_stack_record_msg(int record)
{
    UDA_LOG(UDA_LOG_DEBUG, "record {}", record);
    UDA_LOG(UDA_LOG_DEBUG, "count  {}", _server_block.error_stack.size());
    if (record < 0 || static_cast<size_t>(record) >= _server_block.error_stack.size()) {
        return nullptr;
    }
    return _server_block.error_stack[record].msg; // Server Error Stack Record Message
}

int uda::client::Client::get_server_error_stack_record_code(int record)
{
    if (record < 0 || static_cast<size_t>(record) >= _server_block.error_stack.size()) {
        return 0;
    }
    return _server_block.error_stack[record].code; // Server Error Stack Record Code
}

namespace
{
int get_signal_status(const DataBlock* data_block)
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

void uda::client::Client::new_socket_connection()
{
    //TODO: replace client error stack with exceptions
    if (_connection.create() != 0) {
        int err = NO_SOCKET_CONNECTION;
        add_error(_error_stack, ErrorType::Code, __func__, err, "No Socket Connection to Server");
        throw uda::exceptions::ClientError("No socket connection to server");
    }

    _connection.register_new_xdr_streams();
    std::tie(_client_input, _client_output) = _connection.get_socket_xdr_streams();
    if (_client_output == nullptr || _client_input == nullptr) {
        throw uda::exceptions::ClientError("failed to open new XDR streams");
    }

}

void uda::client::Client::ensure_connection()
{
    if (_connection.open() and _connection.current_socket_timeout()) {
        auto age = _connection.get_current_socket_age();
        UDA_LOG(UDA_LOG_DEBUG, "Server Age Limit Reached {}", (long)age);
        UDA_LOG(UDA_LOG_DEBUG, "Server Closed and New Instance Started");
        // this call may have to change when we add ssl back in
        // closedown(ClosedownType::CLOSE_SOCKETS, &_connection);
        _connection.close_down(ClosedownType::CLOSE_SOCKETS);
    }
    if (_connection.reconnect_required()) {
        _connection.maybe_reuse_existing_socket();
    }
    if (_connection.open()){
        std::tie(_client_input, _client_output) = _connection.get_socket_xdr_streams();
        if (_client_input == nullptr || _client_output == nullptr)
        {
            add_error(_error_stack, ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
            _connection.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        if (_client_output->x_ops == nullptr || _client_input->x_ops == nullptr) {
            add_error(_error_stack, ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
            // this call may have to change when we add ssl back in
            // closedown(ClosedownType::CLOSE_SOCKETS, &_connection);
            _connection.close_down(ClosedownType::CLOSE_SOCKETS);
        }
    }

    if (!_connection.open()) {
        new_socket_connection();
    } else {
        xdrrec_eof(_client_input); // Flush input socket
    }
}

int uda::client::Client::get_requests(RequestBlock& request_block, int* indices)
{
    // TODO: this can be removed now?
    // logging can be closed from a closedown(type=CLOSE_ALL) command.
    // reopen if required
    if (!logging_initialised()) {
        initialise_logging();
    }

    init_server_block(&_server_block, 0);
    _error_stack.clear();

    _connection.set_maximum_socket_age(_client_flags.user_timeout);
    auto age = _connection.get_current_socket_age();
    UDA_LOG(UDA_LOG_DEBUG, "Server Age: {}", age);
    ensure_connection();

    update_client_block(_client_block, _client_flags, _private_flags);
    print_client_block(_client_block);

    //-------------------------------------------------------------------------
    // Client and Server States at Startup only (1 RTT)
    // Will be passed during mutual authentication step

    if (_connection.startup_state) {
        perform_handshake();
        _connection.startup_state = false;
    }

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

    if (!_server_block.error_stack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        err = _server_block.error_stack[0].code; // Problem on the Server Side!
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        // server_side = true;        // Most Server Side errors are benign so don't close the server
        return err;
    }

    if (_client_block.get_meta) {
        if ((err = fetch_meta()) != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Fetch the data Block

    std::vector<DataBlock> recv_data_block_list;

    try
    {

        if ((err = protocol2(_error_stack, _client_input, ProtocolId::DataBlockList, XDRStreamDirection::Receive, nullptr,
                        _log_malloc_list, _user_defined_type_list, &recv_data_block_list, _protocol_version,
                        &_log_struct_list, _private_flags, _malloc_source)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");

            add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
            throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
        }
    }
    catch (std::exception& e)
    {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
        throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
        // throw e.what();
    }

    print_data_block_list(recv_data_block_list);

    bool data_received = false;
    std::vector<int> data_block_indices(request_block.size());

    for (size_t i = 0; i < request_block.size(); ++i) {
        _data_blocks.emplace_back(DataBlock{});
        auto data_block_idx = _data_blocks.size() - 1;
        DataBlock* data_block = &_data_blocks.back();

        copy_data_block(data_block, &recv_data_block_list[i]);
        copy_client_block(&data_block->client_block, &_client_flags);

        if (_client_block.get_meta) {
            data_block->meta_data = _metadata;
        }

        fetch_hierarchical_data(data_block);

        //------------------------------------------------------------------------------
        // Cache the data if the server has passed permission and the application (client) has enabled caching
        if (_client_flags.flags & client_flags::FileCache) {
            cache::udaFileCacheWrite(_error_stack, data_block, &request_block, _log_malloc_list,
                                     _user_defined_type_list, _protocol_version, &_log_struct_list, _private_flags,
                                     _malloc_source);
        }

        if (_cache != nullptr && _client_flags.flags & client_flags::Cache) {
            cache_write(_config, _cache, &request_block[i], data_block, _log_malloc_list,
                        _user_defined_type_list, _protocol_version, _client_flags.flags, &_log_struct_list,
                        _private_flags, _malloc_source);
        }

        data_block_indices[i] = data_block_idx;
        data_received = true;
    }

    if (data_received) {
        if (err != 0) {
            // Close Socket & XDR Streams but Not Files
            // closedown(ClosedownType::CLOSE_SOCKETS, nullptr, _client_input, _client_output, &reopen_logs_);
            _connection.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &_data_blocks[data_block_idx];

            if (err == 0 && (get_data_status(data_block) == MIN_STATUS) && !_client_flags.get_bad) {
                // If Data are not usable, flag the client
                add_error(_error_stack, ErrorType::Code, __func__, DATA_STATUS_BAD,
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

        concat_errors(_server_block);
        _error_stack.clear();
        print_errors(_server_block.error_stack, _client_block, request_block);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (const auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &_data_blocks[data_block_idx];

            if (data_block->errcode == 0 && !_server_block.error_stack.empty()) {
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
            // closedown(ClosedownType::CLOSE_SOCKETS, nullptr, _client_input, _client_output, &reopen_logs_);
            _connection.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        concat_errors(_server_block);
        _error_stack.clear();
        print_errors(_server_block.error_stack, _client_block, request_block);

        if (err == 0) {
            return ERROR_CONDITION_UNKNOWN;
        }

        return -abs(err); // Abnormal Exit
    }

    return 0;
}

int uda::client::Client::send_putdata(const RequestBlock& request_block)
{
    for (size_t i = 0; i < request_block.size(); ++i) {
        const RequestData* request = &request_block[i];

        if (request->put) {
            ProtocolId protocol_id = ProtocolId::PutdataBlockList;
            int err = 0;
            if ((err = protocol2(_error_stack, _client_output, protocol_id, XDRStreamDirection::Send, nullptr,
                                 _log_malloc_list, _user_defined_type_list, const_cast<PutDataBlockList*>(&request->putDataBlockList),
                                 _protocol_version, &_log_struct_list, _private_flags, _malloc_source)) != 0) {
                add_error(_error_stack, ErrorType::Code, __func__, err,
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

    auto signal_ptr = data_signal.data();
    auto source_ptr = data_source.data();

    if (make_request_block(_error_stack, _config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (_error_stack.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [{}]", data_source);
            add_error(_error_stack, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source [{}]", data_source);
    }

    print_request_block(request_block);

    std::vector<int> indices(request_block.size());
    get_requests(request_block, indices.data());

    return indices[0];
}

std::vector<int> uda::client::Client::get(std::vector<std::pair<std::string, std::string>>& requests)
{
    RequestBlock request_block;

    std::vector<const char*> signals;
    std::transform(requests.begin(), requests.end(), std::back_inserter(signals),
                   [](const std::pair<std::string, std::string>& p) { return p.first.c_str(); });

    std::vector<const char*> sources;
    std::transform(requests.begin(), requests.end(), std::back_inserter(sources),
                   [](const std::pair<std::string, std::string>& p) { return p.second.c_str(); });

    if (make_request_block(_error_stack, _config, signals.data(), sources.data(), requests.size(), &request_block) !=
        0) {
        if (_error_stack.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(_error_stack, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw uda::exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    std::vector<int> indices(request_block.size());
    get_requests(request_block, indices.data());

    return indices;
}

void uda::client::Client::set_host(std::string_view host)
{
    _connection.set_host(host);
}

void uda::client::Client::load_host_list(std::string_view file_path)
{
    _connection.load_host_list(file_path);
}

void uda::client::Client::set_port(int port)
{
    _connection.set_port(port);
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
                _error_stack, ErrorType::Code, __func__, 999,
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
            add_error(_error_stack, ErrorType::Code, __func__, err,
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
    auto protocol_id = ProtocolId::RequestBlock; // This is what the Client Wants
    int err = 0;
    if ((err = protocol2(_error_stack, _client_output, protocol_id, XDRStreamDirection::Send, nullptr, _log_malloc_list,
                         _user_defined_type_list, &request_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 1 Error (Request Block)");
        throw uda::exceptions::ClientError("Protocol 1 Error (Request Block)");
    }

    return 0;
}

int uda::client::Client::send_client_block()
{
    auto protocol_id = ProtocolId::ClientBlock; // Send Client Block
    int err = 0;
    if ((err = protocol2(_error_stack, _client_output, protocol_id, XDRStreamDirection::Send, nullptr, _log_malloc_list,
                         _user_defined_type_list, &_client_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    return 0;
}

int uda::client::Client::perform_handshake()
{
    // Flush (mark as at EOF) the input socket buffer (before the exchange begins)

    auto protocol_id = ProtocolId::ClientBlock; // Send Client Block (proxy for authenticationStep = 6)

    int err = 0;
    if ((err = protocol2(_error_stack, _client_output, protocol_id, XDRStreamDirection::Send, nullptr, _log_malloc_list,
                         _user_defined_type_list, &_client_block, _protocol_version, &_log_struct_list, _private_flags,
                         _malloc_source)) != 0) {
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error Sending Client Block");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    if (!(xdrrec_endofrecord(_client_output, 1))) { // Send data now
        err = (int)ProtocolError::Error7;
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 7 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_endofrecord after Client Block");
        throw uda::exceptions::ClientError("Protocol 7 Error (Client Block)");
    }

    // Flush (mark as at EOF) the input socket buffer (start of wait for data)
    // int rc = xdrrec_eof(_client_input);

    // Wait for data, then position buffer reader to the start of a new record
    if (!(xdrrec_skiprecord(_client_input))) {
        err = static_cast<int>(ProtocolError::Error5);
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 5 Error (Server Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_skiprecord prior to Server Block");
        throw exceptions::ClientError("Protocol 5 Error (Server Block)");
    }

    protocol_id =
        ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement (proxy for authenticationStep = 8)

    if ((err = protocol2(_error_stack, _client_input, protocol_id, XDRStreamDirection::Receive, nullptr,
                         _log_malloc_list, _user_defined_type_list, &_server_block, _protocol_version,
                         &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 11 Error (Server Block #1)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid concatenation problems
        _server_block.error_stack.clear();
        UDA_LOG(UDA_LOG_DEBUG, "Error receiving Server Block");
        throw exceptions::ClientError("Protocol 11 Error (Server Block #1)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

    int rc = xdrrec_eof(_client_input);

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
    UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} [1 => no more input]", rc);
    print_server_block(_server_block);

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    _protocol_version =
        std::min(get_protocol_version(_client_block.version), get_protocol_version(_server_block.version));

    if (!_server_block.error_stack.empty()) {
        err = _server_block.error_stack[0].code; // Problem on the Server Side!
        throw exceptions::ServerError(_server_block.error_stack[0].msg);
    }

    return 0;
}

int uda::client::Client::flush_sockets()
{
    //------------------------------------------------------------------------------
    // Send the Full TCP packet and wait for the returned data

    if (!xdrrec_endofrecord(_client_output, 1)) {
        constexpr int err = static_cast<int>(ProtocolError::Error7);
        add_error(_error_stack, ErrorType::Code, __func__, err, "Protocol 7 Error (Request Block & putDataBlockList)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Outgoing tcp packet sent without error. Waiting for data.");

    if (!xdrrec_skiprecord(_client_input)) {
        constexpr int err = static_cast<int>(ProtocolError::Error5);
        add_error(_error_stack, ErrorType::Code, __func__, err, " Protocol 5 Error (Server & Data Structures)");
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
    if ((err = protocol2(_error_stack, _client_input, ProtocolId::ServerBlock, XDRStreamDirection::Receive, nullptr,
                         _log_malloc_list, _user_defined_type_list, &_server_block, _protocol_version,
                         &_log_struct_list, _private_flags, _malloc_source)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 11 Error (Server Block #2) = {}", err);
        add_error(_error_stack, ErrorType::Code, __func__, err, " Protocol 11 Error (Server Block #2)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid future concatonation problems
        _server_block.error_stack.clear();
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

void uda::client::Client::free_handle(int handle_idx)
{
    if (handle_idx < 0 || handle_idx >= static_cast<int>(_data_blocks.size())) {
        UDA_LOG(UDA_LOG_ERROR, "Invalid handle index");
        return;
    }
    _data_blocks.at(handle_idx) = {};
}

void uda::client::Client::free_all()
{
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
            if (strstr(name, "timeout=") != nullptr) {
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
                if (strcasestr(name, "altRank=") != nullptr) {
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
            return static_cast<int>(_client_flags.flags & client_flags::ReuseLastHandle);
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            return static_cast<int>(_client_flags.flags & client_flags::FreeReuseLastHandle);
        }
        if (STR_IEQUALS(property, "verbose")) {
            return get_log_level() == UDA_LOG_INFO;
        }
        if (STR_IEQUALS(property, "debug")) {
            return get_log_level() == UDA_LOG_DEBUG;
        }
        if (STR_IEQUALS(property, "altData")) {
            return static_cast<int>(_client_flags.flags & client_flags::AltData);
        }
        if (STR_IEQUALS(property, "fileCache")) {
            return static_cast<int>(_client_flags.flags & client_flags::FileCache);
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
    _client_flags.flags = 0;
    _client_flags.alt_rank = 0;
}

const DataBlock* uda::client::Client::data_block(int handle) const
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < _data_blocks.size()) {
        return &_data_blocks[idx];
    }
    return nullptr;
}

DataBlock* uda::client::Client::data_block(int handle)
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < _data_blocks.size()) {
        return &_data_blocks[idx];
    }
    return nullptr;
}

const ClientBlock* uda::client::Client::client_block(int handle) const
{
    return &_client_block;
}

void uda::client::Client::concat_errors(ServerBlock& server_block) const
{
    if (_error_stack.empty()) {
        return;
    }

    for (const auto& error : _error_stack) {
        server_block.error_stack.push_back(error);
    }
}

const uda::client::ClientFlags* uda::client::Client::client_flags() const
{
    return &_client_flags;
}

const DataBlock* uda::client::Client::current_data_block() const
{
    if (!_data_blocks.empty()) {
        return &_data_blocks.back();
    }
    return nullptr;
}

const ServerBlock* uda::client::Client::server_block() const
{
    return &_server_block;
}

void uda::client::Client::set_user_defined_type_list(UserDefinedTypeList* userdefinedtypelist)
{
    _user_defined_type_list = userdefinedtypelist;
}

UserDefinedTypeList* uda::client::Client::user_defined_type_list()
{
    return _user_defined_type_list;
}

const LogStructList* uda::client::Client::log_struct_list() const
{
    return &_log_struct_list;
}

LogStructList* uda::client::Client::log_struct_list()
{
    return &_log_struct_list;
}

LogMallocList* uda::client::Client::log_malloc_list() const
{
    return _log_malloc_list;
}

void uda::client::Client::set_log_malloc_list(LogMallocList* logmalloclist)
{
    _log_malloc_list = logmalloclist;
}

void uda::client::Client::set_full_ntree(NTREE* full_ntree)
{
    _full_ntree = static_cast<NTree*>(full_ntree);
}

std::vector<UdaError>& uda::client::Client::error_stack()
{
    return _error_stack;
}

const std::vector<UdaError>& uda::client::Client::error_stack() const
{
    return _error_stack;
}

int uda::client::Client::put(std::string_view put_instruction, const PutDataBlock* put_data_block)
{
    RequestBlock request_block;

    const auto* signal_ptr = put_instruction.data();
    const auto* source_ptr = "";

    if (make_request_block(_error_stack, _config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (_error_stack.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(_error_stack, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    request_block[0].put = 1; // flags the direction of data (0 is default => get operation)
    request_block[0].putDataBlockList.push_back(*put_data_block);

    std::vector<int> indices(request_block.size());
    get_requests(request_block, indices.data());

    return indices[0];
}

int uda::client::Client::put(std::string_view put_instruction, const PutDataBlockList& put_data_block_list)
{
    RequestBlock request_block{};

    const auto* signal_ptr = put_instruction.data();
    const auto* source_ptr = "";

    if (make_request_block(_error_stack, _config, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (_error_stack.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(_error_stack, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
        }
        throw exceptions::ClientError("Error identifying the Data Source");
    }

    print_request_block(request_block);

    request_block[0].put = 1; // flags the direction of data (0 is default => get operation)
    request_block[0].putDataBlockList = put_data_block_list;

    std::vector<int> indices(request_block.size());
    get_requests(request_block, indices.data());

    return indices[0];
}

void uda::client::Client::close_all_connections()
{
    _connection.close_down(ClosedownType::CLOSE_ALL);
}

void uda::client::Client::close_sockets()
{
    _connection.close_down(ClosedownType::CLOSE_SOCKETS);
}
