#pragma once

#ifndef UDA_CLIENT_CLIENTXDRSTREAM_H
#define UDA_CLIENT_CLIENTXDRSTREAM_H

#include <rpc/rpc.h>
#include <utility>

#include "connection.hpp"

namespace uda
{
namespace client
{

std::pair<XDR*, XDR*> createXDRStream(IoData* io_data);

}
} // namespace uda

#endif // UDA_CLIENT_CLIENTXDRSTREAM_H
