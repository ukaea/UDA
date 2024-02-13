#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_HPP
#define UDA_SERVER_GETSERVERENVIRONMENT_HPP

#include "clientserver/udaStructs.h"

namespace uda {
namespace server {

class Environment
{
public:
    Environment();

    void print();
    [[nodiscard]] uda::client_server::Environment* p_env() { return &environment_; }
    [[nodiscard]] const uda::client_server::Environment* p_env() const { return &environment_; }
    uda::client_server::Environment* operator->() { return &environment_; }
    const uda::client_server::Environment* operator->() const { return &environment_; }

private:
    uda::client_server::Environment environment_ = {};
};

} // namespace server
} // namespace uda

#endif // UDA_SERVER_GETSERVERENVIRONMENT_HPP
