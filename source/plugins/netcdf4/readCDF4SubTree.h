#ifndef IDAM_NETCDFPLUGIN_READCDF4SUBTREE_H
#define IDAM_NETCDFPLUGIN_READCDF4SUBTREE_H

#include <netcdf.h>

#include "readCDF4.h"

int idamAtomicType(nc_type type);
void initVariable(VARIABLE* variable);
void initAttribute(ATTRIBUTE* attribute);
void initGroup(GROUP* group);
void initHGroup(HGROUPS* hgroups);
int addHGroup(HGROUPS* hgroups, GROUP group);
GROUP* findHGroup(HGROUPS* hgroups, int grpid);
void freeGroup(GROUP* group);
void freeHGroups(HGROUPS* hgroups);
char*
getUniqueTypeName(char* proposed, int ref_id, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);
void updateUdt(HGROUPS* hgroups, USERDEFINEDTYPELIST* userdefinedtypelist);
int findUserDefinedTypeIndexReverse(USERDEFINEDTYPELIST* userdefinedtypelist, int ref_id);
int getCDF4SubTreeUserDefinedTypes(int grpid, GROUPLIST* grouplist, USERDEFINEDTYPELIST* userdefinedtypelist);
void replaceSubTreeEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   USERDEFINEDTYPE* udt, int ndata, char* dvec);
int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE* variable, USERDEFINEDTYPE* udt, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist);
int getCDF4SubTreeMeta(int grpid, int parent, USERDEFINEDTYPE* udt, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, HGROUPS* hgroups);
char* dimShapeLabel(int grpid, int rank, int* dimids, int* count, int** shp);
ENUMLIST* getCDF4EnumList(LOGMALLOCLIST* logmalloclist, int grpid, nc_type vartype);
int getCDF4SubTreeVarData(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, int grpid, void** data, VARIABLE* variable);
int getCDF4SubTreeData(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, void** data, GROUP* group, HGROUPS* hgroups);

#endif // IDAM_NETCDFPLUGIN_READCDF4SUBTREE_H
