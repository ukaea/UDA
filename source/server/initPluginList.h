#pragma once

#include <vector>

#include "clientserver/udaStructs.h"
#include "serverPlugin.h"

namespace uda::server
{

void initPluginList(std::vector<client_server::PluginData>& plugin_list);

}
