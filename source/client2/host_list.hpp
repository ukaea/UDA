#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "clientserver/socket_structs.h"
#include "config/config.h"

namespace uda::client
{

class HostList
{
  public:
    HostList(); // uses weird default paths and env vars
    explicit HostList(std::string_view config_file);
    explicit HostList(const config::Config& config);
    explicit HostList(std::istream& stream, std::string_view source_path, bool is_toml=true);

    [[nodiscard]] bool empty() const {
        return hosts_.empty();
    }

    [[nodiscard]] size_t size() const {
        return hosts_.size();
    }

    void load_from_default_locations();
    void load_from_file(std::string_view file_path);
    void load_from_stream(std::istream& stream, std::string_view source_path);

    [[nodiscard]] const client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const client_server::HostData* find_by_name(std::string_view name) const;

    [[nodiscard]] const std::vector<client_server::HostData>& get_host_list() const
    {
        return hosts_;
    }
  private:
    //TODO: change to unordered map with alias as key to avoid duplicate names
    std::vector<client_server::HostData> hosts_;
    // std::unordered_map<std::string, client_server::HostData> hosts_map_;

    void load_list_from_toml(const config::Config& config);
    void load_list_from_custom_file_format(std::string_view file_path);
    void load_list_from_custom_stream(std::istream& stream);
};

} // namespace uda::client
