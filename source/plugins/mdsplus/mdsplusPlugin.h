#pragma once

#ifndef UDA_PLUGIN_MDSPLUSPLUGIN_H
#define UDA_PLUGIN_MDSPLUSPLUGIN_H

#include <plugins/udaPlugin.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

LIBRARY_API int mdsplusPlugin(IDAM_PLUGIN_INTERFACE * plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_MDSPLUSPLUGIN_H
