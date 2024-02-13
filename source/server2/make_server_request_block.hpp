#pragma once

#include "clientserver/udaStructs.h"
#include "plugins.hpp"
#include "server_environment.hpp"

namespace uda {

int makeServerRequestBlock(uda::client_server::RequestBlock* request_block, const uda::Plugins& plugins,
                           const server::Environment& environment);

int makeServerRequestData(uda::client_server::RequestData* request, const uda::Plugins& plugins,
                          const uda::server::Environment& environment);

} // namespace uda
