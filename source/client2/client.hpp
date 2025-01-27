#pragma once

#ifndef UDA_SOURCE_CLIENT2_CLIENT_H
#define UDA_SOURCE_CLIENT2_CLIENT_H

#include <exception>
#include <string_view>
#include <string>
#include <vector>

#include "clientserver/udaStructs.h"
#include "cache/memcache.hpp"
#include "config/config.h"
#include "clientserver/version.h"

#include "connection.hpp"
#include "host_list.hpp"

constexpr auto DefaultHost = "localhost";
constexpr auto DefaultPort = 56565;

typedef struct __rpc_xdr XDR;

namespace uda {
namespace client {

// constexpr int ProtocolVersion = 8;
// constexpr int ClientVersion = UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);


struct MetadataBlock {
    uda::client_server::DataSource data_source;
    uda::client_server::Signal signal_rec;
    uda::client_server::SignalDesc signal_desc;
    uda::client_server::SystemConfig system_config;
    uda::client_server::DataSystem data_system;
};

typedef struct ClientFlags {
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
} CLIENT_FLAGS;

class Client
{
public:
    Client();

    int get(std::string_view data_signal, std::string_view data_source);
    std::vector<int> get(std::vector<std::pair<std::string, std::string>>& requests);

    int put(std::string_view put_instruction, uda::client_server::PutDataBlock* putdata_block);
    int put(std::string_view put_instruction, uda::client_server::PutDataBlockList* putdata_block_list);

    void set_host(std::string_view host);
    inline const std::string& get_host() const {return _host;}
    void set_port(int port);
    inline int get_port() const {return _port;}
    void clear();
    uda::client_server::DataBlock* current_data_block();
    uda::client_server::DataBlock* data_block(int handle);
    int new_handle();
    void set_flag(unsigned int flag, bool private_flag=false);
    void reset_flag(unsigned int flag, bool private_flag=false);
    void set_property(const char* property);
    int get_property(const char* property);
    void reset_property(const char* property);
    void reset_properties();
    const uda::client_server::ClientBlock* client_block(int handle);
    const CLIENT_FLAGS* client_flags();
    const uda::client_server::ServerBlock* server_block();
    void set_user_defined_type_list(uda::structures::UserDefinedTypeList* userdefinedtypelist);
    void set_log_malloc_list(uda::structures::LogMallocList* logmalloclist);
    void set_full_ntree(NTREE* full_ntree);

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    const int version;

private:
    int get_requests(uda::client_server::RequestBlock& request_block, int* indices);
    void concat_errors(uda::client_server::ErrorStack* error_stack);
    const char* get_server_error_stack_record_msg(int record);
    int get_server_error_stack_record_code(int record);

    std::string _host = {};
    int _port = 0;
    uint32_t _flags = 0;
    int _alt_rank = 0;
    ClientFlags _client_flags = {};
    uint32_t _private_flags = 0;
    uda::client_server::ClientBlock _client_block = {};
    uda::client_server::ServerBlock _server_block = {};
    std::vector<uda::client_server::DataBlock> _data_blocks = {};
    uda::cache::UdaCache* _cache = nullptr;
    std::vector<uda::client_server::UdaError> _error_stack = {};
    XDR* _client_input = nullptr;
    XDR* _client_output = nullptr;
    config::Config _config = {};
    Connection _connection;
    HostList _host_list = {};
    IoData _io_data = {};
    bool _env_host = false;
    bool _env_port = false;
    bool _reopen_logs = false;
    std::string _client_username = "client";
    int _protocol_version;
    uda::structures::UserDefinedTypeList* _userdefinedtypelist = nullptr; // List of all known User Defined Structure Types
    uda::structures::LogMallocList* _logmalloclist = nullptr;             // List of all Heap Allocations for Data
    uda::structures::NTree* _full_ntree = nullptr;
    uda::structures::LogStructList _log_struct_list = {};
    int _malloc_source = UDA_MALLOC_SOURCE_NONE;
    MetadataBlock _metadata = {};
    bool _server_reconnect = false;
    bool _server_change_sockets = false;

    int send_putdata(const uda::client_server::RequestBlock& request_block);
    int send_request_block(uda::client_server::RequestBlock& request_block);
    int send_client_block();
    int test_connection();
    int perform_handshake();
    int flush_sockets();
    int receive_server_block();
    int fetch_meta();
    int fetch_hierarchical_data(uda::client_server::DataBlock* data_block);
};

} // namespace client
} // namespace uda

#endif // UDA_SOURCE_CLIENT2_CLIENT_H
