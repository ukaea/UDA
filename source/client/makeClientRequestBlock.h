#pragma once

#include "clientserver/udaStructs.h"

namespace uda::client
{

int makeClientRequestBlock(const char** signals, const char** sources, int count,
                           uda::client_server::RequestBlock* request_block);

void freeClientRequestBlock(uda::client_server::RequestBlock* request_block);

} // namespace uda::client
