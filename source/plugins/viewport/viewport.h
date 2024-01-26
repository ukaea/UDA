#ifndef IDAM_PLUGINS_VIEWPORT_VIEWPORT_H
#define IDAM_PLUGINS_VIEWPORT_VIEWPORT_H

#include "export.h"
#include "udaPlugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION 1
#define THISPLUGIN_MAX_INTERFACE_VERSION 1 // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD "get"

#define MAXHANDLES 8
#define MAXSIGNALNAME 256
#define FREEHANDLEBLOCK 4

int viewport(UDA_PLUGIN_INTERFACE* plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_VIEWPORT_VIEWPORT_H
