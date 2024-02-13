#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::server
{

int udaGetData(int* depth, uda::client_server::REQUEST_DATA* request_data,
               uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_BLOCK* data_block,
               uda::client_server::DATA_SOURCE* data_source, uda::client_server::SIGNAL* signal_rec,
               uda::client_server::SIGNAL_DESC* signal_desc, uda::client_server::ACTIONS* actions_desc,
               uda::client_server::ACTIONS* actions_sig, const uda::plugins::PluginList* pluginlist,
               LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
               uda::client_server::SOCKETLIST* socket_list, int protocolVersion);

}
