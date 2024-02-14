#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"
#include <cstdio>

void writeCacheData(FILE* fp, LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                    const uda::client_server::DataBlock* data_block, int protocolVersion,
                    LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

uda::client_server::DataBlock* readCacheData(FILE* fp, LogMallocList* logmalloclist,
                                              UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                                              LogStructList* log_struct_list, unsigned int private_flags,
                                              int malloc_source);

#endif // UDA_CACHE_CACHE_H