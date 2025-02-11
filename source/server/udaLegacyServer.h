#pragma once

#include <vector>

#include "clientserver/socketStructs.h"
#include "clientserver/uda_structs.h"
#include "serverPlugin.h"
#include "structures/genStructs.h"
#include "uda/types.h"

namespace uda::config
{
class Config;
}

namespace uda::server
{

/**
 * UDA Legacy Data Server (protocol versions <= 6)
 */
int legacyServer(config::Config& config, client_server::ClientBlock client_block, const std::vector<client_server::PluginData>& pluginlist,
                 structures::LogMallocList* logmalloclist,
                 structures::UserDefinedTypeList* userdefinedtypelist, client_server::SOCKETLIST* socket_list,
                 int protocolVersion, XDR* server_input, XDR* server_output, unsigned int private_flags,
                 int malloc_source);

} // namespace uda::server
