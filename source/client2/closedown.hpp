#pragma once

#ifndef UDA_CLIENT_CLOSEDOWN_H
#define UDA_CLIENT_CLOSEDOWN_H

#include <clientserver/socketStructs.h>
#include "export.h"

namespace uda {
namespace client {

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

class Connection;

int
closedown(ClosedownType type, Connection* connection, XDR* client_input, XDR* client_output, bool* reopen_logs,
          bool* env_host, bool* env_port);

}
}

#endif // UDA_CLIENT_CLOSEDOWN_H
