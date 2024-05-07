#include "make_server_request_block.hpp"

#include "clientserver/makeRequestBlock.h"

#include "plugins.hpp"

using namespace uda::client_server;
using namespace uda::config;

int uda::server::make_server_request_block(const Config& config, RequestBlock *request_block, const Plugins& plugins)
{
    auto plugin_list = plugins.as_plugin_list();
    return make_request_block(config, request_block, &plugin_list);
}

int uda::server::make_server_request_data(const Config& config, RequestData *request, const Plugins& plugins)
{
    auto plugin_list = plugins.as_plugin_list();
    return make_request_data(config, request, &plugin_list);
}
