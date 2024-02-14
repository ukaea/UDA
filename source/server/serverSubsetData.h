#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::server
{

int serverSubsetData(uda::client_server::DataBlock* data_block, const uda::client_server::Action& action,
                     uda::structures::LogMallocList* logmalloclist);

int serverParseServerSide(uda::client_server::RequestData* request_block,
                          uda::client_server::Actions* actions_serverside, const uda::plugins::PluginList* plugin_list);

} // namespace uda::server
