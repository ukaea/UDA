#ifndef UDA_PLUGINS_LIBS3_H
#define UDA_PLUGINS_LIBS3_H

#ifdef __cplusplus
extern "C" {
#endif

#include <plugins/udaPlugin.h>
#include <server/makeServerRequestBlock.h>
#include <server/serverPlugin.h>
#include <client/udaClient.h>
#include <clientserver/compressDim.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "get"

int s3(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_LIBS3_H
