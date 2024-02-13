#pragma once

#include "clientserver/socketStructs.h"

namespace uda::server
{

void closeServerSocket(uda::client_server::SOCKETLIST* socks, int fh);

void closeServerSockets(uda::client_server::SOCKETLIST* socks);

} // namespace uda::server
