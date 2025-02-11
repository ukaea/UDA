#pragma once

#include <vector>

#include "clientserver/parseXML.h"
#include "clientserver/uda_structs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::server
{

int serverSubsetData(client_server::DataBlock* data_block,
                     const uda::client_server::Action& action,
                     structures::LogMallocList* logmalloclist);

int serverParseServerSide(client_server::RequestData* request_block,
                          client_server::Actions* actions_serverside,
                          const std::vector<client_server::PluginData>& plugin_list);

} // namespace uda::server
