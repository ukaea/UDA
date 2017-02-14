#ifndef IDAM_GETPLUGIN_ADDRESS_H
#define IDAM_GETPLUGIN_ADDRESS_H

#include <plugins/idamPlugin.h>

int getPluginAddress(void **pluginHandle, char *library, char *symbol, PLUGINFUNP *idamPlugin);

#endif // IDAM_GETPLUGIN_ADDRESS_H
