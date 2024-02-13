#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void copy_request_data(REQUEST_DATA* out, REQUEST_DATA in);

void copy_request_block(REQUEST_BLOCK* out, REQUEST_BLOCK in);

} // namespace uda::client_server
