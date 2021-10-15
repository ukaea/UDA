#ifndef UDA_CLIENT_CLIENTXDRSTREAM_H
#define UDA_CLIENT_CLIENTXDRSTREAM_H

#include <utility>
#include <rpc/rpc.h>

#include <clientserver/export.h>

std::pair<XDR*, XDR*> createXDRStream();

#endif // UDA_CLIENT_CLIENTXDRSTREAM_H
