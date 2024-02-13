#pragma once

#include "clientserver/socketStructs.h"
#include "clientserver/udaStructs.h"

#include "closedown.h"

namespace uda::client {

int connectionOpen();

int reconnect(uda::client_server::ENVIRONMENT *environment, XDR **client_input, XDR **client_output,
              time_t *tv_server_start, int *user_timeout);

int createConnection(XDR *client_input, XDR *client_output, time_t *tv_server_start, int user_timeout);

void closeConnection(ClosedownType type);

int clientWriteout(void *iohandle, char *buf, int count);

int clientReadin(void *iohandle, char *buf, int count);

}
