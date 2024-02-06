#pragma once

#include "clientserver/udaStructs.h"

namespace uda {
namespace client {

int make_request_block(const Environment* environment, const char** signals, const char** sources, int count,
                       REQUEST_BLOCK* request_block);

}
} // namespace uda
