#pragma once

#ifndef UDA_CLIENT_CONNECTION_H
#  define UDA_CLIENT_CONNECTION_H

#  include "export.h"
#  include "udaStructs.h"
#  include <clientserver/socketStructs.h>

#  include "closedown.h"

#  ifdef __cplusplus
extern "C" {
#  endif

int connectionOpen();
int reconnect(ENVIRONMENT* environment, XDR** client_input, XDR** client_output, time_t* tv_server_start,
              int* user_timeout);
int createConnection(XDR* client_input, XDR* client_output, time_t* tv_server_start, int user_timeout);
void closeConnection(ClosedownType type);

int clientWriteout(void* iohandle, char* buf, int count);
int clientReadin(void* iohandle, char* buf, int count);

#  ifdef __cplusplus
}
#  endif

#endif // UDA_CLIENT_CONNECTION_H
