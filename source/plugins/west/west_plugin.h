#ifndef IDAM_PLUGINS_WEST_WEST_PLUGIN_H
#define IDAM_PLUGINS_WEST_WEST_PLUGIN_H

#include <plugins/pluginStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int westPlugin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_WEST_WEST_PLUGIN_H
