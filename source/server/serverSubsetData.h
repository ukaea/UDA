#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::server
{

int serverSubsetData(uda::client_server::DATA_BLOCK* data_block, const uda::client_server::ACTION& action,
                     LOGMALLOCLIST* logmalloclist);

int serverParseServerSide(uda::client_server::REQUEST_DATA* request_block,
                          uda::client_server::ACTIONS* actions_serverside, const uda::plugins::PluginList* plugin_list);

} // namespace uda::server
