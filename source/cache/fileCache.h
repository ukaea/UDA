#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include "export.h"
#include "genStructs.h"
#include "clientserver/udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API DATA_BLOCK* udaFileCacheRead(const REQUEST_DATA* request, LOGMALLOCLIST* logmalloclist,
                                         USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion,
                                         LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source);

LIBRARY_API int udaFileCacheWrite(const DATA_BLOCK* data_block, const REQUEST_BLOCK* request_block,
                                  LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                  int protocolVersion, LOGSTRUCTLIST* log_struct_list, unsigned int private_flags,
                                  int malloc_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_FILECACHE_H
