#pragma once

#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#  define UDA_SERVER_SERVERSUBSETDATA_H

#  include "clientserver/parseXML.h"
#  include "clientserver/udaStructs.h"
#  include "structures/genStructs.h"

namespace uda
{

int serverSubsetData(uda::client_server::DATA_BLOCK* data_block, uda::client_server::ACTION action,
                     LOGMALLOCLIST* logmalloclist);

int serverParseServerSide(uda::client_server::REQUEST_DATA* request_block,
                          uda::client_server::ACTIONS* actions_serverside,
                          uda::client_server::Environment* environment);

} // namespace uda

#endif // UDA_SERVER_SERVERSUBSETDATA_H
