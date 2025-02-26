#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

void init_request_data(RequestData* str);

void init_client_block(ClientBlock* str, int version, const char* clientname);

void init_server_block(ServerBlock* str, int version);

void init_data_block(DataBlock* str);

void init_dim_block(Dims* str);

void init_put_data_block(PutDataBlock* str);

} // namespace uda::client_server
