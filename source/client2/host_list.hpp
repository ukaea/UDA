#pragma once

#include <string_view>
#include <vector>

#include "clientserver/socket_structs.h"
#include "config/config.h"

namespace uda::client
{

class HostList
{
  public:
    HostList(); //uses weird default paths and env vars
    explicit HostList(std::string_view config_file);
    ~HostList() = default;

    void load_config_file(std::string_view config_file);
    void load_list_from_toml(config::Config& config); //won't replace anything found in default locations...
    [[nodiscard]] const uda::client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const uda::client_server::HostData* find_by_name(std::string_view name) const;

    inline const std::vector<uda::client_server::HostData>& get_host_list() const
    {
        return hosts_;
    }
  private:
    std::vector<uda::client_server::HostData> hosts_;
};

} // namespace uda::client
