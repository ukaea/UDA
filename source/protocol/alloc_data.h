#pragma once

#include "clientserver/uda_structs.h"

namespace uda::protocol
{

int alloc_array(int data_type, size_t ndata, char** ap);

int alloc_data(client_server::DataBlock* data_block);

int alloc_dim(client_server::DataBlock* data_block);

int alloc_put_data(client_server::PutDataBlock* putData);

void add_put_data_block_list(client_server::PutDataBlock* putDataBlock, client_server::PutDataBlockList* putDataBlockList);

} // namespace uda::protocol