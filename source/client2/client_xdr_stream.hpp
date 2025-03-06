#pragma once

#include <rpc/rpc.h>

#include "connection.hpp"

namespace uda::client
{

struct XDRDeleter {
    void operator()(XDR* ptr) const {
        if (ptr) {
            if (ptr->x_ops != nullptr) {
                xdr_destroy(ptr);
            }
            delete ptr;
        }
    }
};

std::pair<std::unique_ptr<XDR, XDRDeleter>, std::unique_ptr<XDR, XDRDeleter>> create_xdr_stream(IoData* io_data);

}

