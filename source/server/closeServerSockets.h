#ifndef IDAM_SERVER_CLOSESERVERSOCKETS_H
#define IDAM_SERVER_CLOSESERVERSOCKETS_H

#include <clientserver/socketStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void closeNamedServerSocket(SOCKETLIST *socks, char *host, int port);
void closeServerSocket(SOCKETLIST *socks, int fh);
void closeServerSockets(SOCKETLIST *socks);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_CLOSESERVERSOCKETS_H
