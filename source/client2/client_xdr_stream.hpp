#pragma once

#include <rpc/rpc.h>

#include "connection.hpp"

namespace uda::client
{
void xdr_deleter(XDR* xdr_ptr);

std::pair<std::unique_ptr<XDR, void(*)(XDR*)>, std::unique_ptr<XDR, void(*)(XDR*)>> create_xdr_stream(IoData* io_data);

}

