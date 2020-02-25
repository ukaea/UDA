#ifndef UDA_SERVER_UDASERVER_H
#define UDA_SERVER_UDASERVER_H

#define MAXOPENFILEDESC 50  // Maximum number of Open File Descriptors

#define XDEBUG          0   // Socket Streams

#include <plugins/pluginStructs.h>
#include <clientserver/socketStructs.h>
#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaServer(CLIENT_BLOCK client_block);

LIBRARY_API int fatServer(CLIENT_BLOCK client_block, SERVER_BLOCK* server_block, REQUEST_BLOCK* request_block0,
              DATA_BLOCK* data_block0);

//--------------------------------------------------------------
// Static Global variables

extern unsigned int totalDataBlockSize;
extern int serverVersion;
extern int altRank;

#ifndef FATCLIENT
extern XDR* serverInput;
extern XDR* serverOutput;
#endif
extern int server_tot_block_time;
extern int server_timeout;

extern USERDEFINEDTYPELIST parseduserdefinedtypelist;

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_UDASERVER_H
