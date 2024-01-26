#ifndef UDA_CLIENT_ACCAPI_H
#define UDA_CLIENT_ACCAPI_H

#include <stdbool.h>
#include <stdio.h>

#include "client.h"
#include "export.h"
#include "genStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API void setIdamPrivateFlag(unsigned int flag);

LIBRARY_API void resetIdamPrivateFlag(unsigned int flag);

LIBRARY_API void setIdamClientFlag(unsigned int flag);

LIBRARY_API void resetIdamClientFlag(unsigned int flag);

LIBRARY_API void setIdamProperty(const char* property);

LIBRARY_API int getIdamProperty(const char* property);

LIBRARY_API void resetIdamProperty(const char* property);

LIBRARY_API void resetIdamProperties();

#ifndef __APPLE__

LIBRARY_API int getIdamMemoryFree();

LIBRARY_API int getIdamMemoryUsed();

#endif

LIBRARY_API void putIdamErrorModel(int handle, int model, int param_n, const float* params);

LIBRARY_API void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, const float* params);

LIBRARY_API void putIdamServer(const char* host, int port);

LIBRARY_API void putIdamServerHost(const char* host);

LIBRARY_API void putIdamServerPort(int port);

LIBRARY_API void putIdamServerSocket(int socket);

LIBRARY_API void getIdamServer(const char** host, int* port, int* socket);

LIBRARY_API int getIdamErrorCode(int handle);

LIBRARY_API const char* getIdamErrorMsg(int handle);

LIBRARY_API int getIdamSourceStatus(int handle);

LIBRARY_API int getIdamSignalStatus(int handle);

LIBRARY_API int getIdamDataStatus(int handle);

LIBRARY_API int getIdamDataNum(int handle);

LIBRARY_API int getIdamRank(int handle);

LIBRARY_API int getIdamOrder(int handle);

LIBRARY_API unsigned int getIdamCachePermission(int handle);

LIBRARY_API unsigned int getIdamTotalDataBlockSize(int handle);

LIBRARY_API int getIdamDataType(int handle);

LIBRARY_API int getIdamDataOpaqueType(int handle);

LIBRARY_API void* getIdamDataOpaqueBlock(int handle);

LIBRARY_API int getIdamDataOpaqueCount(int handle);

LIBRARY_API void getIdamErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int getIdamErrorType(int handle);

LIBRARY_API int getIdamDataTypeId(const char* type);

LIBRARY_API int getIdamDataTypeSize(int type);

LIBRARY_API void getIdamErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int getIdamErrorAsymmetry(int handle);

LIBRARY_API int getIdamErrorModelId(const char* model);

LIBRARY_API char* acc_getSyntheticData(int handle);

LIBRARY_API char* acc_getSyntheticDimData(int handle, int ndim);

LIBRARY_API void acc_setSyntheticData(int handle, char* data);

LIBRARY_API void acc_setSyntheticDimData(int handle, int ndim, char* data);

LIBRARY_API char* getIdamSyntheticData(int handle);

LIBRARY_API char* getIdamData(int handle);

LIBRARY_API void getIdamDataTdi(int handle, char* data);

LIBRARY_API char* getIdamAsymmetricError(int handle, int above);

LIBRARY_API char* getIdamDataErrLo(int handle);

LIBRARY_API char* getIdamDataErrHi(int handle);

LIBRARY_API int getIdamDataErrAsymmetry(int handle);

LIBRARY_API void acc_setIdamDataErrAsymmetry(int handle, int asymmetry);

LIBRARY_API void acc_setIdamDataErrType(int handle, int type);

LIBRARY_API void acc_setIdamDataErrLo(int handle, char* errlo);

LIBRARY_API char* getIdamDimErrLo(int handle, int ndim);

LIBRARY_API char* getIdamDimErrHi(int handle, int ndim);

LIBRARY_API int getIdamDimErrAsymmetry(int handle, int ndim);

LIBRARY_API void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry);

LIBRARY_API void acc_setIdamDimErrType(int handle, int ndim, int type);

LIBRARY_API void acc_setIdamDimErrLo(int handle, int ndim, char* errlo);

LIBRARY_API char* getIdamError(int handle);

LIBRARY_API void getIdamDoubleData(int handle, double* fp);

LIBRARY_API void getIdamFloatData(int handle, float* fp);

LIBRARY_API void getIdamGenericData(int handle, void* data);

LIBRARY_API void getIdamFloatAsymmetricError(int handle, int above, float* fp);

LIBRARY_API void getIdamFloatError(int handle, float* fp);

LIBRARY_API const char* getIdamDataLabel(int handle);

LIBRARY_API void getIdamDataLabelTdi(int handle, char* label);

LIBRARY_API const char* getIdamDataUnits(int handle);

LIBRARY_API void getIdamDataUnitsTdi(int handle, char* units);

LIBRARY_API const char* getIdamDataDesc(int handle);

LIBRARY_API void getIdamDataDescTdi(int handle, char* desc);

LIBRARY_API int getIdamDimNum(int handle, int ndim);

LIBRARY_API int getIdamDimType(int handle, int ndim);

LIBRARY_API int getIdamDimErrorType(int handle, int ndim);

LIBRARY_API int getIdamDimErrorAsymmetry(int handle, int ndim);

LIBRARY_API void getIdamDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params);

LIBRARY_API char* getIdamSyntheticDimData(int handle, int ndim);

LIBRARY_API char* getIdamDimData(int handle, int ndim);

LIBRARY_API const char* getIdamDimLabel(int handle, int ndim);

LIBRARY_API const char* getIdamDimUnits(int handle, int ndim);

LIBRARY_API void getIdamDimLabelTdi(int handle, int ndim, char* label);

LIBRARY_API void getIdamDimUnitsTdi(int handle, int ndim, char* units);

LIBRARY_API void getIdamDoubleDimData(int handle, int ndim, double* fp);

LIBRARY_API void getIdamFloatDimData(int handle, int ndim, float* fp);

LIBRARY_API void getIdamGenericDimData(int handle, int ndim, void* data);

LIBRARY_API char* getIdamDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char* getIdamDimError(int handle, int ndim);

LIBRARY_API void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

LIBRARY_API void getIdamFloatDimError(int handle, int ndim, float* fp);

LIBRARY_API int idamDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int getIdamDataCheckSum(int handle);

LIBRARY_API int getIdamDimDataCheckSum(int handle, int ndim);

LIBRARY_API int getIdamThreadLastHandle();

LIBRARY_API void putIdamThreadLastHandle(int handle);

LIBRARY_API int getIdamMaxThreadCount();

LIBRARY_API int setIdamDataTree(int handle);

// Return a specific data tree

typedef struct NTree NTREE;

LIBRARY_API NTREE* getIdamDataTree(int handle);

// Return a user defined data structure definition

LIBRARY_API NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_ACCAPI_H
