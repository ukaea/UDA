#pragma once

#include "clientserver/udaStructs.h"

namespace uda::server
{

int uda_server(uda::client_server::ClientBlock client_block);

int fat_server(uda::client_server::ClientBlock client_block, uda::client_server::ServerBlock* server_block,
               uda::client_server::RequestBlock* request_block0, uda::client_server::DataBlockList* data_blocks0);

} // namespace uda::server
