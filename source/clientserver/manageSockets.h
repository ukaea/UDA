#ifndef UDA_CLIENTSERVER_MANAGESOCKETS_H
#define UDA_CLIENTSERVER_MANAGESOCKETS_H

#include <clientserver/socketStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

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

