#pragma once

#include <vector>

#include "plugins/udaPlugin.h"
#include "udaStructs.h"

namespace uda::config {
class Config;
}

namespace uda::client_server
{

struct PluginData;

int make_request_block(const config::Config& config, RequestBlock* request_block, const std::vector<PluginData>& pluginList);

int make_request_data(const config::Config& config, RequestData* request, const std::vector<PluginData>& pluginList);

int name_value_pairs(const char* pairList, NameValueList* nameValueList, unsigned short strip);

void free_name_value_list(NameValueList* nameValueList);

} // namespace uda::client_server