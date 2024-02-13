#ifndef UDA_CLIENT_CLOSEDOWN_H
#define UDA_CLIENT_CLOSEDOWN_H

#include "clientserver/socketStructs.h"

#ifdef FATCLIENT
#  define closedown closedownFat
#endif

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

int closedown(ClosedownType type, uda::client_server::SOCKETLIST* socket_list, XDR* client_input, XDR* client_output,
              bool* reopen_logs);

#endif // UDA_CLIENT_CLOSEDOWN_H
