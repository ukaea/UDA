#pragma once

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"
#include <cstdio>

namespace uda::cache {

void writeCacheData(std::vector<client_server::UdaError>& error_stack, FILE* fp, structures::LogMallocList* logmalloclist,
                    structures::UserDefinedTypeList* userdefinedtypelist,
                    const client_server::DataBlock* data_block, int protocolVersion,
                    structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

client_server::DataBlock* readCacheData(std::vector<client_server::UdaError>& error_stack, FILE* fp, structures::LogMallocList* logmalloclist,
                                             structures::UserDefinedTypeList* userdefinedtypelist,
                                             int protocolVersion, structures::LogStructList* log_struct_list,
                                             unsigned int private_flags, int malloc_source);

}
