#pragma once

#include "structures/genStructs.h"
#include "clientserver/udaStructs.h"
#include "serverPlugin.h"
#include "clientserver/parseXML.h"
#include "clientserver/socketStructs.h"

int udaGetData(int* depth, REQUEST_DATA* request_data, CLIENT_BLOCK client_block, DATA_BLOCK* data_block,
               DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc, ACTIONS* actions_desc,
               ACTIONS* actions_sig, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
               USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list, int protocolVersion);
