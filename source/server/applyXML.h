#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/uda_structs.h"

namespace uda::server
{

int serverParseSignalXML(uda::client_server::DataSource data_source, uda::client_server::Signal signal,
                         uda::client_server::SignalDesc signal_desc, uda::client_server::Actions* actions_desc,
                         uda::client_server::Actions* actions_sig);

void serverApplySignalXML(uda::client_server::ClientBlock client_block, uda::client_server::DataSource* data_source,
                          uda::client_server::Signal* signal, uda::client_server::SignalDesc* signal_desc,
                          uda::client_server::DataBlock* data_block, uda::client_server::Actions actions);

void serverDeselectSignalXML(uda::client_server::Actions* actions_desc, uda::client_server::Actions* actions_sig);

} // namespace uda::server
