#pragma once

#  include "export.h"
#  include "genStructs.h"
#  include "plugins/pluginStructs.h"
#  include <clientserver/socketStructs.h>

int uda_server(CLIENT_BLOCK client_block);

int fat_server(CLIENT_BLOCK client_block, SERVER_BLOCK* server_block, REQUEST_BLOCK* request_block0,
                          DATA_BLOCK_LIST* data_blocks0);

