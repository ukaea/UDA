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
#include <fstream>
#include <iostream>

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
}

} // namespace

uda::client::Client::Client() : version{ClientVersion}, protocol_version_{ClientVersion}, connection_{error_stack_}
{
    flags_ = 0;

    client_flags_ = {};
    client_flags_.alt_rank = 0;
    client_flags_.user_timeout = TimeOut;

    cache_ = uda::cache::open_cache();

    char username[StringLength];
    user_id(username);
    client_username_ = username;

    init_client_block(&client_block_, ClientVersion, client_username_.c_str());
}

uda::client::Client::Client(std::string_view config_path) : Client()
{
    // load config throws on error, no need to check config_ has read a valid file
    load_config(config_path);
    // connection_ = Connection(error_stack_, config_);
    connection_.load_config(config_);

    // TODO: this will throw if the config path is illegal
    // what behaviour do we want? ignore silently and fallback to default constructed client
    // or let it crash here?
    // config_.load(config_path);
    // connection_ = Connection(config_);
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
    // auto client_flags = config_.get("client.flags");
    // auto alt_rank = config_.get("client.alt_rank");
    //
    // if (client_flags) {
    //     flags_ |= client_flags.as<int>();
    // }
    //
    // if (alt_rank) {
    //     alt_rank_ = alt_rank.as<int>();
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
    //     auto log_level = static_cast<LogLevel>(config_.get("logging.level")
    //             .as_or_default(default_log_level));
    //
    //     if (log_level == UDA_LOG_NONE) {
    //         return;
    //     }
    //
    //     //---------------------------------------------------------------
    //     // Open the Log File
    //
    //     const auto log_dir = config_.get("logging.path").as_or_default(""s);
    //     const auto log_mode = config_.get("logging.mode").as_or_default("a"s);
    //     initialise_logging(log_dir, log_level, log_mode);
    //
    //     errno = 0;
    //     if (errno == 0)
    //     {
    //         config_.print();
    //     }
}

void uda::client::Client::load_config(std::string_view path)
{
    // load step throws on error
    config_.load(path);
    set_client_flags_from_config();
    initialise_logging_from_config();
    connection_.load_config(config_);
}

