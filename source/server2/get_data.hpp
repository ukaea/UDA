#pragma once

#include "clientserver/uda_structs.h"

namespace uda::server {

int swap_signal_error(client_server::DataBlock *data_block, client_server::DataBlock *data_block2, int asymmetry);

int swap_signal_dim(int from_dim, int to_dim, client_server::DataBlock* data_block, client_server::DataBlock* data_block2);

int swap_signal_dim_error(int from_dim, int to_dim, client_server::DataBlock* data_block, client_server::DataBlock* data_block2, int asymmetry);

} // uda::server
