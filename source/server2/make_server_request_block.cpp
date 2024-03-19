#include "make_server_request_block.hpp"

#include "clientserver/makeRequestBlock.h"

#include "plugins.hpp"
#include "server_environment.hpp"

using namespace uda::client_server;

int uda::server::make_server_request_block(uda::client_server::RequestBlock *request_block, const Plugins& plugins,
                                           const server::Environment& environment)
{
    auto plugin_list = plugins.as_plugin_list();
    return make_request_block(request_block, &plugin_list, environment.p_env());
}

int uda::server::make_server_request_data(uda::client_server::RequestData *request, const Plugins& plugins,
                                          const uda::server::Environment &environment)
{
    auto plugin_list = plugins.as_plugin_list();
    return make_request_data(request, &plugin_list, environment.p_env());
}
