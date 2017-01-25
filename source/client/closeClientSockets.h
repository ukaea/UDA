#ifndef IDAM_CLIENT_CLOSECLIENTSOCKETS_H
#define IDAM_CLIENT_CLOSECLIENTSOCKETS_H

#include "idamclientserver.h"

void closeNamedIdamClientSocket(SOCKETLIST *socks, char *host, int port);
void closeIdamClientSockets(SOCKETLIST *socks);
void closeIdamClientSocket(SOCKETLIST *socks, int fh);

#endif // IDAM_CLIENT_CLOSECLIENTSOCKETS_H
