#ifndef UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H
#define UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H

#include "clientserver/udaStructs.h"

int makeClientRequestBlock(const char** signals, const char** sources, int count, REQUEST_BLOCK* request_block);
void freeClientRequestBlock(REQUEST_BLOCK* request_block);

#endif // UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H
