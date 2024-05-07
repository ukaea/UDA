#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"

namespace uda::config
{
class Config;
}

namespace uda::server
{

int get_data(const config::Config& config, int* depth, client_server::RequestData* request_data,
             client_server::ClientBlock client_block, client_server::DataBlock* data_block,
             client_server::DataSource* data_source, client_server::Signal* signal_rec,
             client_server::SignalDesc* signal_desc, client_server::Actions* actions_desc,
             client_server::Actions* actions_sig, const plugins::PluginList* pluginlist,
             structures::LogMallocList* logmalloclist, structures::UserDefinedTypeList* userdefinedtypelist,
             client_server::SOCKETLIST* socket_list, int protocolVersion);

}
