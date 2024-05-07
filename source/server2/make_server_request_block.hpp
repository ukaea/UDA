#pragma once

#include "clientserver/udaStructs.h"
#include "config/config.h"

namespace uda::server
{

class Plugins;

int make_server_request_block(const config::Config& config, client_server::RequestBlock *request_block, const Plugins& plugins);

int make_server_request_data(const config::Config& config, client_server::RequestData *request, const Plugins& plugins);

} // namespace uda::server
