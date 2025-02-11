#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

void print_request_data(const RequestData& str);

void print_request_block(const RequestBlock& str);

void print_client_block(const ClientBlock& str);

void print_server_block(const ServerBlock& str);

void print_data_block_list(const std::vector<DataBlock>& str);

void print_data_block(const DataBlock& str);

void print_meta_data(const MetaData& str);

} // namespace uda::client_server
