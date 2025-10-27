#ifndef UDA_LOGGING_ACCESSLOG_H
#define UDA_LOGGING_ACCESSLOG_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

#define HOSTNAMELENGTH    20
#define DATELENGTH        27

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void
udaAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, SERVER_BLOCK server_block,
             unsigned int total_datablock_size);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_ACCESSLOG_H
