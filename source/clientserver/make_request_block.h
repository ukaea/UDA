#pragma once

#include <vector>

#include "uda_structs.h"

namespace uda::config {
class Config;
}

namespace uda::client_server
{

struct PluginData;

int make_request_block(const config::Config& config, RequestBlock* request_block, const std::vector<PluginData>& plugin_list);

int make_request_data(const config::Config& config, RequestData* request, const std::vector<PluginData>& plugin_list);

void expand_environmental_variables(char* path);

} // namespace uda::client_server