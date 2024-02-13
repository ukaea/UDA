#pragma once

#include "clientserver/udaStructs.h"

namespace uda::server
{

void printServerEnvironment(const uda::client_server::Environment* environment);

uda::client_server::Environment* getServerEnvironment();

} // namespace uda::server
