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

// constexpr auto DefaultHost = "localhost";
// constexpr auto DefaultPort = 56565;

using XDR = __rpc_xdr;

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
    std::string log_dir;
    logging::LogLevel log_level = logging::LogLevel::UDA_LOG_NONE;
    logging::LogOpenMode open_mode = logging::LogOpenMode::APPEND;
    bool log_to_stdout = false;
    bool log_to_file = true;
};

// TODO: make interfaces mock-able (connection, config, and protocol) for unit testing

class Client
{
public:
    Client() noexcept;
    ~Client() = default;
    explicit Client(std::string_view config_path);

    Client(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&) = delete;

    void load_config(std::string_view path);
    void initialise_logging(const std::string& log_dir, logging::LogLevel log_level, uda::logging::LogOpenMode log_mode);
    inline void initialise_logging();
    void load_host_list(std::string_view file_path);

    int get(std::string_view data_signal, std::string_view data_source);
    std::vector<int> get(std::vector<std::pair<std::string, std::string>>& requests);

    int put(std::string_view put_instruction, const client_server::PutDataBlock* putdata_block);
    int put(std::string_view put_instruction, const client_server::PutDataBlockList& putdata_block_list);

    void set_host(std::string_view host);
    [[nodiscard]] const std::string& get_host() const {return _connection.get_host();}
    void set_port(int port);
    [[nodiscard]] int get_port() const {return _connection.get_port();}
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
    int version() const { return _version; }

protected:
    int get_requests(client_server::RequestBlock& request_block, int* indices);
    void concat_errors(client_server::ServerBlock& server_block) const;
    const char* get_server_error_stack_record_msg(int record);
    int get_server_error_stack_record_code(int record);

    void set_client_flags_from_config();
    void initialise_logging_from_config();

    // -------------------------------------------------
    //   configuration options and state
    // -------------------------------------------------

private:
    const int _version;

    // std::string _host = {};
    // int _port = 0;
    // uint32_t _flags = 0;
    int _alt_rank = 0;
    ClientFlags _client_flags = {};
    uint32_t _private_flags = 0;

    LoggingOptions _logging_options = {};

    std::string _client_username = "client";
    int _protocol_version;

    // -------------------------------------------------
    //   idam struct stuff
    // -------------------------------------------------

    client_server::ClientBlock _client_block = {};
    client_server::ServerBlock _server_block = {};
    std::vector<client_server::DataBlock> _data_blocks;
    cache::UdaCache* _cache = nullptr;
    std::vector<client_server::UdaError> _error_stack;
    client_server::MetaData _metadata = {};

    // -------------------------------------------------
    //   interface classes (mockable / testable?)
    // -------------------------------------------------

    config::Config _config;
    Connection _connection = {_error_stack};

    // -------------------------------------------------
    //   xdr stuff
    // -------------------------------------------------

    // required by protocol functions for data transport
    // NOTE: these are now owned by the socket struct in _connection
    // TODO: how to signal these are cached here, not owned
    mutable XDR* _client_input = nullptr;
    mutable XDR* _client_output = nullptr;

    // -------------------------------------------------
    //   legacy structured data stuff
    // -------------------------------------------------
    structures::UserDefinedTypeList* _user_defined_type_list = nullptr; // List of all known User Defined Structure Types
    structures::LogMallocList* _log_malloc_list = nullptr;             // List of all Heap Allocations for Data
    structures::NTree* _full_ntree = nullptr;
    structures::LogStructList _log_struct_list = {};
    int _malloc_source = UDA_MALLOC_SOURCE_NONE;

    int send_putdata(const client_server::RequestBlock& request_block);
    int send_request_block(client_server::RequestBlock& request_block);
    int send_client_block();
    int test_connection();
    int perform_handshake();
    int flush_sockets();
    int receive_server_block();
    int fetch_meta();
    int fetch_hierarchical_data(client_server::DataBlock* data_block);
    void new_socket_connection();
    void ensure_connection();
};

} // namespace uda::client


#endif // UDA_SOURCE_CLIENT2_CLIENT_H
