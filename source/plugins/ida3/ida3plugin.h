#ifndef UDA_PLUGINS_IDA3_IDA3PLUGIN_H
#define UDA_PLUGINS_IDA3_IDA3PLUGIN_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "help"

int ida3plugin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_IDA3_IDA3PLUGIN_H
