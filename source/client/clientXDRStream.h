#ifndef UDA_CLIENT_CLIENTXDRSTREAM_H
#define UDA_CLIENT_CLIENTXDRSTREAM_H

#include <rpc/rpc.h>
#include <utility>

#include "export.h"

std::pair<XDR*, XDR*> clientCreateXDRStream();

#endif // UDA_CLIENT_CLIENTXDRSTREAM_H
