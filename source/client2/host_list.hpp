#pragma once

#include <string_view>
#include <vector>
#include <unordered_map>

#include "clientserver/socket_structs.h"
#include "config/config.h"

namespace uda::client
{

class HostList
{
  public:
    HostList(); //uses weird default paths and env vars
    explicit HostList(std::string_view config_file);
    explicit HostList(const uda::config::Config& config);
    ~HostList() = default;

    inline bool empty()
    {
        return hosts_.empty();
    }

    inline size_t size()
    {
        return hosts_.size();
    }

    void load_from_default_locations();
    void load_from_file(std::string_view file_path);
    [[nodiscard]] const uda::client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const uda::client_server::HostData* find_by_name(std::string_view name) const;

    inline const std::vector<uda::client_server::HostData>& get_host_list() const
    {
        return hosts_;
    }
  private:
    //TODO: change to unordered map with alias as key to avoid duplicate names
    std::vector<uda::client_server::HostData> hosts_;
    // std::unordered_map<std::string, uda::client_server::HostData> hosts_map_;

    void load_list_from_toml(const config::Config& config);
    void load_list_from_custom_file_format(std::string_view file_path);
};

} // namespace uda::client
