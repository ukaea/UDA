#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::server
{

int get_data(int* depth, uda::client_server::RequestData* request_data, uda::client_server::ClientBlock client_block,
             uda::client_server::DataBlock* data_block, uda::client_server::DataSource* data_source,
             uda::client_server::Signal* signal_rec, uda::client_server::SignalDesc* signal_desc,
             uda::client_server::Actions* actions_desc, uda::client_server::Actions* actions_sig,
             const uda::plugins::PluginList* pluginlist, uda::structures::LogMallocList* logmalloclist,
             uda::structures::UserDefinedTypeList* userdefinedtypelist, uda::client_server::SOCKETLIST* socket_list,
             int protocolVersion);

}
