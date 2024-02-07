#pragma once

#ifndef UDA_SERVER_APPLYXML_H
#  define UDA_SERVER_APPLYXML_H

#  include "clientserver/parseXML.h"
#  include "clientserver/udaStructs.h"

int serverParseSignalXML(DATA_SOURCE data_source, SIGNAL signal, SIGNAL_DESC signal_desc, ACTIONS* actions_desc,
                         ACTIONS* actions_sig);

void serverApplySignalXML(CLIENT_BLOCK client_block, DATA_SOURCE* data_source, SIGNAL* signal, SIGNAL_DESC* signal_desc,
                          DATA_BLOCK* data_block, ACTIONS actions);

void serverDeselectSignalXML(ACTIONS* actions_desc, ACTIONS* actions_sig);

#endif // UDA_SERVER_APPLYXML_H
