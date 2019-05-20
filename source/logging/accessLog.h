#ifndef UDA_LOGGING_ACCESSLOG_H
#define UDA_LOGGING_ACCESSLOG_H

#include <plugins/udaPlugin.h>

#define HOSTNAMELENGTH    20
#define DATELENGTH        27

#ifdef __cplusplus
extern "C" {
#endif

unsigned int countDataBlockSize(DATA_BLOCK* data_block, CLIENT_BLOCK* client_block);

void idamAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request, SERVER_BLOCK server_block,
                   const PLUGINLIST* pluginlist, const ENVIRONMENT* environment);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_ACCESSLOG_H