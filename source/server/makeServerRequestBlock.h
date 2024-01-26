#pragma once

#if defined(SERVERBUILD) || defined(FATCLIENT)

#  include "clientserver/udaStructs.h"
#  include "serverPlugin.h"

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);
int makeServerRequestData(REQUEST_DATA* request, PLUGINLIST pluginList);

#endif
