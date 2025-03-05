#pragma once

#include "protocol/protocol.h"

#include <rpc/rpc.h>
#include <utility>

namespace uda::server
{

struct IoData {
    int* server_tot_block_time;
    int* server_timeout;
};

std::pair<XDR*, XDR*> serverCreateXDRStream(IoData* io_data);

} // namespace uda::server
