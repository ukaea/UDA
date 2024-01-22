#pragma once

#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#  define UDA_SERVER_SERVERSUBSETDATA_H

#  include "export.h"
#  include "genStructs.h"
#  include "udaStructs.h"
#  include <clientserver/parseXML.h>

namespace uda
{

int serverSubsetData(DATA_BLOCK* data_block, ACTION action, LOGMALLOCLIST* logmalloclist);

int serverParseServerSide(REQUEST_DATA* request_block, ACTIONS* actions_serverside, Environment* environment);

} // namespace uda

#endif // UDA_SERVER_SERVERSUBSETDATA_H
