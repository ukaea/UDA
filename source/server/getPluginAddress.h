#pragma once

#include "serverPlugin.h"

namespace uda::server
{

int getPluginAddress(void** pluginHandle, const char* library, const char* symbol, UDA_PLUGIN_ENTRY_FUNC* pluginfunp);

}
