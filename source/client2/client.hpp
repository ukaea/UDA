#pragma once

#ifndef UDA_SOURCE_CLIENT2_CLIENT_H
#define UDA_SOURCE_CLIENT2_CLIENT_H

#include <exception>
#include <string_view>
#include <string>
#include <vector>
#include <rpc/rpc.h>

#include "clientserver/udaStructs.h"
#include "cache/memcache.hpp"

#include "connection.hpp"
#include "host_list.hpp"

constexpr auto DefaultHost = "localhost";
constexpr auto DefaultPort = 56565;

namespace uda {
namespace client {

constexpr int ClientVersion = 8;

struct MetadataBlock {
    DATA_SOURCE data_source;
    SIGNAL signal_rec;
    SIGNAL_DESC signal_desc;
    SYSTEM_CONFIG system_config;
    DATA_SYSTEM data_system;
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

    int put(std::string_view put_instruction, PUTDATA_BLOCK* putdata_block);
    int put(std::string_view put_instruction, PUTDATA_BLOCK_LIST* putdata_block_list);

    void set_host(std::string_view host);
    void set_port(int port);
    void clear();
    DATA_BLOCK* current_data_block();
    DATA_BLOCK* data_block(int handle);
    int new_handle();
    void set_flag(unsigned int flag, bool private_flag=false);
    void reset_flag(unsigned int flag, bool private_flag=false);
    void set_property(const char* property);
    int get_property(const char* property);
    void reset_property(const char* property);
    void reset_properties();
    const CLIENT_BLOCK* client_block(int handle);
    const CLIENT_FLAGS* client_flags();
    const SERVER_BLOCK* server_block();
    ENVIRONMENT* environment();
    void set_user_defined_type_list(USERDEFINEDTYPELIST* userdefinedtypelist);
    void set_log_malloc_list(LOGMALLOCLIST* logmalloclist);
    void set_full_ntree(NTREE* full_ntree);

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

private:
    int get_requests(RequestBlock& request_block, int* indices);
    void concat_errors(UDA_ERROR_STACK* error_stack);
    const char* get_server_error_stack_record_msg(int record);
    int get_server_error_stack_record_code(int record);

    std::string host_ = {};
    int port_ = 0;
    uint32_t flags_ = 0;
    int alt_rank_ = 0;
    ENVIRONMENT environment_ = {};
    ClientFlags client_flags_ = {};
    uint32_t private_flags_= 0;
    ClientBlock client_block_ = {};
    ServerBlock server_block_ = {};
    std::vector<DataBlock> data_blocks_ = {};
    uda::cache::UdaCache* cache_ = nullptr;
    std::vector<UDA_ERROR> error_stack_ = {};
    XDR* client_input_ = nullptr;
    XDR* client_output_ = nullptr;
    Connection connection_;
    HostList host_list_ = {};
    IoData io_data_ = {};
    bool env_host_ = false;
    bool env_port_ = false;
    bool reopen_logs_ = false;
    std::string client_username_ = "client";
    int protocol_version_ = ClientVersion;
    USERDEFINEDTYPELIST* userdefinedtypelist_ = nullptr;            // List of all known User Defined Structure Types
    LOGMALLOCLIST* logmalloclist_ = nullptr;                        // List of all Heap Allocations for Data
    NTREE* full_ntree_ = nullptr;
    LOGSTRUCTLIST log_struct_list_ = {};
    int malloc_source_ = UDA_MALLOC_SOURCE_NONE;
    MetadataBlock metadata_ = {};

    int send_putdata(const RequestBlock& request_block);
    int send_request_block(RequestBlock& request_block);
    int send_client_block();
    int test_connection();
    int perform_handshake();
    int flush_sockets();
    int receive_server_block();
    int fetch_meta();
    int fetch_hierarchical_data(DATA_BLOCK* data_block);
};

}
}

#endif //UDA_SOURCE_CLIENT2_CLIENT_H
