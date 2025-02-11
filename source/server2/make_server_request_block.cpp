#include "make_server_request_block.hpp"

#include "clientserver/make_request_block.h"

#include "plugins.hpp"

using namespace uda::client_server;
using namespace uda::config;

int uda::server::make_server_request_block(const Config& config, RequestBlock *request_block, const Plugins& plugins)
{
    return make_request_block(config, request_block, plugins.plugin_list());
}

int uda::server::make_server_request_data(const Config& config, RequestData *request, const Plugins& plugins)
{
    return make_request_data(config, request, plugins.plugin_list());
}
