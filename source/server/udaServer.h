#pragma once

#include "clientserver/udaStructs.h"

int uda_server(uda::client_server::CLIENT_BLOCK client_block);

int fat_server(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::SERVER_BLOCK* server_block,
               uda::client_server::REQUEST_BLOCK* request_block0, uda::client_server::DATA_BLOCK_LIST* data_blocks0);
