#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <clientserver/socketStructs.h>
#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API extern int clientVersion;                   // Client Library Version
LIBRARY_API extern char clientUsername[STRING_LENGTH];  // Only obtain userid once

LIBRARY_API int connectionOpen();
LIBRARY_API int reconnect(ENVIRONMENT* environment);
LIBRARY_API int createConnection();
LIBRARY_API void closeConnection(int type);

LIBRARY_API int clientWriteout(void* iohandle, char* buf, int count);
LIBRARY_API int clientReadin(void* iohandle, char* buf, int count);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CONNECTION_H
