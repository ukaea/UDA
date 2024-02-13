#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void print_request_data(REQUEST_DATA str);

void print_request_block(REQUEST_BLOCK str);

void print_client_block(CLIENT_BLOCK str);

void print_server_block(SERVER_BLOCK str);

void print_data_block_list(DATA_BLOCK_LIST str);

void print_data_block(DATA_BLOCK str);

void print_system_config(SYSTEM_CONFIG str);

void print_data_system(DATA_SYSTEM str);

void print_data_source(DATA_SOURCE str);

void print_signal(SIGNAL str);

void print_signal_desc(SIGNAL_DESC str);

} // namespace uda::client_server
