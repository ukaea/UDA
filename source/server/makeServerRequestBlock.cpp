#include "makeServerRequestBlock.h"

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

using namespace uda::client_server;

int uda::server::makeServerRequestBlock(const config::Config& config, RequestBlock* request_block, const std::vector<PluginData>& pluginList)
{
    return make_request_block(config, request_block, pluginList);
}

int uda::server::makeServerRequestData(const config::Config& config, RequestData* request, const std::vector<PluginData>& pluginList)
{
    return make_request_data(config, request, pluginList);
}

#endif
