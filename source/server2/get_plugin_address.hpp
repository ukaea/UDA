#pragma once

#include "plugins.hpp"

namespace uda::server
{

class Config;

int get_plugin_address(const Config& config, void** pluginHandle, const char* library, const char* symbol,
                       uda::plugins::PLUGINFUNP* pluginfunp);

} // namespace uda::server
