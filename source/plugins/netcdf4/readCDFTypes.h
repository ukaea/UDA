#ifndef UDA_PLUGINS_NETCDF4_READCDFTYPES_H
#define UDA_PLUGINS_NETCDF4_READCDFTYPES_H

#include <stdio.h>
#include <netcdf.h>

#include <structures/genStructs.h>

int readCDFTypes(int grpid, USERDEFINEDTYPELIST* userdefinedtypelist);

int convertNCType(nc_type type);

void printNCType(FILE* fd, nc_type type);

#endif // UDA_PLUGINS_NETCDF4_READCDFTYPES_H
