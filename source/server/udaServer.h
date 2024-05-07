#pragma once

#include "clientserver/udaStructs.h"

namespace uda::config
{
class Config;
}

namespace uda::server
{

int uda_server(const config::Config& config, client_server::ClientBlock client_block);

int fat_server(const config::Config& config, client_server::ClientBlock client_block, client_server::ServerBlock* server_block,
               client_server::RequestBlock* request_block0, client_server::DataBlockList* data_blocks0);

} // namespace uda::server
