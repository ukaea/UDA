#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void init_name_value_list(NAMEVALUELIST* nameValueList);

void init_request_data(REQUEST_DATA* str);

void init_request_block(REQUEST_BLOCK* str);

void init_client_block(CLIENT_BLOCK* str, int version, const char* clientname);

void init_server_block(SERVER_BLOCK* str, int version);

void init_data_block(DATA_BLOCK* str);

void init_data_block_list(DATA_BLOCK_LIST* str);

void init_dim_block(DIMS* str);

void init_data_system(DATA_SYSTEM* str);

void init_system_config(SYSTEM_CONFIG* str);

void init_data_source(DATA_SOURCE* str);

void init_signal(SIGNAL* str);

void init_signal_desc(SIGNAL_DESC* str);

void init_put_data_block(PutDataBlock* str);

void init_put_data_block_list(PutDataBlockList* putDataBlockList);

} // namespace uda::client_server
