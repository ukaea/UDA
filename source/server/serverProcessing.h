#pragma once

#ifndef UDA_SERVER_SERVERPROCESSING_H
#  define UDA_SERVER_SERVERPROCESSING_H

#  include "clientserver/udaStructs.h"

int serverProcessing(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_BLOCK* data_block);

#endif // UDA_SERVER_SERVERPROCESSING_H
