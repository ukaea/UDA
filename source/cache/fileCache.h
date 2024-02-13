#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"

uda::client_server::DATA_BLOCK* udaFileCacheRead(const uda::client_server::REQUEST_DATA* request,
                                                 LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                                 int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                                                 unsigned int private_flags, int malloc_source);

int udaFileCacheWrite(const uda::client_server::DATA_BLOCK* data_block,
                      const uda::client_server::REQUEST_BLOCK* request_block, LOGMALLOCLIST* logmalloclist,
                      USERDEFINEDTYPELIST* userdefinedtypelist, int protocolVersion, LOGSTRUCTLIST* log_struct_list,
                      unsigned int private_flags, int malloc_source);

#endif // UDA_CACHE_FILECACHE_H
