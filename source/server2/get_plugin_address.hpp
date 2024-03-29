#pragma once

#ifndef UDA_GETPLUGIN_ADDRESS_HPP
#define UDA_GETPLUGIN_ADDRESS_HPP

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

namespace uda {

int get_plugin_address(void** pluginHandle, const char* library, const char* symbol, PLUGINFUNP* pluginfunp);

}

#endif // UDA_GETPLUGIN_ADDRESS_HPP
