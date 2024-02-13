#pragma once

#ifndef UDA_SERVER_APPLYXML_H
#define UDA_SERVER_APPLYXML_H

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "include/uda/export.h"

namespace uda {

int server_parse_signal_XML(uda::client_server::DataSource data_source, uda::client_server::Signal signal, uda::client_server::SignalDesc signal_desc,
                            uda::client_server::Actions* actions_desc, uda::client_server::Actions* actions_sig);

void server_apply_signal_XML(uda::client_server::ClientBlock client_block, uda::client_server::DataSource* data_source, uda::client_server::Signal* signal,
                             uda::client_server::SignalDesc* signal_desc, uda::client_server::DataBlock* data_block, uda::client_server::Actions actions);

void server_deselect_signal_XML(uda::client_server::Actions* actions_desc, uda::client_server::Actions* actions_sig);

} // namespace uda

#endif // UDA_SERVER_APPLYXML_H
