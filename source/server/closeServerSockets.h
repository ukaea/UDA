#pragma once

#ifndef UDA_SERVER_CLOSESERVERSOCKETS_H
#  define UDA_SERVER_CLOSESERVERSOCKETS_H

#  include "clientserver/socketStructs.h"

void closeServerSocket(uda::client_server::SOCKETLIST* socks, int fh);
void closeServerSockets(uda::client_server::SOCKETLIST* socks);

#endif // UDA_SERVER_CLOSESERVERSOCKETS_H
