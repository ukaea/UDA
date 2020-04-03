#ifndef UDA_SERVER_CLOSESERVERSOCKETS_H
#define UDA_SERVER_CLOSESERVERSOCKETS_H

#include <clientserver/socketStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void closeServerSocket(SOCKETLIST *socks, int fh);
LIBRARY_API void closeServerSockets(SOCKETLIST *socks);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_CLOSESERVERSOCKETS_H
