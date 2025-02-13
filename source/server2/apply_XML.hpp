#pragma once

#include "clientserver/parse_xml.h"
#include "clientserver/uda_structs.h"

namespace uda
{

int server_parse_signal_xml(std::vector<client_server::UdaError>& error_stack,
    client_server::MetaData meta_data, client_server::Actions* actions_desc,
                            client_server::Actions* actions_sig);

void server_apply_signal_xml(client_server::ClientBlock client_block, client_server::MetaData* meta_data,
                             client_server::DataBlock* data_block, client_server::Actions actions);

void server_deselect_signal_xml(client_server::Actions* actions_desc, client_server::Actions* actions_sig);

} // namespace uda
