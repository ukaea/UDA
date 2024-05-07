#pragma once

#include <boost/optional.hpp>
#include <vector>

#include "clientserver/udaDefines.h"
#include "clientserver/udaStructs.h"
#include "plugins/udaPlugin.h"

namespace uda::config {
class Config;
}

namespace uda::server
{

class Plugins
{
  public:
    Plugins(const config::Config& config) : _config{config} {}

    void init();

    void close();

    [[nodiscard]] uda::plugins::PluginList as_plugin_list() const;

    [[nodiscard]] boost::optional<const uda::plugins::PluginData&> find_by_format(const char* format) const;
    [[nodiscard]] boost::optional<const uda::plugins::PluginData&> find_by_request(int request) const;

  private:
    const config::Config& _config;
    std::vector<uda::plugins::PluginData> _plugins;

    void init_serverside_functions();

    void init_generic_plugin();

    bool _initialised = false;

    void process_config_file(std::ifstream& conf_file);
};

} // namespace uda::server
