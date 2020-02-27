#ifndef UDA_GETPLUGIN_ADDRESS_H
#define UDA_GETPLUGIN_ADDRESS_H

#include <plugins/udaPlugin.h>

int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *idamPlugin);

#endif // UDA_GETPLUGIN_ADDRESS_H
