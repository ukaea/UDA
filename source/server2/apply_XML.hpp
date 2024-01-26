#pragma once

#ifndef UDA_SERVER_APPLYXML_H
#  define UDA_SERVER_APPLYXML_H

#  include "export.h"
#  include "udaStructs.h"
#  include <clientserver/parseXML.h>

namespace uda
{

int server_parse_signal_XML(DATA_SOURCE data_source, SIGNAL signal, SIGNAL_DESC signal_desc, ACTIONS* actions_desc,
                            ACTIONS* actions_sig);

void server_apply_signal_XML(CLIENT_BLOCK client_block, DATA_SOURCE* data_source, SIGNAL* signal,
                             SIGNAL_DESC* signal_desc, DATA_BLOCK* data_block, ACTIONS actions);

void server_deselect_signal_XML(ACTIONS* actions_desc, ACTIONS* actions_sig);

} // namespace uda

#endif // UDA_SERVER_APPLYXML_H
