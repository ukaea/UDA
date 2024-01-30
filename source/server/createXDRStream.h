#pragma once

#ifndef UDA_SERVER_CREATEXDRSTREAM_H
#  define UDA_SERVER_CREATEXDRSTREAM_H

#  include <rpc/rpc.h>
#  include <utility>

struct IoData {
    int* server_tot_block_time;
    int* server_timeout;
};

std::pair<XDR*, XDR*> serverCreateXDRStream(IoData* io_data);

#endif // UDA_SERVER_CREATEXDRSTREAM_H
