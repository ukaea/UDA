#ifndef IDAM_CLIENTSERVER_MANAGESOCKETS_H
#define IDAM_CLIENTSERVER_MANAGESOCKETS_H

#include <clientserver/socketStructs.h>

// Initialise
void initSocketList(SOCKETLIST *socks);

// Add a New Socket to the Socket List
int addSocket(SOCKETLIST *socks, int type, int status, char *host, int port, int fh);

// Search for an Open Socket in the Socket List
int getSocket(SOCKETLIST *socks, int type, int *status, char *host, int port, int *fh);

// Search for an Open Socket in the Socket List
int getSocketRecordId(SOCKETLIST *socks, int fh);

#endif // IDAM_CLIENTSERVER_MANAGESOCKETS_H

