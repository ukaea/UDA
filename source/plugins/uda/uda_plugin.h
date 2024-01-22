#ifndef UDA_PLUGIN_UDA_PLUGIN_H
#define UDA_PLUGIN_UDA_PLUGIN_H

#include "pluginStructs.h"
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int UDAPlugin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_UDA_PLUGIN_H
