#pragma once

#ifndef UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H
#define UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H

#include "udaStructs.h"
#include "export.h"

namespace uda {
namespace client {

int make_request_block(const Environment* environment, const char** signals, const char** sources, int count,
                       REQUEST_BLOCK* request_block);

}
}

#endif // UDA_CLIENT_MAKECLIENTREQUESTBLOCK_H