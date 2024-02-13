#pragma once

#include <rpc/rpc.h>
#include <utility>

namespace uda::client {

std::pair<XDR *, XDR *> clientCreateXDRStream();

}
