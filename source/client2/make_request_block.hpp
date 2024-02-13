#pragma once

#include "clientserver/udaStructs.h"

namespace uda {
namespace client {

int make_request_block(const uda::client_server::Environment* environment, const char** signals, const char** sources, int count,
                       uda::client_server::REQUEST_BLOCK* request_block);

}
} // namespace uda
