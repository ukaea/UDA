#ifndef UDA_GETPLUGIN_ADDRESS_H
#define UDA_GETPLUGIN_ADDRESS_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *pluginfunp);

#ifdef __cplusplus
}
#endif

#endif // UDA_GETPLUGIN_ADDRESS_H
