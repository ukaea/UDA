#pragma once

#include <vector>

#include "plugins.hpp"
#include "xdr_protocol.hpp"

#include "cache/memcache.hpp"
#include "clientserver/socket_structs.h"
#include "clientserver/version.h"
#include <uda/export.h>
#include "config/config.h"

#include <uda/version.h>

namespace uda::server
{

class Server
{
  public:
    constexpr static int ServerVersion = UDA_GET_VERSION(UDA_VERSION_MAJOR, UDA_VERSION_MINOR, UDA_VERSION_BUGFIX, UDA_VERSION_DELTA);
    constexpr static int LegacyServerVersion = 6;

    LIBRARY_API Server(config::Config config);
    LIBRARY_API void run();

  protected:
    void shutdown();
    void initialise();
    void connect(int socket_fd);
    void loop();
    int handle_request();
    int report_to_client();
    void handshake_client();
    void start_logs();
    int get_data(int* depth, client_server::RequestData* request_data, client_server::DataBlock* data_block,
                 int protocol_version);
    int read_data(client_server::RequestData* request, client_server::DataBlock* data_block);

    config::Config config_;
    std::vector<client_server::UdaError> error_stack_;
    client_server::RequestBlock _request_block;
    client_server::ServerBlock server_block_;
    client_server::ClientBlock client_block_;
    cache::UdaCache* cache_;
    XdrProtocol _protocol;
    std::vector<client_server::Socket> _sockets;
    Plugins _plugins;
    bool _server_closedown = false;
    std::vector<client_server::DataBlock> data_blocks_;
    size_t _total_data_block_size;
    client_server::MetaData _meta_data;
    int _server_timeout = client_server::TimeOut;
    int _server_tot_block_time;
    bool _fatal_error = false;
    structures::LogMallocList* _log_malloc_list = nullptr;
    structures::UserDefinedTypeList* _user_defined_type_list = nullptr;
};

} // namespace uda::server
