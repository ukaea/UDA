#pragma once

#include "udaStructs.h"

namespace uda::client_server
{

void init_name_value_list(NameValueList* nameValueList);

void init_request_data(RequestData* str);

void init_request_block(RequestBlock* str);

void init_client_block(ClientBlock* str, int version, const char* clientname);

void init_server_block(ServerBlock* str, int version);

void init_data_block(DataBlock* str);

void init_data_block_list(DataBlockList* str);

void init_dim_block(Dims* str);

void init_data_system(DataSystem* str);

void init_system_config(SystemConfig* str);

void init_data_source(DataSource* str);

void init_signal(Signal* str);

void init_signal_desc(SignalDesc* str);

void init_put_data_block(PutDataBlock* str);

void init_put_data_block_list(PutDataBlockList* putDataBlockList);

} // namespace uda::client_server
