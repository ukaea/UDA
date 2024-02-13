#include "makeServerRequestBlock.h"

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"

#include "getServerEnvironment.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

using namespace uda::client_server;

int uda::server::makeServerRequestBlock(RequestBlock* request_block, uda::plugins::PluginList pluginList)
{
    return make_request_block(request_block, &pluginList, getServerEnvironment());
}

int uda::server::makeServerRequestData(RequestData* request, uda::plugins::PluginList pluginList)
{
    return make_request_data(request, &pluginList, getServerEnvironment());
}

#endif
