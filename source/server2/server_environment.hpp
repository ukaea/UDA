#pragma once

#ifndef UDA_SERVER_GETSERVERENVIRONMENT_HPP
#define UDA_SERVER_GETSERVERENVIRONMENT_HPP

#include "clientserver/udaStructs.h"
#include "export.h"

namespace uda {
namespace server {

class Environment
{
public:
    Environment();

    void print();
    [[nodiscard]] ::Environment* p_env() { return &environment_; }
    [[nodiscard]] const ::Environment* p_env() const { return &environment_; }
    ::Environment* operator->() { return &environment_; }
    const ::Environment* operator->() const { return &environment_; }

private:
    ::Environment environment_ = {};
};

} // namespace server
} // namespace uda

#endif // UDA_SERVER_GETSERVERENVIRONMENT_HPP
