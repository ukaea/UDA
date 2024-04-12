#ifndef UDA_PLUGIN_CAPNP_NETCDF_H
#define UDA_PLUGIN_CAPNP_NETCDF_H

#include "plugins/udaPlugin.h"
#include "plugins/udaPluginFiles.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD           "get"

#define NC_IGNOREHIDDENATTS     1
#define NC_IGNOREHIDDENVARS     2
#define NC_IGNOREHIDDENGROUPS   4
#define NC_IGNOREHIDDENDIMS     8
#define NC_NOTPOINTERTYPE       16        // Returned data structure members are pointers by default to avoid structure size variations within arrays
#define NC_NODIMENSIONDATA      32
//#define NC_ROOTSTRUCTURE	    64		// If the root structure contains a single data structure then return this only
#define NC_USESTRUCTUREDEF      128        // Use the local structure definition as default


#define NC_HIDDENPREFIX    '_'        // This should be overridden by an environment variable: IDAM_HIDDENPREFIX or passed as a keyword pair
#define DEFAULT_ARCHIVE_DIR     "/net/fuslsa/data/MAST_Data"

int capnp_netcdf(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_CAPNP_NETCDF_H
