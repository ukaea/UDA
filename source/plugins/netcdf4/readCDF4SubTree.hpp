#ifndef UDA_PLUGIN_READCDF4SUBTREE_H
#define UDA_PLUGIN_READCDF4SUBTREE_H

#include "readCDF4.hpp"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

LIBRARY_API void freeHGroups(HGROUPS* hgroups);

LIBRARY_API void updateUdt(HGROUPS* hgroups, USERDEFINEDTYPELIST* userdefinedtypelist);


#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void initVariable(VARIABLE* variable);

LIBRARY_API void initAttribute(ATTRIBUTE* attribute);

LIBRARY_API void initGroup(HGROUP* group);

LIBRARY_API int addHGroup(HGROUPS* hgroups, HGROUP group);

LIBRARY_API HGROUP* findHGroup(HGROUPS* hgroups, int grpid);

LIBRARY_API void freeGroup(HGROUP* group);

LIBRARY_API char*
getUniqueTypeName(char* proposed, int ref_id, LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API void repeatUdt(USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API int findUserDefinedTypeIndexReverse(USERDEFINEDTYPELIST* userdefinedtypelist, int ref_id);

LIBRARY_API void replaceSubTreeEmbeddedStrings(LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist,
                                   USERDEFINEDTYPE* udt, int ndata, char* dvec);

LIBRARY_API int getCDF4SubTreeVarMeta(int grpid, int varid, VARIABLE* variable, USERDEFINEDTYPE* udt,
                          LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API int getCDF4SubTreeVar2Meta(int grpid, int varid, VARIABLE* variable, int* offset, COMPOUNDFIELD* field,
                           USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API char* dimShapeLabel(int grpid, int rank, int* dimids, int* count, int** shp);

LIBRARY_API ENUMLIST* getCDF4EnumList(int grpid, nc_type vartype, LOGMALLOCLIST* logmalloclist);

LIBRARY_API int getCDF4SubTreeVarData(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API int getCDF4SubTreeVar2Data(int grpid, void** data, VARIABLE* variable, LOGMALLOCLIST* logmalloclist,
                           USERDEFINEDTYPELIST* userdefinedtypelist, COMPOUNDFIELD* field);

LIBRARY_API int readCDF4SubTreeVar3Data(GROUPLIST grouplist, int varid, int rank, int* dimids, int** shape,
                            int* ndvec, int* data_type, char** data, LOGMALLOCLIST* logmalloclist,
                            USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE** udt);

#ifdef __cplusplus
}
#endif

#endif // UDA_PLUGIN_READCDF4SUBTREE_H
