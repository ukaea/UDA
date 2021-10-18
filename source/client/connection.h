#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <clientserver/socketStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#include "closedown.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int connectionOpen();
LIBRARY_API int reconnect(ENVIRONMENT* environment, XDR** client_input, XDR** client_output, time_t* tv_server_start,
                          int* user_timeout);
LIBRARY_API int createConnection(XDR* client_input, XDR* client_output);
LIBRARY_API void closeConnection(ClosedownType type);

LIBRARY_API int clientWriteout(void* iohandle, char* buf, int count);
LIBRARY_API int clientReadin(void* iohandle, char* buf, int count);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CONNECTION_H
