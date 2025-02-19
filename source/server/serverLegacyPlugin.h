#pragma once

#include "clientserver/uda_structs.h"

namespace uda::server
{

int udaServerLegacyPlugin(uda::client_server::RequestData* request, uda::client_server::DataSource* data_source,
                          uda::client_server::SignalDesc* signal_desc);

}
