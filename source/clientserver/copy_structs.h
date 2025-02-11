#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

void copy_request_data(RequestData* out, RequestData in);

void copy_request_block(RequestBlock* out, RequestBlock in);

} // namespace uda::client_server
