#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#define UDA_CLIENT_CONNECTION_H

#include <clientserver/socketStructs.h>
#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#include "closedown.h"

#ifdef __cplusplus
extern "C" {
#endif

int connectionOpen();
// Called from a second thread while an API request is blocked.
// Cancels the entire TCP session (POSIX, no TLS, xinetd server only).
LIBRARY_API int cancelConnection();
// Checked by the request thread; do not destroy XDR from the cancelling thread.
int connectionCancelled();
int reconnect(ENVIRONMENT* environment, XDR** client_input, XDR** client_output, time_t* tv_server_start,
                          int* user_timeout);
int createConnection(XDR* client_input, XDR* client_output, time_t *tv_server_start, int user_timeout, int client_version);
LIBRARY_API void closeAllConnections();
void closeConnection(ClosedownType type);

int clientWriteout(void* iohandle, char* buf, int count);
int clientReadin(void* iohandle, char* buf, int count);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_CONNECTION_H
