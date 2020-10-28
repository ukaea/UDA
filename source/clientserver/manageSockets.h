#ifndef UDA_CLIENTSERVER_MANAGESOCKETS_H
#define UDA_CLIENTSERVER_MANAGESOCKETS_H

#include "socketStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialise
LIBRARY_API void initSocketList(SOCKETLIST *socks);

// Add a New Socket to the Socket List
LIBRARY_API int addSocket(SOCKETLIST *socks, int type, int status, char *host, int port, int fh);

// Search for an Open Socket in the Socket List
LIBRARY_API int getSocket(SOCKETLIST *socks, int type, int *status, char *host, int port, int *fh);

// Search for an Open Socket in the Socket List
LIBRARY_API int getSocketRecordId(SOCKETLIST *socks, int fh);

LIBRARY_API void closeClientSockets(SOCKETLIST* socks);

LIBRARY_API void closeClientSocket(SOCKETLIST* socks, int fh);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_MANAGESOCKETS_H

