#pragma once

using XDR = struct __rpc_xdr;

namespace uda::client {

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

class Connection;

int closedown(ClosedownType type, Connection* connection, XDR* client_input, XDR* client_output, bool* reopen_logs,
              bool* env_host, bool* env_port);

} // namespace uda::client
