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

int cache_write(uda::cache::UdaCache* cache, const uda::client_server::REQUEST_DATA* request_data, uda::client_server::DATA_BLOCK* data_block,
                LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, uda::client_server::ENVIRONMENT environment,
                int protocolVersion, uint32_t flags, LOGSTRUCTLIST* log_struct_list, unsigned int private_flags,
                int malloc_source);

uda::client_server::DATA_BLOCK* cache_read(uda::cache::UdaCache* cache, const uda::client_server::REQUEST_DATA* request_data, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, uda::client_server::ENVIRONMENT environment, int protocolVersion,
                       uint32_t flags, LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace cache
} // namespace uda

#endif // UDA_CACHE_MEMCACHE_H
