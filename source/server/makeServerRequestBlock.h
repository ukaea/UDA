#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#  include "clientserver/udaStructs.h"
#  include "serverPlugin.h"

int makeServerRequestBlock(uda::client_server::REQUEST_BLOCK* request_block, PLUGINLIST pluginList);
int makeServerRequestData(uda::client_server::REQUEST_DATA* request, PLUGINLIST pluginList);

#endif
