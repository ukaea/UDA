#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_H
#define UDA_SERVER_MAKESERVERREQUESTBLOCK_H

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SERVERBUILD) || defined(FATCLIENT)

LIBRARY_API int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);
LIBRARY_API int makeServerRequestData(REQUEST_DATA* request, PLUGINLIST pluginList);

#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_H
