#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#include <vector>

#include "clientserver/uda_structs.h"
#include "serverPlugin.h"
#include "config/config.h"

namespace uda::server
{

int makeServerRequestBlock(const config::Config& config, client_server::RequestBlock* request_block, const std::vector<client_server::PluginData>& pluginList);

int makeServerRequestData(const config::Config& config, client_server::RequestData* request, const std::vector<client_server::PluginData>& pluginList);

} // namespace uda::server

#endif
