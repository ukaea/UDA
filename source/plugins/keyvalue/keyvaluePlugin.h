#ifndef UDA_PLUGIN_KEYVALUEPLUGIN_H
#define UDA_PLUGIN_KEYVALUEPLUGIN_H

#include <uda/plugins.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION 1
#define THISPLUGIN_MAX_INTERFACE_VERSION 1 // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD "help"

int keyValue(UDA_PLUGIN_INTERFACE* plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_KEYVALUEPLUGIN_H
