#ifndef UDA_SERVER_UDASERVER_H
#define UDA_SERVER_UDASERVER_H

#include <plugins/pluginStructs.h>
#include <clientserver/socketStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaServer(CLIENT_BLOCK client_block);

LIBRARY_API int fatServer(CLIENT_BLOCK client_block, SERVER_BLOCK* server_block, REQUEST_BLOCK* request_block0,
              DATA_BLOCK_LIST* data_blocks0);

//--------------------------------------------------------------
// Static Global variables

extern USERDEFINEDTYPELIST parseduserdefinedtypelist;

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_UDASERVER_H
