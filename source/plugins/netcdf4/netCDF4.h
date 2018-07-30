#ifndef UDA_PLUGINS_NETCDF4_NETCDF4_H
#define UDA_PLUGINS_NETCDF4_NETCDF4_H

#ifdef __cplusplus
extern "C" {
#endif

#include <plugins/udaPlugin.h>
#include <plugins/udaPluginFiles.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "get"

#define NC_IGNOREHIDDENATTS     1
#define NC_IGNOREHIDDENVARS     2
#define NC_IGNOREHIDDENGROUPS   4
#define NC_IGNOREHIDDENDIMS     8
#define NC_NOTPOINTERTYPE       16        // Returned data structure members are pointers by default to avoid structure size variations within arrays
#define NC_NODIMENSIONDATA      32
#define NC_USESTRUCTUREDEF      128        // Use the local structure definition as default

#define NC_HIDDENPREFIX    '_'        // This should be overridden by an environment variable: IDAM_HIDDENPREFIX or passed as a keyword pair

int netCDF4(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

extern IDAMPLUGINFILELIST pluginFileList;

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_NETCDF4_NETCDF4_H