#ifndef UDA_PLUGINS_NETCDF4_READCDFVAR_H
#define UDA_PLUGINS_NETCDF4_READCDFVAR_H

#include <netcdf.h>

#include <structures/genStructs.h>

#include "readCDFStructs.h"

void readCDF4CreateIndex(int ndata, void* dvec);

int readCDF4Var(GROUPLIST grouplist, int varid, int isCoordinate, int rank, int* dimids,
                unsigned int* extent, int* ndvec, int* data_type, int* isIndex, char** data,
                LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt);

int readCDF4AVar(GROUPLIST grouplist, int grpid, int varid, nc_type atttype, char* name, int* ndvec, int ndims[2],
                 int* data_type, char** data, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                 USERDEFINEDTYPE** udt);

int applyCDFCalibration(int grpid, int varid, int ndata, int* type, char** data);

int readCDF4Err(int grpid, int varid, int isCoordinate, int class, int rank, int* dimids, int* nevec,
                int* error_type, char** edata, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

int readCDFCheckCoordinate(int grpid, int varid, int rank, int ncoords, char* coords, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist);

int isAtomicNCType(nc_type type);

int scopedUserDefinedTypes(int grpid);

void replaceStrings(char** svec, int* ndata, char** dvec, int* ndims);

void replaceEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                            USERDEFINEDTYPE* udt, int ndata, char* dvec);

int idamAtomicType(nc_type type);

#endif // UDA_PLUGINS_NETCDF4_READCDFVAR_H
