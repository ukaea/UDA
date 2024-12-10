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
#include "clientserver/export.h"

namespace uda {

struct MetadataBlock {
    DataSource data_source;
    Signal signal_rec;
    SignalDesc signal_desc;
    SystemConfig system_config;
    DataSystem data_system;
};

class Server {
public:
    constexpr static int ServerVersion = 10;
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
    int get_data(int* depth, RequestData* request_data, DataBlock* data_block, int protocol_version);
    int read_data(RequestData* request, DATA_BLOCK* data_block);

    std::vector<UDA_ERROR> error_stack_;
    RequestBlock request_block_;
    ServerBlock server_block_;
    ClientBlock client_block_;
    Actions actions_desc_;
    Actions actions_sig_;
    cache::UdaCache* cache_;
    server::Environment environment_;
    XdrProtocol protocol_;
    std::vector<Sockets> sockets_;
    Plugins plugins_;
    bool server_closedown_ = false;
    int malloc_source_ = UDA_MALLOC_SOURCE_NONE;
    std::vector<DATA_BLOCK> data_blocks_;
    size_t total_datablock_size_;
    MetadataBlock metadata_block_;
    int server_timeout_ = TIMEOUT;
    int server_tot_block_time_;
    bool fatal_error_ = false;
    LogMallocList* log_malloc_list_ = nullptr;
    UserDefinedTypeList* user_defined_type_list_ = nullptr;
    UserDefinedTypeList parsed_user_defined_type_list_;
};

} // namespace uda

#endif // UDA_SERVER_SERVER_HPP