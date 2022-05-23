#pragma once

#ifndef UDA_SERVER_SERVERPROCESSING_H
#define UDA_SERVER_SERVERPROCESSING_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

namespace uda {

int serverProcessing(ClientBlock client_block, DataBlock* data_block);

} // namespace uda

#endif // UDA_SERVER_SERVERPROCESSING_H

