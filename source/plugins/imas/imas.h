#ifndef UDA_PLUGINS_IMAS_PLUGIN_IMAS_H
#define UDA_PLUGINS_IMAS_PLUGIN_IMAS_H

#include <plugins/pluginStructs.h>

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1        // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD           "help"

#define PUT_OPERATION               0
#define ISTIMED_OPERATION           1
#define PUTSLICE_OPERATION          2
#define REPLACELASTSLICE_OPERATION  3

#define GET_OPERATION               0
#define GETSLICE_OPERATION          1
#define GETDIMENSION_OPERATION      2

#define UNKNOWN_TYPE    0
#define STRING          1        // Must be the same as the client for consistency
#define INT             2
#define FLOAT           3
#define DOUBLE          4
#define STRING_VECTOR   5

const char* getImasIdsVersion();

void putImasIdsDevice(const char* device);

const char* getImasIdsDevice();

int findIMASType(const char* typeName);

int findIMASIDAMType(int type);

int imas(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

void putImasIdsVersion(const char* version);

void setSliceIdx(int idx1, int idx2);

int getSliceIdx1();

int getSliceIdx2();

void setSliceTime(double time1, double time2);

double getSliceTime1();

double getSliceTime2();

#endif // UDA_PLUGINS_IMAS_PLUGIN_IMAS_H
