#pragma once

#include "plugins.hpp"

namespace uda::config {
class Config;
}

namespace uda::server
{

int get_plugin_address(const config::Config& config, void** plugin_handle, const char* library, const char* symbol,
                       uda::plugins::PLUGINFUNP* plugin_fun);

} // namespace uda::server
