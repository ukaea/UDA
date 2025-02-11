#pragma once

#include "clientserver/parse_xml.h"
#include "clientserver/udaStructs.h"

namespace uda::server {

int swap_signal_error(client_server::DataBlock *data_block, client_server::DataBlock *data_block2, int asymmetry);

int swap_signal_dim(client_server::DimComposite dim_composite, client_server::DataBlock* data_block, client_server::DataBlock* data_block2);

int swap_signal_dim_error(client_server::DimComposite dim_composite, client_server::DataBlock* data_block, client_server::DataBlock* data_block2, int asymmetry);

} // uda::server
