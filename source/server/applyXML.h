#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"

namespace uda::server
{

int serverParseSignalXML(uda::client_server::DATA_SOURCE data_source, uda::client_server::SIGNAL signal,
                         uda::client_server::SIGNAL_DESC signal_desc, uda::client_server::ACTIONS* actions_desc,
                         uda::client_server::ACTIONS* actions_sig);

void serverApplySignalXML(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_SOURCE* data_source,
                          uda::client_server::SIGNAL* signal, uda::client_server::SIGNAL_DESC* signal_desc,
                          uda::client_server::DATA_BLOCK* data_block, uda::client_server::ACTIONS actions);

void serverDeselectSignalXML(uda::client_server::ACTIONS* actions_desc, uda::client_server::ACTIONS* actions_sig);

} // namespace uda::server
