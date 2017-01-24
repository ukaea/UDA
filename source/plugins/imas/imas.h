#ifndef IdamIMASPluginInclude
#define IdamIMASPluginInclude

// Change History:
//
// 22Apr2015	dgm	Original Version
// 20Aug2015	dgm	Added imas_mds for the legacy mdsplus components

#include <idamplugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

#define FILEISNEW       1
#define FILEISOPEN      2
#define FILEISCLOSED    3

#define PUT_OPERATION               0
#define ISTIMED_OPERATION           1
#define PUTSLICE_OPERATION          2
#define REPLACELASTSLICE_OPERATION  3


#define GET_OPERATION           0
#define GETSLICE_OPERATION      1
#define GETDIMENSION_OPERATION  2

#define CLOSEST_SAMPLE  1
#define PREVIOUS_SAMPLE 2
#define INTERPOLATION   3

#define TIMEBASEPATHLENGTH 256

int imas(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

int imas_mds(IDAM_PLUGIN_INTERFACE * idam_plugin_interface);

char * getImasIdsVersion();

void putImasIdsVersion(char * version);

void putImasIdsDevice(char * device);

char * getImasIdsDevice();

#ifdef __cplusplus
}
#endif

#endif
