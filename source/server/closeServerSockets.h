
#ifndef IDAM_CLOSESERVERSOCKETS_H

#include "idamclientserver.h"

#ifndef	NOMDSPLUSPLUGIN
#  include <mdslib.h>
#endif

void closeNamedServerSocket(SOCKETLIST *socks, char *host, int port);
void closeServerSocket(SOCKETLIST *socks, int fh);
void closeServerSockets(SOCKETLIST *socks);

#endif // IDAM_CLOSESERVERSOCKETS_H
