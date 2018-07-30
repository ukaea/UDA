#ifndef UDA_PLUGINS_NETCDF_READCDF4_H
#define UDA_PLUGINS_NETCDF_READCDF4_H

#include <netcdf.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#include "readCDFStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int IMAS_HDF_READER;

extern CDFSUBSET cdfsubset;

extern nc_type ctype;
extern nc_type dctype;

int readCDF(const char* path, char* signal_name, const char* signal_alias, DATA_BLOCK* data_block,
            const char* subset, const DATASUBSET* data_subset,
            LOGMALLOCLIST** logmalloclist, USERDEFINEDTYPELIST** userdefinedtypelist);

int readCDFGlobalMeta(const char* path, DATA_BLOCK* data_block,
                    LOGMALLOCLIST** logmalloclist, USERDEFINEDTYPELIST** userdefinedtypelist);

unsigned int readCDF4Properties();

int getGroupId(int ncgrpid, char* target, int* targetid);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_NETCDF_READCDF4_H
