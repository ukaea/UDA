#pragma once

#include "clientserver/uda_structs.h"

namespace uda::config
{
class Config;
}

namespace uda::client {

int make_request_block(std::vector<client_server::UdaError>& error_stack, const config::Config& config,
                       const char** signals, const char** sources, int count, client_server::RequestBlock* request_block);

} // namespace uda::client
