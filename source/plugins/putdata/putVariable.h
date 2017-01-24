#ifndef IDAM_PUTVARIABLE_H
#define IDAM_PUTVARIABLE_H

#include <idamplugin.h>
#include <netcdf.h>

int do_variable(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

nc_type swapType(int type, int ctype, int dctype);

#endif //IDAM_PUTVARIABLE_H