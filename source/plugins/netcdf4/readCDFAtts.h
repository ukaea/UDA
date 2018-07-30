#ifndef UDA_PLUGINS_NETCDF_READCDFATTS_H
#define UDA_PLUGINS_NETCDF_READCDFATTS_H

#ifdef __cplusplus
extern "C" {
#endif

int readCDF4Atts(int grpid, int varid, char* units, char* title, char* class, char* comment);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGINS_NETCDF_READCDFATTS_H
