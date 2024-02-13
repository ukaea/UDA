#pragma once

#include "plugins/udaPlugin.h"
#include "udaStructs.h"

typedef struct PluginList PLUGINLIST;

namespace uda::client_server
{

int make_request_block(REQUEST_BLOCK* request_block, const uda::plugins::PluginList* pluginList,
                       const ENVIRONMENT* environment);

int make_request_data(REQUEST_DATA* request, const uda::plugins::PluginList* pluginList, const ENVIRONMENT* environment);

int name_value_pairs(const char* pairList, NAMEVALUELIST* nameValueList, unsigned short strip);

void free_name_value_list(NAMEVALUELIST* nameValueList);

} // namespace uda::client_server