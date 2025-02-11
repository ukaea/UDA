#pragma once

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"

namespace uda::config {
class Config;
}

namespace uda::cache {

struct UdaCache;

UdaCache* open_cache();

void free_cache();

int cache_write(const config::Config& config, cache::UdaCache* cache, const client_server::RequestData* request_data, client_server::DataBlock* data_block,
                structures::LogMallocList* logmalloclist, structures::UserDefinedTypeList* userdefinedtypelist,
                int protocolVersion, uint32_t flags, structures::LogStructList* log_struct_list, unsigned int private_flags,
                int malloc_source);

client_server::DataBlock* cache_read(const config::Config& config, cache::UdaCache* cache, const client_server::RequestData* request_data, structures::LogMallocList* logmalloclist,
                                          structures::UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                       uint32_t flags, structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace uda::cache
