#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API DATA_BLOCK* udaFileCacheRead(const REQUEST_BLOCK* request_block,
                                         LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                         int protocolVersion);
LIBRARY_API int udaFileCacheWrite(const DATA_BLOCK* data_block, const REQUEST_BLOCK* request_block,
                                  LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                  int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_FILECACHE_H
