#pragma once

#include "uda_structs.h"

namespace uda::client_server
{

int parse_operation(std::vector<UdaError>& error_stack, Subset* sub);
void init_subset(Subset* sub);
void print_subset(const Subset& sub);

}
