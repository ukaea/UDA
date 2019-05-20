#ifndef UDA_READCDF4SUBTREE_H
#define UDA_READCDF4SUBTREE_H

#include "readCDF4.h"

void initVariable(VARIABLE* variable);

void initAttribute(ATTRIBUTE* attribute);

void initGroup(GROUP* group);

int addHGroup(HGROUPS* hgroups, GROUP group);

GROUP* findHGroup(HGROUPS* hgroups, int grpid);

void freeGroup(GROUP* group);

void freeHGroups(HGROUPS* hgroups);

char*
getUniqueTypeName(char* proposed, int ref_id, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

void updateUdt(HGROUPS* hgroups, USERDEFINEDTYPELIST* userdefinedtypelist);

void repeatUdt(USERDEFINEDTYPELIST* userdefinedtypelist);

int findUserDefinedTypeIndexReverse(USERDEFINEDTYPELIST* userdefinedtypelist, int ref_id);

void replaceSubTreeEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   USERDEFINEDTYPE* udt, int ndata, char* dvec);

int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE* variable, USERDEFINEDTYPE* udt,
                          LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

int getCDF4SubTreeVar2Meta(int grpid, int varid, VARIABLE* variable, int* offset, COMPOUNDFIELD* field,
                           USERDEFINEDTYPELIST* userdefinedtypelist);

char* dimShapeLabel(int grpid, int rank, int* dimids, int* count, int** shp);

ENUMLIST* getCDF4EnumList(int grpid, nc_type vartype, LOGMALLOCLIST* logmalloclist);

int getCDF4SubTreeVarData(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist);

int getCDF4SubTreeVar2Data(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist, COMPOUNDFIELD* field);

int readCDF4SubTreeVar3Data(GROUPLIST grouplist, int varid, int rank, int* dimids, int** shape,
                            int* ndvec, int* data_type, char** data, LOGMALLOCLIST* logmalloclist,
                            USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt);

#endif // UDA_READCDF4SUBTREE_H
