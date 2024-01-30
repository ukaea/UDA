#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include "structures/genStructs.h"
#include "clientserver/udaStructs.h"
#include <cstdio>

void writeCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                const DATA_BLOCK* data_block, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                                unsigned int private_flags, int malloc_source);

DATA_BLOCK* readCacheData(FILE* fp, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                      int protocolVersion, LOGSTRUCTLIST* log_struct_list, unsigned int private_flags,
                                      int malloc_source);

#endif // UDA_CACHE_CACHE_H