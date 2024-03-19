#pragma once

#include "clientserver/udaStructs.h"
#include "server_environment.hpp"

namespace uda::server
{

class Plugins;

int make_server_request_block(uda::client_server::RequestBlock *request_block, const Plugins& plugins,
                              const server::Environment& environment);

int make_server_request_data(uda::client_server::RequestData *request, const Plugins& plugins,
                             const uda::server::Environment &environment);

} // namespace uda::server
