#pragma once

#ifndef UDA_SERVER_APPLYXML_H
#define UDA_SERVER_APPLYXML_H

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "include/uda/export.h"

namespace uda {

int server_parse_signal_XML(uda::client_server::DATA_SOURCE data_source, uda::client_server::SIGNAL signal, uda::client_server::SIGNAL_DESC signal_desc,
                            uda::client_server::ACTIONS* actions_desc, uda::client_server::ACTIONS* actions_sig);

void server_apply_signal_XML(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_SOURCE* data_source, uda::client_server::SIGNAL* signal,
                             uda::client_server::SIGNAL_DESC* signal_desc, uda::client_server::DATA_BLOCK* data_block, uda::client_server::ACTIONS actions);

void server_deselect_signal_XML(uda::client_server::ACTIONS* actions_desc, uda::client_server::ACTIONS* actions_sig);

} // namespace uda

#endif // UDA_SERVER_APPLYXML_H