void uda::client::Client::set_client_flags_from_config()
{
    if (!config_) {
        return;
    }

    for (const auto& [key, value] : config_.get_section_as_map("client_flags")) {
        if (key == "timeout" and value) {
            client_flags_.user_timeout = value.as_or_default<int>(DefaultTimeout);
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
    auto log_level = static_cast<LogLevel>(config_.get("logging.level").as_or_default(default_log_level));

    if (log_level == UDA_LOG_NONE) {
        return;
    }
    logging_options_.log_level = log_level;

    //---------------------------------------------------------------
    // Open the Log File

    const auto log_dir = config_.get("logging.path").as_or_default<std::string>(""s);
    if (!log_dir.empty()) {
        logging_options_.log_dir = log_dir;
    }
    const auto log_mode = config_.get("logging.mode").as_or_default<std::string>("a"s);
    if (log_mode != "a"s) {
        logging_options_.open_mode = LogOpenMode::CLEAR;
    }

    errno = 0;

    initialise_logging(logging_options_.log_dir, logging_options_.log_level, logging_options_.open_mode);

    if (errno == 0) {
        config_.print();
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
        add_error(error_stack_, ErrorType::System, __func__, errno, "failed to open debug log");
        close_logging();
        return;
    }

    if (get_log_level() <= UDA_LOG_ERROR) {
        file_name = (std::filesystem::path(log_dir) / "Error.err").string();
        set_log_file(UDA_LOG_ERROR, file_name, log_mode);
    }

    if (errno != 0) {
        add_error(error_stack_, ErrorType::System, __func__, errno, "failed to open error log");
        close_logging();
        return;
    }
}

void uda::client::Client::initialise_logging()
{
    initialise_logging(logging_options_.log_dir, logging_options_.log_level, logging_options_.open_mode);
}

int uda::client::Client::fetch_meta()
{
    int err = 0;

#ifndef FATCLIENT // <========================== Client Server Code Only

    if ((err = protocol2(error_stack_, client_input_, ProtocolId::MetaData, XDRStreamDirection::Receive, nullptr,
                         log_malloc_list_, user_defined_type_list_, &metadata_, protocol_version_, &log_struct_list_,
                         private_flags_, malloc_source_)) != 0) {
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 4 Error (Data System)");
        return err;
    }
    print_meta_data(metadata_);

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
        if ((err = protocol2(error_stack_, client_input_, protocol_id, XDRStreamDirection::Receive, nullptr,
                             log_malloc_list_, user_defined_type_list_, data_block, protocol_version_,
                             &log_struct_list_, private_flags_, malloc_source_)) != 0) {
            add_error(error_stack_, ErrorType::Code, __func__, err,
                      "Client Side Protocol Error (Opaque Structure Type)");
            return err;
        }
    }

    return 0;
}

const char* uda::client::Client::get_server_error_stack_record_msg(int record)
{
    UDA_LOG(UDA_LOG_DEBUG, "record {}", record);
    UDA_LOG(UDA_LOG_DEBUG, "count  {}", server_block_.error_stack.size());
    if (record < 0 || static_cast<size_t>(record) >= server_block_.error_stack.size()) {
        return nullptr;
    }
    return server_block_.error_stack[record].msg; // Server Error Stack Record Message
}

int uda::client::Client::get_server_error_stack_record_code(int record)
{
    if (record < 0 || static_cast<size_t>(record) >= server_block_.error_stack.size()) {
        return 0;
    }
    return server_block_.error_stack[record].code; // Server Error Stack Record Code
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

void uda::client::Client::new_socket_connection()
{
    //TODO: replace client error stack with exceptions
    if (connection_.create() != 0) {
        int err = NO_SOCKET_CONNECTION;
        add_error(error_stack_, ErrorType::Code, __func__, err, "No Socket Connection to Server");
        throw uda::exceptions::ClientError("No socket connection to server");
    }

    connection_.register_new_xdr_streams();
    std::tie(client_input_, client_output_) = connection_.get_socket_xdr_streams();
    if (client_output_ == nullptr || client_input_ == nullptr) {
        throw uda::exceptions::ClientError("failed to open new XDR streams");
    }

}

void uda::client::Client::ensure_connection()
{
    if (connection_.open() and connection_.current_socket_timeout()) {
        auto age = connection_.get_current_socket_age();
        UDA_LOG(UDA_LOG_DEBUG, "Server Age Limit Reached {}", (long)age);
        UDA_LOG(UDA_LOG_DEBUG, "Server Closed and New Instance Started");
        // this call may have to change when we add ssl back in
        // closedown(ClosedownType::CLOSE_SOCKETS, &connection_);
        connection_.close_down(ClosedownType::CLOSE_SOCKETS);
    }
    if (connection_.reconnect_required()) {
        connection_.maybe_reuse_existing_socket();
    }
    if (connection_.open()){
        std::tie(client_input_, client_output_) = connection_.get_socket_xdr_streams();
        if (client_input_ == nullptr || client_output_ == nullptr)
        {
            add_error(error_stack_, ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
            connection_.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        if (client_output_->x_ops == nullptr || client_input_->x_ops == nullptr) {
            add_error(error_stack_, ErrorType::Code, __func__, 999, "XDR Streams are Closed!");
            UDA_LOG(UDA_LOG_DEBUG, "XDR Streams are Closed!");
            // this call may have to change when we add ssl back in
            // closedown(ClosedownType::CLOSE_SOCKETS, &connection_);
            connection_.close_down(ClosedownType::CLOSE_SOCKETS);
        }
    }

    if (!connection_.open()) {
        new_socket_connection();
    } else {
        xdrrec_eof(client_input_); // Flush input socket
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

    init_server_block(&server_block_, 0);
    error_stack_.clear();

    connection_.set_maximum_socket_age(client_flags_.user_timeout);
    auto age = connection_.get_current_socket_age();
    UDA_LOG(UDA_LOG_DEBUG, "Server Age: {}", age);
    ensure_connection();

    update_client_block(client_block_, client_flags_, private_flags_);
    print_client_block(client_block_);

    //-------------------------------------------------------------------------
    // Client and Server States at Startup only (1 RTT)
    // Will be passed during mutual authentication step

    if (connection_.startup_state) {
        perform_handshake();
        connection_.startup_state = false;
    }

    UDA_LOG(UDA_LOG_DEBUG, "Protocol Version {}", protocol_version_);
    UDA_LOG(UDA_LOG_DEBUG, "Client Version   {}", client_block_.version);
    UDA_LOG(UDA_LOG_DEBUG, "Server Version   {}", server_block_.version);

    int err = test_connection();

    err = send_client_block();
    err = send_request_block(request_block);
    err = send_putdata(request_block);

    err = flush_sockets();

    err = receive_server_block();

    // bool server_side = false;

    if (!server_block_.error_stack.empty()) {
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        err = server_block_.error_stack[0].code; // Problem on the Server Side!
        UDA_LOG(UDA_LOG_DEBUG, "Server Block passed Server Error State {}", err);
        // server_side = true;        // Most Server Side errors are benign so don't close the server
        return err;
    }

    if (client_block_.get_meta) {
        if ((err = fetch_meta()) != 0) {
            return err;
        }
    }

    //------------------------------------------------------------------------------
    // Fetch the data Block

    std::vector<DataBlock> recv_data_block_list;

    try
    {

        if ((err = protocol2(error_stack_, client_input_, ProtocolId::DataBlockList, XDRStreamDirection::Receive, nullptr,
                        log_malloc_list_, user_defined_type_list_, &recv_data_block_list, protocol_version_,
                        &log_struct_list_, private_flags_, malloc_source_)) != 0) {
            UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");

            add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
            throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
        }
    }
    catch (std::exception& e)
    {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 2 Error (Failure Receiving Data Block)");
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 2 Error (Failure Receiving Data Block)");
        throw uda::exceptions::ClientError("Protocol 2 Error (Failure Receiving Data Block)");
        // throw e.what();
    }

    print_data_block_list(recv_data_block_list);

    bool data_received = false;
    std::vector<int> data_block_indices(request_block.size());

    for (size_t i = 0; i < request_block.size(); ++i) {
        data_blocks_.emplace_back(DataBlock{});
        auto data_block_idx = data_blocks_.size() - 1;
        DataBlock* data_block = &data_blocks_.back();

        copy_data_block(data_block, &recv_data_block_list[i]);
        copy_client_block(&data_block->client_block, &client_flags_);

        if (client_block_.get_meta) {
            data_block->meta_data = metadata_;
        }

        fetch_hierarchical_data(data_block);

        //------------------------------------------------------------------------------
        // Cache the data if the server has passed permission and the application (client) has enabled caching
        if (client_flags_.flags & client_flags::FileCache) {
            cache::udaFileCacheWrite(error_stack_, data_block, &request_block, log_malloc_list_,
                                     user_defined_type_list_, protocol_version_, &log_struct_list_, private_flags_,
                                     malloc_source_);
        }

        if (cache_ != nullptr && client_flags_.flags & client_flags::Cache) {
            cache_write(config_, cache_, &request_block[i], data_block, log_malloc_list_,
                        user_defined_type_list_, protocol_version_, client_flags_.flags, &log_struct_list_,
                        private_flags_, malloc_source_);
        }

        data_block_indices[i] = data_block_idx;
        data_received = true;
    }

    if (data_received) {
        if (err != 0) {
            // Close Socket & XDR Streams but Not Files
            // closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input_, client_output_, &reopen_logs_);
            connection_.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        for (auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &data_blocks_[data_block_idx];

            if (err == 0 && (get_data_status(data_block) == MIN_STATUS) && !client_flags_.get_bad) {
                // If Data are not usable, flag the client
                add_error(error_stack_, ErrorType::Code, __func__, DATA_STATUS_BAD,
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

        concat_errors(server_block_);
        error_stack_.clear();
        print_errors(server_block_.error_stack, client_block_, request_block);

        //------------------------------------------------------------------------------
        // Copy Most Significant Error Stack Message to the Data Block if a Handle was Issued

        for (const auto data_block_idx : data_block_indices) {
            DataBlock* data_block = &data_blocks_[data_block_idx];

            if (data_block->errcode == 0 && !server_block_.error_stack.empty()) {
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
            // closedown(ClosedownType::CLOSE_SOCKETS, nullptr, client_input_, client_output_, &reopen_logs_);
            connection_.close_down(ClosedownType::CLOSE_SOCKETS);
        }

        concat_errors(server_block_);
        error_stack_.clear();
        print_errors(server_block_.error_stack, client_block_, request_block);

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
            if ((err = protocol2(error_stack_, client_output_, protocol_id, XDRStreamDirection::Send, nullptr,
                                 log_malloc_list_, user_defined_type_list_, const_cast<PutDataBlockList*>(&request->putDataBlockList),
                                 protocol_version_, &log_struct_list_, private_flags_, malloc_source_)) != 0) {
                add_error(error_stack_, ErrorType::Code, __func__, err,
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

    if (make_request_block(error_stack_, config_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (error_stack_.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source [{}]", data_source);
            add_error(error_stack_, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
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

    if (make_request_block(error_stack_, config_, signals.data(), sources.data(), requests.size(), &request_block) !=
        0) {
        if (error_stack_.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(error_stack_, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
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
    connection_.set_host(host);
}

void uda::client::Client::load_host_list(std::string_view file_path)
{
    connection_.load_host_list(file_path);
}

void uda::client::Client::set_port(int port)
{
    connection_.set_port(port);
}

int uda::client::Client::test_connection()
{
    int rc = 0;
    if (!(rc = xdrrec_eof(client_input_))) { // Test for an EOF

        UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} => more input when none expected!", rc);

        int count = 0;
        char temp;

        do {
            rc = xdr_char(client_input_, &temp); // Flush the input (limit to 64 bytes)

            if (rc) {
                UDA_LOG(UDA_LOG_DEBUG, "[{}] [{}]", count++, temp);
            }
        } while (rc && count < 64);

        if (count > 0) { // Error if data is waiting
            add_error(
                error_stack_, ErrorType::Code, __func__, 999,
                "Data waiting in the input data buffer when none expected! Please contact the system administrator.");
            UDA_LOG(UDA_LOG_DEBUG, "[{}] excess data bytes waiting in input buffer!", count++);
            throw uda::exceptions::ClientError(
                "Data waiting in the input data buffer when none expected! Please contact the system administrator.");
        }

        rc = xdrrec_eof(client_input_); // Test for an EOF

        if (!rc) {
            rc = xdrrec_skiprecord(client_input_); // Flush the input buffer (Zero data waiting but not an EOF!)
        }

        rc = xdrrec_eof(client_input_); // Test for an EOF

        if (!rc) {
            int err = 999;
            add_error(error_stack_, ErrorType::Code, __func__, err,
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
    if ((err = protocol2(error_stack_, client_output_, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list_,
                         user_defined_type_list_, &request_block, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 1 Error (Request Block)");
        throw uda::exceptions::ClientError("Protocol 1 Error (Request Block)");
    }

    return 0;
}

int uda::client::Client::send_client_block()
{
    ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block
    int err = 0;
    if ((err = protocol2(error_stack_, client_output_, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list_,
                         user_defined_type_list_, &client_block_, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    return 0;
}

int uda::client::Client::perform_handshake()
{
    // Flush (mark as at EOF) the input socket buffer (before the exchange begins)

    ProtocolId protocol_id = ProtocolId::ClientBlock; // Send Client Block (proxy for authenticationStep = 6)

    int err = 0;
    if ((err = protocol2(error_stack_, client_output_, protocol_id, XDRStreamDirection::Send, nullptr, log_malloc_list_,
                         user_defined_type_list_, &client_block_, protocol_version_, &log_struct_list_, private_flags_,
                         malloc_source_)) != 0) {
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 10 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error Sending Client Block");
        throw uda::exceptions::ClientError("Protocol 10 Error (Client Block)");
    }

    if (!(xdrrec_endofrecord(client_output_, 1))) { // Send data now
        err = (int)ProtocolError::Error7;
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 7 Error (Client Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_endofrecord after Client Block");
        throw uda::exceptions::ClientError("Protocol 7 Error (Client Block)");
    }

    // Flush (mark as at EOF) the input socket buffer (start of wait for data)
    // int rc = xdrrec_eof(client_input_);

    // Wait for data, then position buffer reader to the start of a new record
    if (!(xdrrec_skiprecord(client_input_))) {
        err = static_cast<int>(ProtocolError::Error5);
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 5 Error (Server Block)");
        UDA_LOG(UDA_LOG_DEBUG, "Error xdrrec_skiprecord prior to Server Block");
        throw exceptions::ClientError("Protocol 5 Error (Server Block)");
    }

    protocol_id =
        ProtocolId::ServerBlock; // Receive Server Block: Server Aknowledgement (proxy for authenticationStep = 8)

    if ((err = protocol2(error_stack_, client_input_, protocol_id, XDRStreamDirection::Receive, nullptr,
                         log_malloc_list_, user_defined_type_list_, &server_block_, protocol_version_,
                         &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 11 Error (Server Block #1)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid concatenation problems
        server_block_.error_stack.clear();
        UDA_LOG(UDA_LOG_DEBUG, "Error receiving Server Block");
        throw exceptions::ClientError("Protocol 11 Error (Server Block #1)");
    }

    // Flush (mark as at EOF) the input socket buffer (not all server state data may have been read - version dependent)

    int rc = xdrrec_eof(client_input_);

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
    UDA_LOG(UDA_LOG_DEBUG, "xdrrec_eof rc = {} [1 => no more input]", rc);
    print_server_block(server_block_);

    // Protocol Version: Lower of the client and server version numbers
    // This defines the set of elements within data structures passed between client and server
    // Must be the same on both sides of the socket
    protocol_version_ =
        std::min(get_protocol_version(client_block_.version), get_protocol_version(server_block_.version));

    if (!server_block_.error_stack.empty()) {
        err = server_block_.error_stack[0].code; // Problem on the Server Side!
        throw exceptions::ServerError(server_block_.error_stack[0].msg);
    }

    return 0;
}

int uda::client::Client::flush_sockets()
{
    //------------------------------------------------------------------------------
    // Send the Full TCP packet and wait for the returned data

    int rc = 0;
    if (!(rc = xdrrec_endofrecord(client_output_, 1))) {
        int err = (int)ProtocolError::Error7;
        add_error(error_stack_, ErrorType::Code, __func__, err, "Protocol 7 Error (Request Block & putDataBlockList)");
        return err;
    }

    UDA_LOG(UDA_LOG_DEBUG, "****** Outgoing tcp packet sent without error. Waiting for data.");

    if (!xdrrec_skiprecord(client_input_)) {
        int err = (int)ProtocolError::Error5;
        add_error(error_stack_, ErrorType::Code, __func__, err, " Protocol 5 Error (Server & Data Structures)");
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
    if ((err = protocol2(error_stack_, client_input_, ProtocolId::ServerBlock, XDRStreamDirection::Receive, nullptr,
                         log_malloc_list_, user_defined_type_list_, &server_block_, protocol_version_,
                         &log_struct_list_, private_flags_, malloc_source_)) != 0) {
        UDA_LOG(UDA_LOG_DEBUG, "Protocol 11 Error (Server Block #2) = {}", err);
        add_error(error_stack_, ErrorType::Code, __func__, err, " Protocol 11 Error (Server Block #2)");
        // Assuming the server_block is corrupted, replace with a clean copy to avoid future concatonation problems
        server_block_.error_stack.clear();
        throw uda::exceptions::ClientError("Protocol 11 Error (Server Block #2) = {}", err);
    }

    UDA_LOG(UDA_LOG_DEBUG, "Server Block Received");
    print_server_block(server_block_);

    return 0;
}

void uda::client::Client::clear()
{
    data_blocks_.clear();
}

int uda::client::Client::new_handle()
{
    data_blocks_.emplace_back(DataBlock{});
    return static_cast<int>(data_blocks_.size() - 1);
}

void uda::client::Client::free_handle(int handle_idx)
{
    if (handle_idx < 0 || handle_idx >= static_cast<int>(data_blocks_.size())) {
        UDA_LOG(UDA_LOG_ERROR, "Invalid handle index");
        return;
    }
    data_blocks_.at(handle_idx) = {};
}

void uda::client::Client::free_all()
{
    data_blocks_.clear();
}

void uda::client::Client::set_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        private_flags_ |= flag;
    } else {
        client_flags_.flags |= flag;
    }
}

void uda::client::Client::reset_flag(unsigned int flag, bool private_flag)
{
    if (private_flag) {
        private_flags_ &= !flag;
    } else {
        client_flags_.flags &= !flag;
    }
}

void uda::client::Client::set_property(const char* property)
{
    // User settings for Client and Server behaviour

    char name[56];
    char* value;

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            client_flags_.get_datadble = 1;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            client_flags_.get_dimdble = 1;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            client_flags_.get_timedble = 1;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            client_flags_.get_bytes = 1;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            client_flags_.get_bad = 1;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            client_flags_.get_meta = 1;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            client_flags_.get_asis = 1;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            client_flags_.get_uncal = 1;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            client_flags_.get_notoff = 1;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            client_flags_.get_synthetic = 1;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            client_flags_.get_scalar = 1;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            client_flags_.get_nodimdata = 1;
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
                    client_flags_.user_timeout = atoi(value);
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
                client_flags_.flags = client_flags_.flags | client_flags::AltData;
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
                        client_flags_.alt_rank = atoi(value);
                    }
                }
            }
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            client_flags_.flags = client_flags_.flags | client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            client_flags_.flags = client_flags_.flags | client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            client_flags_.flags = client_flags_.flags | client_flags::FileCache;
        }
    }
}

int uda::client::Client::get_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            return client_flags_.get_datadble;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            return client_flags_.get_dimdble;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            return client_flags_.get_timedble;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            return client_flags_.get_bytes;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            return client_flags_.get_bad;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            return client_flags_.get_meta;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            return client_flags_.get_asis;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            return client_flags_.get_uncal;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            return client_flags_.get_notoff;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            return client_flags_.get_synthetic;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            return client_flags_.get_scalar;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            return client_flags_.get_nodimdata;
        }
    } else {
        if (STR_IEQUALS(property, "timeout")) {
            return client_flags_.user_timeout;
        }
        if (STR_IEQUALS(property, "altRank")) {
            return alt_rank_;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            return static_cast<int>(client_flags_.flags & client_flags::ReuseLastHandle);
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            return static_cast<int>(client_flags_.flags & client_flags::FreeReuseLastHandle);
        }
        if (STR_IEQUALS(property, "verbose")) {
            return get_log_level() == UDA_LOG_INFO;
        }
        if (STR_IEQUALS(property, "debug")) {
            return get_log_level() == UDA_LOG_DEBUG;
        }
        if (STR_IEQUALS(property, "altData")) {
            return static_cast<int>(client_flags_.flags & client_flags::AltData);
        }
        if (STR_IEQUALS(property, "fileCache")) {
            return static_cast<int>(client_flags_.flags & client_flags::FileCache);
        }
    }
    return 0;
}

void uda::client::Client::reset_property(const char* property)
{
    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            client_flags_.get_datadble = 0;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            client_flags_.get_dimdble = 0;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            client_flags_.get_timedble = 0;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            client_flags_.get_bytes = 0;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            client_flags_.get_bad = 0;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            client_flags_.get_meta = 0;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            client_flags_.get_asis = 0;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            client_flags_.get_uncal = 0;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            client_flags_.get_notoff = 0;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            client_flags_.get_synthetic = 0;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            client_flags_.get_scalar = 0;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            client_flags_.get_nodimdata = 0;
        }
    } else {
        if (STR_IEQUALS(property, "verbose")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "debug")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "altData")) {
            client_flags_.flags &= !client_flags::AltData;
        }
        if (STR_IEQUALS(property, "altRank")) {
            client_flags_.alt_rank = 0;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            client_flags_.flags &= !client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            client_flags_.flags &= !client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            client_flags_.flags &= !client_flags::FileCache;
        }
    }
}

void uda::client::Client::reset_properties()
{
    // Reset on Both Client and Server

    client_flags_.get_datadble = 0;
    client_flags_.get_dimdble = 0;
    client_flags_.get_timedble = 0;
    client_flags_.get_bad = 0;
    client_flags_.get_meta = 0;
    client_flags_.get_asis = 0;
    client_flags_.get_uncal = 0;
    client_flags_.get_notoff = 0;
    client_flags_.get_synthetic = 0;
    client_flags_.get_scalar = 0;
    client_flags_.get_bytes = 0;
    client_flags_.get_nodimdata = 0;
    set_log_level(UDA_LOG_NONE);
    client_flags_.user_timeout = TimeOut;
    client_flags_.flags = 0;
    client_flags_.alt_rank = 0;
}

const DataBlock* uda::client::Client::data_block(int handle) const
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < data_blocks_.size()) {
        return &data_blocks_[idx];
    } else {
        return nullptr;
    }
}

DataBlock* uda::client::Client::data_block(int handle)
{
    const auto idx = static_cast<size_t>(handle);
    if (idx < data_blocks_.size()) {
        return &data_blocks_[idx];
    } else {
        return nullptr;
    }
}

const ClientBlock* uda::client::Client::client_block(int handle) const
{
    return &client_block_;
}

void uda::client::Client::concat_errors(ServerBlock& server_block) const
{
    if (error_stack_.empty()) {
        return;
    }

    for (auto& error : error_stack_) {
        server_block.error_stack.push_back(error);
    }
}

const uda::client::ClientFlags* uda::client::Client::client_flags() const
{
    return &client_flags_;
}

const DataBlock* uda::client::Client::current_data_block() const
{
    if (!data_blocks_.empty()) {
        return &data_blocks_.back();
    } else {
        return nullptr;
    }
}

const ServerBlock* uda::client::Client::server_block() const
{
    return &server_block_;
}

void uda::client::Client::set_user_defined_type_list(UserDefinedTypeList* userdefinedtypelist)
{
    user_defined_type_list_ = userdefinedtypelist;
}

UserDefinedTypeList* uda::client::Client::user_defined_type_list()
{
    return user_defined_type_list_;
}

const LogStructList* uda::client::Client::log_struct_list() const
{
    return &log_struct_list_;
}

LogStructList* uda::client::Client::log_struct_list()
{
    return &log_struct_list_;
}

LogMallocList* uda::client::Client::log_malloc_list() const
{
    return log_malloc_list_;
}

void uda::client::Client::set_log_malloc_list(LogMallocList* logmalloclist)
{
    log_malloc_list_ = logmalloclist;
}

void uda::client::Client::set_full_ntree(NTREE* full_ntree)
{
    full_ntree_ = static_cast<NTree*>(full_ntree);
}

std::vector<UdaError>& uda::client::Client::error_stack()
{
    return error_stack_;
}

const std::vector<UdaError>& uda::client::Client::error_stack() const
{
    return error_stack_;
}

int uda::client::Client::put(std::string_view put_instruction, PutDataBlock* put_data_block)
{
    RequestBlock request_block;

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(error_stack_, config_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (error_stack_.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(error_stack_, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
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

int uda::client::Client::put(std::string_view put_instruction, PutDataBlockList& put_data_block_list)
{
    RequestBlock request_block{};

    auto signal_ptr = put_instruction.data();
    auto source_ptr = "";

    if (make_request_block(error_stack_, config_, &signal_ptr, &source_ptr, 1, &request_block) != 0) {
        if (error_stack_.empty()) {
            UDA_LOG(UDA_LOG_ERROR, "Error identifying the Data Source");
            add_error(error_stack_, ErrorType::Code, __func__, 999, "Error identifying the Data Source");
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
    connection_.close_down(ClosedownType::CLOSE_ALL);
}

void uda::client::Client::close_sockets()
{
    connection_.close_down(ClosedownType::CLOSE_SOCKETS);
}
