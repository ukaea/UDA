#ifndef IDAM_PLUGINS_PUTDATA_PUTVARIABLE_H
#define IDAM_PLUGINS_PUTDATA_PUTVARIABLE_H

#include <netcdf.h>
#include <plugins/idamPlugin.h>

int do_variable(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

nc_type swapType(int type, int ctype, int dctype);

#endif // IDAM_PLUGINS_PUTDATA_PUTVARIABLE_H