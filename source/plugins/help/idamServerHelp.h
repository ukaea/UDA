#ifndef UDA_PLUGIN_IDAMSERVERHELP_H
#define UDA_PLUGIN_IDAMSERVERHELP_H

#include <plugins/pluginStructs.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "help"

int idamServerHelp(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#endif // UDA_PLUGIN_IDAMSERVERHELP_H
