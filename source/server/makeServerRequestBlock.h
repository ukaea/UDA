#ifndef IDAM_SERVER_MAKESERVERREQUESTBLOCK_H
#define IDAM_SERVER_MAKESERVERREQUESTBLOCK_H

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SERVERBUILD) || defined(FATCLIENT)

void initServerRequestBlock(REQUEST_BLOCK* str);
int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);

#endif

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_MAKESERVERREQUESTBLOCK_H
