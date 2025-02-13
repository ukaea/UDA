#ifndef UDA_CACHE_FILECACHE_H
#define UDA_CACHE_FILECACHE_H

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"

namespace uda::cache {

client_server::DataBlock* udaFileCacheRead(std::vector<client_server::UdaError>& error_stack,
                                                const client_server::RequestData* request,
                                                structures::LogMallocList* logmalloclist,
                                                structures::UserDefinedTypeList* userdefinedtypelist,
                                                int protocolVersion, structures::LogStructList* log_struct_list,
                                                unsigned int private_flags, int malloc_source);

int udaFileCacheWrite(std::vector<client_server::UdaError>& error_stack,
                      const client_server::DataBlock* data_block,
                      const client_server::RequestBlock* request_block,
                      structures::LogMallocList* logmalloclist,
                      structures::UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                      structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

}

#endif // UDA_CACHE_FILECACHE_H
