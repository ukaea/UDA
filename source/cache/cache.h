#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include "export.h"
#include "genStructs.h"
#include "clientserver/udaStructs.h"
#include <cstdio>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void writeCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                const DATA_BLOCK* data_block, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                                unsigned int private_flags, int malloc_source);

LIBRARY_API DATA_BLOCK* readCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                      int protocolVersion, LOGSTRUCTLIST* log_struct_list, unsigned int private_flags,
                                      int malloc_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CACHE_CACHE_H