#pragma once

#include "plugins.hpp"

namespace uda {

int get_plugin_address(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* pluginfunp);

}
