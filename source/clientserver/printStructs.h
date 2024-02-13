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

void print_system_config(SystemConfig str);

void print_data_system(DataSystem str);

void print_data_source(DataSource str);

void print_signal(Signal str);

void print_signal_desc(SignalDesc str);

} // namespace uda::client_server
