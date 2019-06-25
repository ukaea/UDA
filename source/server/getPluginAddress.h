#ifndef UDA_GETPLUGIN_ADDRESS_H
#define UDA_GETPLUGIN_ADDRESS_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *idamPlugin);

#ifdef __cplusplus
}
#endif

#endif // UDA_GETPLUGIN_ADDRESS_H
