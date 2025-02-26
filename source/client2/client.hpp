#pragma once

#ifndef UDA_SOURCE_CLIENT2_CLIENT_H
#define UDA_SOURCE_CLIENT2_CLIENT_H

#include <string_view>
#include <string>
#include <vector>

#include "clientserver/uda_structs.h"
#include "cache/memcache.hpp"
#include "config/config.h"
#include "logging/logging.h"

#include "connection.hpp"
#include "host_list.hpp"

// constexpr auto DefaultHost = "localhost";
// constexpr auto DefaultPort = 56565;

using XDR = struct __rpc_xdr;

namespace uda::client {

constexpr int DefaultTimeout = 600;

// constexpr int ProtocolVersion = 8;
// constexpr int ClientVersion = UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);

struct ClientFlags {
    int get_dimdble;  // (Server Side) Return Dimensional Data in Double Precision
    int get_timedble; // (Server Side) Server Side cast of time dimension to double precision if in compresed format
    int get_scalar;   // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
    int get_bytes;    // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
    // calibration factor applied
    int get_meta;     // (Server Side) return All Meta Data
    int get_asis;     // (Server Side) Apply no XML based corrections to Data or Dimensions
    int get_uncal;    // (Server Side) Apply no XML based Calibrations to Data
    int get_notoff;   // (Server Side) Apply no XML based Timing Corrections to Data
    int get_nodimdata;

    int get_datadble;  // (Client Side) Return Data in Double Precision
    int get_bad;       // (Client Side) return data with BAD Status value
    int get_synthetic; // (Client Side) Return Synthetic Data if available instead of Original data

    uint32_t flags;

    int user_timeout;
    int alt_rank;
};

struct LoggingOptions
{
    std::string log_dir = "";
    uda::logging::LogLevel log_level = uda::logging::LogLevel::UDA_LOG_NONE;
    uda::logging::LogOpenMode open_mode = uda::logging::LogOpenMode::APPEND;
    bool log_to_stdout = false;
    bool log_to_file = true;
};

// TODO: make interfaces mock-able (connection, config, and protocol) for unit testing

class Client
{
public:
    Client();
    ~Client() = default;
    explicit Client(std::string_view config_path);

    void load_config(std::string_view path);
    void initialise_logging(const std::string& log_dir, logging::LogLevel log_level, uda::logging::LogOpenMode log_mode);
    inline void initialise_logging();
    void load_host_list(std::string_view file_path);

    int get(std::string_view data_signal, std::string_view data_source);
    std::vector<int> get(std::vector<std::pair<std::string, std::string>>& requests);

    int put(std::string_view put_instruction, client_server::PutDataBlock* putdata_block);
    int put(std::string_view put_instruction, client_server::PutDataBlockList& putdata_block_list);

    void set_host(std::string_view host);
    [[nodiscard]] const std::string& get_host() const {return connection_.get_host();}
    void set_port(int port);
    [[nodiscard]] int get_port() const {return connection_.get_port();}
    void clear();
    [[nodiscard]] const client_server::DataBlock* current_data_block() const;
    [[nodiscard]] const client_server::DataBlock* data_block(int handle) const;
    client_server::DataBlock* data_block(int handle);
    void free_handle(int handle_idx);
    void free_all();
    int new_handle();
    void set_flag(unsigned int flag, bool private_flag=false);
    void reset_flag(unsigned int flag, bool private_flag=false);
    void set_property(const char* property);
    int get_property(const char* property);
    void reset_property(const char* property);
    void reset_properties();
    [[nodiscard]] const client_server::ClientBlock* client_block(int handle) const;
    [[nodiscard]] const ClientFlags* client_flags() const;
    [[nodiscard]] const client_server::ServerBlock* server_block() const;
    void set_user_defined_type_list(structures::UserDefinedTypeList* userdefinedtypelist);
    structures::UserDefinedTypeList* user_defined_type_list();
    [[nodiscard]] structures::LogMallocList* log_malloc_list() const;
    [[nodiscard]] const structures::LogStructList* log_struct_list() const;
    [[nodiscard]] structures::LogStructList* log_struct_list();
    void set_log_malloc_list(structures::LogMallocList* logmalloclist);
    void set_full_ntree(NTREE* full_ntree);
    [[nodiscard]] std::vector<client_server::UdaError>& error_stack();
    [[nodiscard]] const std::vector<client_server::UdaError>& error_stack() const;
    void close_all_connections();
    void close_sockets();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    const int version;

private:
    int get_requests(client_server::RequestBlock& request_block, int* indices);
    void concat_errors(client_server::ServerBlock& server_block) const;
    const char* get_server_error_stack_record_msg(int record);
    int get_server_error_stack_record_code(int record);

    void set_client_flags_from_config();
    void initialise_logging_from_config();
    void set_host_list_from_config();

    // -------------------------------------------------
    //   configuration options and state
    // -------------------------------------------------

    // std::string _host = {};
    // int _port = 0;
    uint32_t flags_ = 0;
    int alt_rank_ = 0;
    ClientFlags client_flags_ = {};
    uint32_t private_flags_ = 0;

    LoggingOptions logging_options_ = {};

    std::string client_username_ = "client";
    int protocol_version_;

    // -------------------------------------------------
    //   idam struct stuff
    // -------------------------------------------------

    client_server::ClientBlock client_block_ = {};
    client_server::ServerBlock server_block_ = {};
    std::vector<client_server::DataBlock> data_blocks_ = {};
    cache::UdaCache* cache_ = nullptr;
    std::vector<client_server::UdaError> error_stack_ = {};
    client_server::MetaData metadata_ = {};

    // -------------------------------------------------
    //   interface classes (mockable / testable?)
    // -------------------------------------------------

    config::Config config_ = {};
    Connection connection_ = {error_stack_};
    HostList host_list_ = {};

    // -------------------------------------------------
    //   xdr stuff
    // -------------------------------------------------

    // required by protocol functions for data transport
    // and by the connection methods for establishing connections
    XDR* client_input_ = nullptr;
    XDR* client_output_ = nullptr;

    // connection.io_data -> IoData{&connection._client_socket}
    // required by createXDRStream only? need to cache?
    IoData io_data_ = {};

    // -------------------------------------------------
    //   don't know
    // -------------------------------------------------

    // reopen_flags set after server timeout, but nothing ever done with it
    bool reopen_logs_ = false;

    //TODO: _env_var flags never used. intention?
    // bool _env_host = false;
    // bool _env_port = false;

    // -------------------------------------------------
    //   legacy structured data stuff
    // -------------------------------------------------
    structures::UserDefinedTypeList* user_defined_type_list_ = nullptr; // List of all known User Defined Structure Types
    structures::LogMallocList* log_malloc_list_ = nullptr;             // List of all Heap Allocations for Data
    structures::NTree* full_ntree_ = nullptr;
    structures::LogStructList log_struct_list_ = {};
    int malloc_source_ = UDA_MALLOC_SOURCE_NONE;

    int send_putdata(const client_server::RequestBlock& request_block);
    int send_request_block(client_server::RequestBlock& request_block);
    int send_client_block();
    int test_connection();
    int perform_handshake();
    int flush_sockets();
    int receive_server_block();
    int fetch_meta();
    int fetch_hierarchical_data(client_server::DataBlock* data_block);
};

} // namespace uda::client


#endif // UDA_SOURCE_CLIENT2_CLIENT_H
