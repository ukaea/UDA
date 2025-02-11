#pragma once

#include <string_view>
#include <vector>

#include "clientserver/socket_structs.h"

namespace uda::client
{

class HostList
{
  public:
    HostList();

    [[nodiscard]] const client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const client_server::HostData* find_by_name(std::string_view name) const;

  private:
    std::vector<client_server::HostData> hosts_;
};

} // namespace uda::client
