#pragma once

#ifndef UDA_SERVER_PLUGINS_HPP
#define UDA_SERVER_PLUGINS_HPP

#include <vector>
#include <boost/optional.hpp>

#include "pluginStructs.h"

namespace uda {

class Plugins {
public:
    void init();

    void close();

    [[nodiscard]] PluginList as_plugin_list() const;

    [[nodiscard]] boost::optional<const PluginData&> find_by_format(const char* format) const;
    [[nodiscard]] boost::optional<const PluginData&> find_by_request(int request) const;

private:
    std::vector<PluginData> plugins_;

    void init_serverside_functions();

    void init_generic_plugin();

    bool initialised_ = false;

    void process_config_file(std::ifstream& conf_file);
};

}

#endif // UDA_SERVER_PLUGINS_HPP