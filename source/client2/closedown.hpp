#pragma once

using XDR = struct __rpc_xdr;

namespace uda::client {

enum class ClosedownType {
    CLOSE_SOCKETS = 0,
    CLOSE_ALL = 1,
};

class Connection;

int closedown(ClosedownType type, Connection* connection);
void close_xdr_stream(XDR*);

} // namespace uda::client
