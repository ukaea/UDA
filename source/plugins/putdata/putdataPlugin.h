#ifndef IDAM_PUTDATAPLUGIN_H
#define IDAM_PUTDATAPLUGIN_H

#include <idamplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

int putdataPlugin(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif //IDAM_PUTDATAPLUGIN_H
