#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#  include "clientserver/udaStructs.h"
#  include "serverPlugin.h"

namespace uda::server
{

int makeServerRequestBlock(uda::client_server::RequestBlock* request_block, uda::plugins::PluginList pluginList);

int makeServerRequestData(uda::client_server::RequestData* request, uda::plugins::PluginList pluginList);

} // namespace uda::server

#endif
