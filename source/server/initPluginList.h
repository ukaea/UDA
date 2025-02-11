#pragma once

#include <vector>

#include "clientserver/uda_structs.h"
#include "serverPlugin.h"

namespace uda::server
{

void initPluginList(std::vector<client_server::PluginData>& plugin_list);

}
