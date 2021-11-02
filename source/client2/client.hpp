#pragma once

#ifndef UDA_SOURCE_CLIENT2_CLIENT_H
#define UDA_SOURCE_CLIENT2_CLIENT_H

#include <exception>
#include <string_view>
#include <string>
#include <vector>
#include <rpc/rpc.h>

#include <clientserver/udaStructs.h>
#include <cache/memcache.hpp>

#include "connection.hpp"
#include "host_list.hpp"
#include "accAPI.h"

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

class Client
{
public:
    Client();
    int get(std::string_view data_signal, std::string_view data_source);
    std::vector<int> get(std::vector<std::pair<std::string, std::string>>& requests);
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
    int malloc_source_ = MALLOCSOURCENONE;
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
