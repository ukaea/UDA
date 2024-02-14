#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"

uda::client_server::DataBlock* udaFileCacheRead(const uda::client_server::RequestData* request,
                                                LogMallocList* logmalloclist, UserDefinedTypeList* userdefinedtypelist,
                                                 int protocolVersion, LogStructList* log_struct_list,
                                                 unsigned int private_flags, int malloc_source);

int udaFileCacheWrite(const uda::client_server::DataBlock* data_block,
                      const uda::client_server::RequestBlock* request_block, LogMallocList* logmalloclist,
                      UserDefinedTypeList* userdefinedtypelist, int protocolVersion, LogStructList* log_struct_list,
                      unsigned int private_flags, int malloc_source);

#endif // UDA_CACHE_FILECACHE_H
