#pragma once

#include "clientserver/parse_xml.h"
#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"
#include "config/config.h"

namespace uda::server
{

int server_subset_data(client_server::DataBlock* data_block, client_server::Action action,
                       structures::LogMallocList* logmalloclist);

int server_parse_server_side(config::Config& config, client_server::RequestData* request_block,
                             client_server::Actions* actions_serverside);

} // namespace uda
