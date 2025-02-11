#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void print_request_data(RequestData str);

void print_request_block(RequestBlock str);

void print_client_block(ClientBlock str);

void print_server_block(ServerBlock str);

void print_data_block_list(DataBlockList str);

void print_data_block(DataBlock str);

void print_meta_data(MetaData str);

} // namespace uda::client_server
