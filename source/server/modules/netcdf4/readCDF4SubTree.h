//
// Created by jholloc on 09/05/16.
//

#ifndef IDAM_READCDF4SUBTREE_H
#define IDAM_READCDF4SUBTREE_H

#include "readCDF4.h"

void initVariable(VARIABLE * variable);

void initAttribute(ATTRIBUTE * attribute);

void initGroup(GROUP * group);

void initHGroup(HGROUPS * hgroups);

int addHGroup(HGROUPS * hgroups, GROUP group);

GROUP * findHGroup(HGROUPS * hgroups, int grpid);

void freeGroup(GROUP * group);

void freeHGroups(HGROUPS * hgroups);

char * getUniqueTypeName(char * proposed, int ref_id, USERDEFINEDTYPELIST * userdefinedtypelist);

void updateUdt(HGROUPS * hgroups, USERDEFINEDTYPELIST * userdefinedtypelist);

void repeatUdt(USERDEFINEDTYPELIST * userdefinedtypelist);

int findUserDefinedTypeIndexReverse(int ref_id);

int getCDF4SubTreeUserDefinedTypes(int grpid, GROUPLIST * grouplist, USERDEFINEDTYPELIST * userdefinedtypelist);

void replaceSubTreeEmbeddedStrings(USERDEFINEDTYPE * udt, int ndata, char * dvec);

int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE * variable, USERDEFINEDTYPE * udt,
                          USERDEFINEDTYPELIST * userdefinedtypelist);

int getCDF4SubTreeVar2Meta(int grpid, int varid, VARIABLE * variable, int * offset, COMPOUNDFIELD * field,
                           USERDEFINEDTYPELIST * userdefinedtypelist);

int getCDF4SubTreeMeta(int grpid, int parent, USERDEFINEDTYPE * udt, USERDEFINEDTYPELIST * userdefinedtypelist,
                       HGROUPS * hgroups);

char * dimShapeLabel(int grpid, int rank, int * dimids, int * count, int ** shp);

ENUMLIST * getCDF4EnumList(int grpid, nc_type vartype);

int getCDF4SubTreeVarData(int grpid, void ** data, VARIABLE * variable);

int getCDF4SubTreeVar2Data(int grpid, void ** data, VARIABLE * variable, COMPOUNDFIELD * field);

int readCDF4SubTreeVar3Data(GROUPLIST grouplist, int varid, int rank, int * dimids, int ** shape,
                            int * ndvec, int * data_type, char ** data, USERDEFINEDTYPE ** udt);

int getCDF4SubTreeData(void ** data, GROUP * group, HGROUPS * hgroups);

#endif //IDAM_READCDF4SUBTREE_H
