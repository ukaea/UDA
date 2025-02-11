#pragma once

#include <boost/optional.hpp>
#include <vector>
#include <filesystem>

#include "clientserver/udaDefines.h"
#include "clientserver/udaStructs.h"
#include "structures/genStructs.h"
#include "clientserver/plugins.h"
#include <uda/plugins.h>

namespace uda::config {
class Config;
}

typedef struct CUdaPluginInterface {
} UDA_PLUGIN_INTERFACE;

namespace uda::server
{

// Standard Plugin interface
struct UdaPluginInterface : UDA_PLUGIN_INTERFACE {
    int interface_version;               // Interface Version
    int plugin_version;
    bool house_keeping;                   // Housekeeping Directive
    bool change_plugin;                   // Use a different Plugin to access the data
    client_server::DataBlock* data_block;
    client_server::RequestData* request_data;
    client_server::ClientBlock* client_block;
    client_server::MetaData* meta_data;
    structures::LogMallocList* log_malloc_list;
    structures::UserDefinedTypeList* user_defined_type_list;
    const std::vector<client_server::PluginData>* pluginList; // List of data readers, filters, models, and servers
    std::vector<client_server::UdaError> error_stack;
    const config::Config* config;
};

struct PluginList {};

class Plugins
{
  public:
    explicit Plugins(const config::Config& config) : _config{config} {}

    void init();
    void close();
    [[nodiscard]] const std::vector<client_server::PluginData>& plugin_list() const {
        return _plugins;
    }

    [[nodiscard]] std::pair<size_t, boost::optional<const client_server::PluginData&>> find_by_name(const std::string_view name) const {
        return find_by_name(std::string(name));
    }
    [[nodiscard]] std::pair<size_t, boost::optional<const client_server::PluginData&>> find_by_name(const char* name) const {
        return find_by_name(std::string(name));
    }
    [[nodiscard]] std::pair<size_t, boost::optional<const client_server::PluginData&>> find_by_name(std::string name) const;
    [[nodiscard]] boost::optional<const client_server::PluginData&> find_by_id(size_t id) const;

#if UDA_TEST
    void add_plugin(client_server::PluginData&& plugin) { _plugins.emplace_back(std::move(plugin)); }
#endif

  private:
    const config::Config& _config;
    std::vector<client_server::PluginData> _plugins;

    bool _initialised = false;

    void discover_plugins();
    void discover_plugins_in_directory(const std::filesystem::path& directory);
    void load_plugin(const std::filesystem::path& directory);
};

} // namespace uda::server
