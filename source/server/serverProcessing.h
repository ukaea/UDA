#pragma once

#include "clientserver/uda_structs.h"

namespace uda::server
{

int serverProcessing(uda::client_server::ClientBlock client_block, uda::client_server::DataBlock* data_block);

}
