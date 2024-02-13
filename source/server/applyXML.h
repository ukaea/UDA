#pragma once

#ifndef UDA_SERVER_APPLYXML_H
#  define UDA_SERVER_APPLYXML_H

#  include "clientserver/parseXML.h"
#  include "clientserver/udaStructs.h"

int serverParseSignalXML(uda::client_server::DATA_SOURCE data_source, uda::client_server::SIGNAL signal,
                         uda::client_server::SIGNAL_DESC signal_desc, uda::client_server::ACTIONS* actions_desc,
                         uda::client_server::ACTIONS* actions_sig);

void serverApplySignalXML(uda::client_server::CLIENT_BLOCK client_block, uda::client_server::DATA_SOURCE* data_source,
                          uda::client_server::SIGNAL* signal, uda::client_server::SIGNAL_DESC* signal_desc,
                          uda::client_server::DATA_BLOCK* data_block, uda::client_server::ACTIONS actions);

void serverDeselectSignalXML(uda::client_server::ACTIONS* actions_desc, uda::client_server::ACTIONS* actions_sig);

#endif // UDA_SERVER_APPLYXML_H
