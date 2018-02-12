// Created by lkogan on 09/02/18.
//

#ifndef UDA_IPXPLUGIN_H
#define UDA_IPXPLUGIN_H

#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int udaIpx(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
int do_ipx_read(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif //UDA_IPXPLUGIN_H
