#pragma once

#include <rpc/types.h>
#include <rpc/xdr.h>

namespace uda::client_server
{

int send_xdr_file(XDR* xdrs, const char* xdrfile);

int receive_xdr_file(XDR* xdrs, const char* xdrfile);

} // namespace uda::client_server
