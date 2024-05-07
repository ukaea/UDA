#include "makeServerRequestBlock.h"

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"

#include "getServerEnvironment.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

using namespace uda::client_server;

int uda::server::makeServerRequestBlock(const config::Config& config, RequestBlock* request_block, uda::plugins::PluginList pluginList)
{
    return make_request_block(config, request_block, &pluginList);
}

int uda::server::makeServerRequestData(const config::Config& config, RequestData* request, uda::plugins::PluginList pluginList)
{
    return make_request_data(config, request, &pluginList);
}

#endif
