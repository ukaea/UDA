#pragma once

#ifndef UDA_SERVER_SERVER_HPP
#define UDA_SERVER_SERVER_HPP

#include <vector>

#include "xdr_protocol.hpp"
#include "plugins.hpp"
#include "get_data.hpp"
#include "server_environment.hpp"

#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"
#include "cache/memcache.hpp"
#include "include/uda/export.h"

namespace uda {

struct MetadataBlock {
    uda::client_server::DataSource data_source;
    uda::client_server::Signal signal_rec;
    uda::client_server::SignalDesc signal_desc;
    uda::client_server::SystemConfig system_config;
    uda::client_server::DataSystem data_system;
};

class Server {
public:
    constexpr static int ServerVersion = 8;
    constexpr static int LegacyServerVersion = 6;

    LIBRARY_API Server();
    LIBRARY_API void run();
    LIBRARY_API void close();

private:
    void startup();
    void loop();
    int handle_request();
    int report_to_client();
    void handshake_client();
    void start_logs();
    int get_data(int* depth, uda::client_server::RequestData* request_data, uda::client_server::DataBlock* data_block, int protocol_version);
    int read_data(uda::client_server::RequestData* request, uda::client_server::DataBlock* data_block);

    std::vector<uda::client_server::UdaError> error_stack_;
    uda::client_server::RequestBlock request_block_;
    uda::client_server::ServerBlock server_block_;
    uda::client_server::ClientBlock client_block_;
    uda::client_server::Actions actions_desc_;
    uda::client_server::Actions actions_sig_;
    cache::UdaCache* cache_;
    server::Environment environment_;
    XdrProtocol protocol_;
    std::vector<uda::client_server::Sockets> sockets_;
    Plugins plugins_;
    bool server_closedown_ = false;
    int malloc_source_ = UDA_MALLOC_SOURCE_NONE;
    std::vector<uda::client_server::DataBlock> data_blocks_;
    size_t total_datablock_size_;
    MetadataBlock metadata_block_;
    int server_timeout_ = TIMEOUT;
    int server_tot_block_time_;
    bool fatal_error_ = false;
    uda::structures::LogMallocList* log_malloc_list_ = nullptr;
    uda::structures::UserDefinedTypeList* user_defined_type_list_ = nullptr;
    uda::structures::UserDefinedTypeList parsed_user_defined_type_list_;
};

} // namespace uda

#endif // UDA_SERVER_SERVER_HPP