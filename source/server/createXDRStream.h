#pragma once

#include "protocol/protocol.h"

#include <rpc/rpc.h>
#include <utility>

namespace uda::server
{

struct IoData : uda::client_server::IoData {
    int* server_tot_block_time;
    int* server_timeout;
};

std::pair<XDR*, XDR*> serverCreateXDRStream(uda::client_server::IoData* io_data);

} // namespace uda::server
