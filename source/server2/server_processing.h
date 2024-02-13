#pragma once

#ifndef UDA_SERVER_SERVERPROCESSING_H
#  define UDA_SERVER_SERVERPROCESSING_H

#  include "clientserver/udaStructs.h"

namespace uda
{

int serverProcessing(uda::client_server::ClientBlock client_block, uda::client_server::DataBlock* data_block);

} // namespace uda

#endif // UDA_SERVER_SERVERPROCESSING_H
