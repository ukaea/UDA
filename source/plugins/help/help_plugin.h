#ifndef UDA_PLUGIN_IDAMSERVERHELP_H
#define UDA_PLUGIN_IDAMSERVERHELP_H

#include <plugins/pluginStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "help"

LIBRARY_API int helpPlugin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_IDAMSERVERHELP_H
