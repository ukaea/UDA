#ifndef UDA_CACHE_CACHE_H
#define UDA_CACHE_CACHE_H

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"
#include <cstdio>

void writeCacheData(FILE* fp, uda::structures::LogMallocList* logmalloclist,
                    uda::structures::UserDefinedTypeList* userdefinedtypelist,
                    const uda::client_server::DataBlock* data_block, int protocolVersion,
                    uda::structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

uda::client_server::DataBlock* readCacheData(FILE* fp, uda::structures::LogMallocList* logmalloclist,
                                             uda::structures::UserDefinedTypeList* userdefinedtypelist,
                                             int protocolVersion, uda::structures::LogStructList* log_struct_list,
                                             unsigned int private_flags, int malloc_source);

#endif // UDA_CACHE_CACHE_H