#pragma once

#include "clientserver/udaStructs.h"

int uda_server(CLIENT_BLOCK client_block);

int fat_server(CLIENT_BLOCK client_block, SERVER_BLOCK* server_block, REQUEST_BLOCK* request_block0,
               DATA_BLOCK_LIST* data_blocks0);
