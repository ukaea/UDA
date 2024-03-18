#pragma once

#include "clientserver/udaStructs.h"
#include "server_environment.hpp"

namespace uda::server
{

class Plugins;

int makeServerRequestBlock(uda::client_server::RequestBlock* request_block, const Plugins& plugins,
                           const server::Environment& environment);

int makeServerRequestData(uda::client_server::RequestData* request, const Plugins& plugins,
                          const uda::server::Environment& environment);

} // namespace uda::server
