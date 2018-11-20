#ifndef CCFE_PLUGINS_IPX_PLUGIN_H
#define CCFE_PLUGINS_IPX_PLUGIN_H

#include <plugins/pluginStructs.h>

#include "ipx.h"

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

#ifdef __cplusplus
extern "C" {
#endif

int ipxPlugin(IDAM_PLUGIN_INTERFACE *idam_plugin_interface);

#ifdef __cplusplus
}
#endif

namespace uda {
namespace plugins {
namespace ipx {

class IPXPlugin {
public:
    void init() {}
    void reset() {}
    int help(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int build_date(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int default_method(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int max_interface_version(IDAM_PLUGIN_INTERFACE* plugin_interface);
    int read(IDAM_PLUGIN_INTERFACE* plugin_interface);
private:
    Cipx ipx_;
};

}
}
}

#endif //CCFE_PLUGINS_IPX_PLUGIN_H
