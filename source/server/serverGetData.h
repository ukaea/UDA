#pragma once

#ifndef UDA_SERVER_SERVERGETDATA_H
#  define UDA_SERVER_SERVERGETDATA_H

#  include "export.h"
#  include "genStructs.h"
#  include "pluginStructs.h"
#  include "udaStructs.h"
#  include <clientserver/parseXML.h>
#  include <clientserver/socketStructs.h>

int udaGetData(int* depth, REQUEST_DATA* request_data, CLIENT_BLOCK client_block, DATA_BLOCK* data_block,
               DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc, ACTIONS* actions_desc,
               ACTIONS* actions_sig, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
               USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion);

#endif // UDA_SERVER_SERVERGETDATA_H
