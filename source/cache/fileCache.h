#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API DATA_BLOCK*
udaFileCacheRead(const REQUEST_DATA* request, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                 int protocolVersion, NTREE* full_ntree, LOGSTRUCTLIST* log_struct_list);

LIBRARY_API int
udaFileCacheWrite(const DATA_BLOCK* data_block, const REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                  USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, NTREE* full_ntree,
                  LOGSTRUCTLIST* log_struct_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_FILECACHE_H
