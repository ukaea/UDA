#pragma once

#include "clientserver/udaStructs.h"

#define HOSTNAMELENGTH 20
#define DATELENGTH 27

namespace uda::logging
{

unsigned int countDataBlockListSize(const uda::client_server::DATA_BLOCK_LIST* data_block_list,
                                    uda::client_server::CLIENT_BLOCK* client_block);

unsigned int countDataBlockSize(const uda::client_server::DATA_BLOCK* data_block,
                                uda::client_server::CLIENT_BLOCK* client_block);

void udaAccessLog(int init, uda::client_server::CLIENT_BLOCK client_block,
                  uda::client_server::REQUEST_BLOCK request_block, uda::client_server::SERVER_BLOCK server_block,
                  unsigned int total_datablock_size);

} // namespace uda::logging
