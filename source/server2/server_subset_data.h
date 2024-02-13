#pragma once

#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#  define UDA_SERVER_SERVERSUBSETDATA_H

#  include "clientserver/parseXML.h"
#  include "clientserver/udaStructs.h"
#  include "structures/genStructs.h"

namespace uda
{

int serverSubsetData(uda::client_server::DataBlock* data_block, uda::client_server::Action action,
                     LOGMALLOCLIST* logmalloclist);

int serverParseServerSide(uda::client_server::RequestData* request_block,
                          uda::client_server::Actions* actions_serverside,
                          uda::client_server::Environment* environment);

} // namespace uda

#endif // UDA_SERVER_SERVERSUBSETDATA_H
