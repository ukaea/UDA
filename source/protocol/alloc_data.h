#pragma once

#include "clientserver/uda_structs.h"

namespace uda::client_server
{

int alloc_array(int data_type, size_t ndata, char** ap);

int alloc_data(DataBlock* data_block);

int alloc_dim(DataBlock* data_block);

int alloc_put_data(PutDataBlock* putData);

void add_put_data_block_list(PutDataBlock* putDataBlock, PutDataBlockList* putDataBlockList);

} // namespace uda::client_server