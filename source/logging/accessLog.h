#pragma once

#include "clientserver/udaStructs.h"

namespace uda::logging
{

void uda_access_log(int init, uda::client_server::ClientBlock client_block,
                  uda::client_server::RequestBlock request_block, uda::client_server::ServerBlock server_block,
                  unsigned int total_datablock_size);

} // namespace uda::logging
