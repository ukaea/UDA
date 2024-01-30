#pragma once

#ifndef UDA_CLIENT_CLIENTXDRSTREAM_H
#define UDA_CLIENT_CLIENTXDRSTREAM_H

#include <utility>
#include <rpc/rpc.h>

#include "include/uda/export.h"
#include "connection.hpp"

namespace uda {
namespace client {

std::pair<XDR*, XDR*> createXDRStream(IoData* io_data);

}
}

#endif // UDA_CLIENT_CLIENTXDRSTREAM_H
