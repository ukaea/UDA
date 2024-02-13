#pragma once

#include "clientserver/udaStructs.h"

namespace uda::server
{

int serverProcessing(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_BLOCK* data_block);

}
