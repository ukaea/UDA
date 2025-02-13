#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

int name_value_substitution(std::vector<UdaError>& error_stack, NameValueList& name_value_list, const char* tpass);

}
