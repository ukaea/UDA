#include "make_server_request_block.hpp"

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"

#include "plugins.hpp"
#include "server_environment.hpp"

#if defined(SERVERBUILD) || defined(FATCLIENT)

int uda::makeServerRequestBlock(RequestBlock* request_block, const uda::Plugins& plugins,
                                const server::Environment& environment)
{
    auto plugin_list = plugins.as_plugin_list();
    return make_request_block(request_block, &plugin_list, environment.p_env());
}

int uda::makeServerRequestData(RequestData* request, const uda::Plugins& plugins,
                               const server::Environment& environment)
{
    auto plugin_list = plugins.as_plugin_list();
    return makeRequestData(request, &plugin_list, environment.p_env());
}

#endif
