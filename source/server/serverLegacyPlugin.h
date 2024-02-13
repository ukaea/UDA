#pragma once

#include "clientserver/udaStructs.h"

namespace uda::server
{

int udaServerLegacyPlugin(uda::client_server::REQUEST_DATA* request, uda::client_server::DATA_SOURCE* data_source,
                          uda::client_server::SIGNAL_DESC* signal_desc);

}
