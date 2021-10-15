#pragma once

#ifndef UDA_SERVER_CREATEXDRSTREAM_H
#define UDA_SERVER_CREATEXDRSTREAM_H

#include <utility>
#include <rpc/rpc.h>

#include <clientserver/export.h>
#include "writer.h"

std::pair<XDR*, XDR*> CreateXDRStream(IoData* io_data);

#endif // UDA_SERVER_CREATEXDRSTREAM_H
