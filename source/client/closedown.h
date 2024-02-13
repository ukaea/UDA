#pragma once

#include "clientserver/socketStructs.h"

#ifdef FATCLIENT
#  define closedown closedownFat
#endif

namespace uda::client {

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

int closedown(ClosedownType type, uda::client_server::SOCKETLIST *socket_list, XDR *client_input, XDR *client_output,
              bool *reopen_logs);

}
