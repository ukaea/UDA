#pragma once

#include "plugins/udaPlugin.h"
#include "udaStructs.h"

typedef struct PluginList PLUGINLIST;

namespace uda::client_server
{

int make_request_block(RequestBlock* request_block, const uda::plugins::PluginList* pluginList,
                       const Environment* environment);

int make_request_data(RequestData* request, const uda::plugins::PluginList* pluginList, const Environment* environment);

int name_value_pairs(const char* pairList, NameValueList* nameValueList, unsigned short strip);

void free_name_value_list(NameValueList* nameValueList);

} // namespace uda::client_server