#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <clientserver/socketStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#include "closedown.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API extern int clientVersion;                   // Client Library Version
LIBRARY_API extern char clientUsername[STRING_LENGTH];  // Only obtain userid once

LIBRARY_API int connectionOpen();
LIBRARY_API int reconnect(ENVIRONMENT* environment);
LIBRARY_API int createConnection();
LIBRARY_API void closeConnection(ClosedownType type);

LIBRARY_API int resetClientConnection();
LIBRARY_API void closeClientConnection();

LIBRARY_API int clientWriteout(void* iohandle, char* buf, int count);
LIBRARY_API int clientReadin(void* iohandle, char* buf, int count);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CONNECTION_H
