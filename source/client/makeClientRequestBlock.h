#ifndef UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H
#define UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H

#include "export.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int makeClientRequestBlock(const char** signals, const char** sources, int count,
                                       REQUEST_BLOCK* request_block);
LIBRARY_API void freeClientRequestBlock(REQUEST_BLOCK* request_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H
