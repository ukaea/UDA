#pragma once

#  include "clientserver/parseXML.h"
#  include "clientserver/udaStructs.h"

#  include "clientserver/udaStructs.h"
#  include "plugins.hpp"
#  include "xdr_protocol.hpp"

namespace uda::server {

int swap_signal_error(client_server::DataBlock *data_block, client_server::DataBlock *data_block2, int asymmetry);

int swap_signal_dim(client_server::DimComposite dim_composite, client_server::DataBlock* data_block, client_server::DataBlock* data_block2);

int swap_signal_dim_error(client_server::DimComposite dim_composite, client_server::DataBlock* data_block, client_server::DataBlock* data_block2, int asymmetry);

} // uda::server
