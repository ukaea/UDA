#pragma once

#include "clientserver/parseXML.h"
#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"

namespace uda::server
{

int server_subset_data(client_server::DataBlock* data_block, client_server::Action action,
                       structures::LogMallocList* logmalloclist);

int server_parse_server_side(client_server::RequestData* request_block,
                             client_server::Actions* actions_serverside,
                             client_server::Environment* environment);

} // namespace uda
