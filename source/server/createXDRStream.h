#ifndef UDA_SERVER_CREATEXDRSTREAM_H
#define UDA_SERVER_CREATEXDRSTREAM_H

#include <utility>
#include <rpc/rpc.h>

#include <clientserver/export.h>

std::pair<XDR*, XDR*> CreateXDRStream();

#endif // UDA_SERVER_CREATEXDRSTREAM_H
