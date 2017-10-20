#ifndef IDAM_PLUGINS_IDAMSERVERHELP_H
#define IDAM_PLUGINS_IDAMSERVERHELP_H

#include <server/pluginStructs.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "help"

int idamServerHelp(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#endif // IDAM_PLUGINS_IDAMSERVERHELP_H
