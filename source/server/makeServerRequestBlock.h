#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "config/config.h"

namespace uda::server
{

int makeServerRequestBlock(const config::Config& config, client_server::RequestBlock* request_block, plugins::PluginList pluginList);

int makeServerRequestData(const config::Config& config, client_server::RequestData* request, plugins::PluginList pluginList);

} // namespace uda::server

#endif
