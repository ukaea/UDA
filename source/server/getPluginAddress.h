#ifndef IDAM_GETPLUGIN_ADDRESS_H
#define IDAM_GETPLUGIN_ADDRESS_H

#include <plugins/udaPlugin.h>

int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *idamPlugin);

#endif // IDAM_GETPLUGIN_ADDRESS_H
