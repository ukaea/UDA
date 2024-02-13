#pragma once

#include "clientserver/socketStructs.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "uda/types.h"

/**
 * UDA Legacy Data Server (protocol versions <= 6)
 */
int legacyServer(uda::client_server::CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist,
                 LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                 uda::client_server::SOCKETLIST* socket_list, int protocolVersion, XDR* server_input,
                 XDR* server_output, unsigned int private_flags, int malloc_source);
