#pragma once

#include <string_view>
#include <vector>

#include "clientserver/socket_structs.h"

namespace uda::client
{

class HostList
{
  public:
    HostList(); //uses weird default paths and env vars
    explicit HostList(std::string_view config_file);
    ~HostList() = default;

    [[nodiscard]] const client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const client_server::HostData* find_by_name(std::string_view name) const;
    [[nodiscard]] const uda::client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const uda::client_server::HostData* find_by_name(std::string_view name) const;

  private:
    std::vector<uda::client_server::HostData> hosts_;
    void load_config_file(std::string_view config_file);
};

} // namespace uda::client
