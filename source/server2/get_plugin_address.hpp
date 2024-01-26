#pragma once

#include "udaPlugin.h"

typedef int (*PLUGINFUNP)(UDA_PLUGIN_INTERFACE*); // Plugin function type

namespace uda {

int get_plugin_address(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* pluginfunp);

}
