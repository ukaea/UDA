#include "makeServerRequestBlock.h"

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"

#include "getServerEnvironment.h"

#if defined(SERVERBUILD) || defined(FATCLIENT)

using namespace uda::client_server;

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList)
{
    return make_request_block(request_block, &pluginList, getServerEnvironment());
}

int makeServerRequestData(REQUEST_DATA* request, PLUGINLIST pluginList)
{
    return makeRequestData(request, &pluginList, getServerEnvironment());
}

#endif
