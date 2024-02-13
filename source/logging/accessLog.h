#pragma once

#include "clientserver/udaStructs.h"

#define HOSTNAMELENGTH 20
#define DATELENGTH 27

namespace uda::logging
{

unsigned int countDataBlockListSize(const uda::client_server::DataBlockList* data_block_list,
                                    uda::client_server::ClientBlock* client_block);

unsigned int countDataBlockSize(const uda::client_server::DataBlock* data_block,
                                uda::client_server::ClientBlock* client_block);

void udaAccessLog(int init, uda::client_server::ClientBlock client_block,
                  uda::client_server::RequestBlock request_block, uda::client_server::ServerBlock server_block,
                  unsigned int total_datablock_size);

} // namespace uda::logging
