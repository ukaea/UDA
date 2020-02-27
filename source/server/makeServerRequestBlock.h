#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_H
#define UDA_SERVER_MAKESERVERREQUESTBLOCK_H

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#if defined(SERVERBUILD) || defined(FATCLIENT)

int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);

#endif

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_H
