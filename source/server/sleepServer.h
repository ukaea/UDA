#pragma once

#include <rpc/rpc.h>

#include "createXDRStream.h"
#include "structures/genStructs.h"

namespace uda::server
{

int sleepServer(XDR* server_input, XDR* server_output, uda::structures::LogMallocList* logmalloclist,
                uda::structures::UserDefinedTypeList* userdefinedtypelist, int protocolVersion,
                uda::structures::LogStructList* log_struct_list, int server_tot_block_time, int server_timeout,
                uda::server::IoData* io_data, int private_flags, int malloc_source);

}
