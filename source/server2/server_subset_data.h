#pragma once

#include "clientserver/uda_structs.h"
#include "structures/genStructs.h"
#include "config/config.h"

namespace uda::server
{

int server_subset_data(client_server::DataBlock* data_block, client_server::Subset subset,
                       structures::LogMallocList* logmalloclist);

int server_parse_server_side(config::Config& config, client_server::RequestData* request_block,
                             client_server::Subset* subset);

} // namespace uda
