#ifndef UDA_LOGGING_ACCESSLOG_H
#define UDA_LOGGING_ACCESSLOG_H

#include <plugins/udaPlugin.h>

#define HOSTNAMELENGTH    20
#define DATELENGTH        27

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API unsigned int countDataBlockSize(DATA_BLOCK* data_block, CLIENT_BLOCK* client_block);

LIBRARY_API void idamAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request, SERVER_BLOCK server_block,
                   const PLUGINLIST* pluginlist, const ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_ACCESSLOG_H