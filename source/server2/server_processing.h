#pragma once

#include "clientserver/udaStructs.h"

namespace uda::server
{

int server_processing(client_server::ClientBlock client_block, client_server::DataBlock* data_block);

} // namespace uda
