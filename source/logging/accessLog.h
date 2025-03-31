#pragma once

#include "clientserver/uda_structs.h"

namespace uda::logging
{

void uda_access_log(int init, client_server::ClientBlock client_block, client_server::RequestBlock request_block,
                    client_server::ServerBlock server_block, unsigned int total_datablock_size);

} // namespace uda::logging
