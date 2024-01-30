#pragma once

#include "uda/types.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "clientserver/socketStructs.h"


/**
 * UDA Legacy Data Server (protocol versions <= 6)
 */
int legacyServer(CLIENT_BLOCK client_block, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                 USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion,
                 XDR* server_input, XDR* server_output, unsigned int private_flags, int malloc_source);

