#pragma once

#include "plugins.hpp"

namespace uda::server
{

class Config;

int get_plugin_address(const Config& config, void** plugin_handle, const char* library, const char* symbol,
                       uda::plugins::PLUGINFUNP* plugin_fun);

} // namespace uda::server
