#ifndef UDA_SERVER_MAKESERVERREQUESTBLOCK_H
#define UDA_SERVER_MAKESERVERREQUESTBLOCK_H

#include <plugins/udaPlugin.h>
#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SERVERBUILD) || defined(FATCLIENT)

LIBRARY_API int makeServerRequestBlock(REQUEST_BLOCK* request_block, PLUGINLIST pluginList);

#endif

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_MAKESERVERREQUESTBLOCK_H
