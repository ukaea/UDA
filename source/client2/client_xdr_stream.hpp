#pragma once

#include <rpc/rpc.h>

#include "connection.hpp"

namespace uda::client
{

std::pair<XDR*, XDR*> create_xdr_stream(IoData* io_data);

}

