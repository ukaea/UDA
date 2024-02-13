#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#  include "clientserver/udaStructs.h"
#  include "serverPlugin.h"

namespace uda::server
{

int makeServerRequestBlock(uda::client_server::REQUEST_BLOCK* request_block, uda::plugins::PluginList pluginList);

int makeServerRequestData(uda::client_server::REQUEST_DATA* request, uda::plugins::PluginList pluginList);

} // namespace uda::server

#endif
