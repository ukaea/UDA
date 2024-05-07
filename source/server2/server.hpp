#pragma once

#include <vector>

#include "get_data.hpp"
#include "plugins.hpp"
#include "server_environment.hpp"
#include "xdr_protocol.hpp"

#include "cache/memcache.hpp"
#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"
#include "include/uda/export.h"
#include "config/config.h"

namespace uda::server
{

struct MetadataBlock {
    uda::client_server::DataSource data_source;
    uda::client_server::Signal signal_rec;
    uda::client_server::SignalDesc signal_desc;
    uda::client_server::SystemConfig system_config;
    uda::client_server::DataSystem data_system;
};

class Server
{
  public:
    constexpr static int ServerVersion = 8;
    constexpr static int LegacyServerVersion = 6;

    LIBRARY_API Server(config::Config config);
    LIBRARY_API void run();
    LIBRARY_API void close();

  private:
    void startup();
    void loop();
    int handle_request();
    int report_to_client();
    void handshake_client();
    void start_logs();
    int get_data(int* depth, uda::client_server::RequestData* request_data, uda::client_server::DataBlock* data_block,
                 int protocol_version);
    int read_data(uda::client_server::RequestData* request, uda::client_server::DataBlock* data_block);

    config::Config _config;
    std::vector<uda::client_server::UdaError> _error_stack;
    client_server::RequestBlock _request_block;
    client_server::ServerBlock _server_block;
    client_server::ClientBlock _client_block;
    client_server::Actions _actions_desc;
    client_server::Actions _actions_sig;
    cache::UdaCache* _cache;
    XdrProtocol _protocol;
    std::vector<uda::client_server::Sockets> _sockets;
    Plugins _plugins;
    bool _server_closedown = false;
    int _malloc_source = UDA_MALLOC_SOURCE_NONE;
    std::vector<uda::client_server::DataBlock> _data_blocks;
    size_t _total_data_block_size;
    MetadataBlock _metadata_block;
    int _server_timeout = TIMEOUT;
    int _server_tot_block_time;
    bool _fatal_error = false;
    structures::LogMallocList* _log_malloc_list = nullptr;
    structures::UserDefinedTypeList* _user_defined_type_list = nullptr;
    structures::UserDefinedTypeList _parsed_user_defined_type_list;
};

} // namespace uda::server
