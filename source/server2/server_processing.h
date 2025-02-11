#pragma once

#include "clientserver/uda_structs.h"

namespace uda::server
{

int server_processing(client_server::ClientBlock client_block, client_server::DataBlock* data_block);

} // namespace uda
