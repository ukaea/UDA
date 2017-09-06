#ifndef UDA_SERVER_IDAMSERVER_H
#define UDA_SERVER_IDAMSERVER_H

#define MAXOPENFILEDESC 50  // Maximum number of Open File Descriptors

#define MAXMAPDEPTH     10  // Maximum number of chained signal name mappings (Recursive depth)
#define MAXREQDEPTH     4   // Maximum number of Device Name to Server Protocol and Host substitutions

#define XDEBUG          0   // Socket Streams

#include <server/pluginStructs.h>
#include <clientserver/socketStructs.h>
#include <structures/genStructs.h>

USERDEFINEDTYPELIST* getIdamServerUserDefinedTypeList();

int udaServer(CLIENT_BLOCK client_block);

int fatServer(CLIENT_BLOCK client_block, REQUEST_BLOCK* request_block0, SERVER_BLOCK* server_block0,
              DATA_BLOCK* data_block0);

//--------------------------------------------------------------
// Static Global variables

extern unsigned int totalDataBlockSize;
extern int serverVersion;
extern int altRank;

extern IDAMERRORSTACK idamerrorstack;

extern XDR* serverInput;
extern XDR* serverOutput;
extern int server_tot_block_time;
extern int server_timeout;

extern USERDEFINEDTYPELIST parseduserdefinedtypelist;

#endif // UDA_SERVER_IDAMSERVER_H
