// MongoDB Query Plugin

#ifndef IDAM_PLUGINS_MONGODB_MONGODB_H
#define IDAM_PLUGINS_MONGODB_MONGODB_H

// Change History:
//
// 18Jan2016	dgm	Original Version

#include <idamplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

int query(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_MONGODB_MONGODB_H
