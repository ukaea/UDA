#ifndef UDA_GETPLUGIN_ADDRESS_H
#define UDA_GETPLUGIN_ADDRESS_H

#include <plugins/udaPlugin.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int getPluginAddress(void **pluginHandle, const char *library, const char *symbol, PLUGINFUNP *idamPlugin);

#ifdef __cplusplus
}
#endif

#endif // UDA_GETPLUGIN_ADDRESS_H
