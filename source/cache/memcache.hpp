#ifndef UDA_CACHE_MEMCACHE_H
#define UDA_CACHE_MEMCACHE_H

#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"
#include "include/uda/export.h"

namespace uda {
namespace cache {

struct UdaCache;

UdaCache* open_cache();

void free_cache();

int cache_write(uda::cache::UdaCache* cache, const uda::client_server::RequestData* request_data, uda::client_server::DataBlock* data_block,
                uda::structures::LogMallocList* logmalloclist, uda::structures::UserDefinedTypeList* userdefinedtypelist, uda::client_server::Environment environment,
                int protocolVersion, uint32_t flags, uda::structures::LogStructList* log_struct_list, unsigned int private_flags,
                int malloc_source);

uda::client_server::DataBlock* cache_read(uda::cache::UdaCache* cache, const uda::client_server::RequestData* request_data, uda::structures::LogMallocList* logmalloclist,
                                          uda::structures::UserDefinedTypeList* userdefinedtypelist, uda::client_server::Environment environment, int protocolVersion,
                       uint32_t flags, uda::structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace cache
} // namespace uda

#endif // UDA_CACHE_MEMCACHE_H
