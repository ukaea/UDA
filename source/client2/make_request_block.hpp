#pragma once

#include "clientserver/udaStructs.h"

namespace uda::config
{
class Config;
}

namespace uda::client {

int make_request_block(const config::Config& config, const char** signals, const char** sources, int count,
                       client_server::RequestBlock* request_block);

} // namespace uda::client
