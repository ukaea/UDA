#pragma once

#include "clientserver/udaStructs.h"

namespace uda::config {
class Config;
}

namespace uda::server {

class Environment
{
  public:
    Environment(const config::Config& config);

    void print();
    [[nodiscard]] uda::client_server::Environment* p_env() { return &_environment; }
    [[nodiscard]] const uda::client_server::Environment* p_env() const { return &_environment; }
    uda::client_server::Environment* operator->() { return &_environment; }
    const uda::client_server::Environment* operator->() const { return &_environment; }

  private:
    const config::Config& _config;
    uda::client_server::Environment _environment = {};
};

} // uda::server
