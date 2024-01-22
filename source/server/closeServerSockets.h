#pragma once

#ifndef UDA_SERVER_CLOSESERVERSOCKETS_H
#  define UDA_SERVER_CLOSESERVERSOCKETS_H

#  include "export.h"
#  include <clientserver/socketStructs.h>

void closeServerSocket(SOCKETLIST* socks, int fh);
void closeServerSockets(SOCKETLIST* socks);

#endif // UDA_SERVER_CLOSESERVERSOCKETS_H
