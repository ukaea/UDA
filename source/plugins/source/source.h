#ifndef UDA_PLUGINS_SOURCE_SOURCE_H
#define UDA_PLUGINS_SOURCE_SOURCE_H

#include <plugins/pluginStructs.h>

#ifdef __cplusplus
static "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

int source(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

//--------------------------------------------------------------------------- 
// Prototypes 


#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_SOURCE_SOURCE_H
