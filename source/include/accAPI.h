#ifndef UDA_CLIENT_ACCAPI_H
#define UDA_CLIENT_ACCAPI_H

#include <stdbool.h>
#include <stdio.h>

#include "client.h"
#include "export.h"
#include "genStructs.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API DATA_BLOCK* udaGetCurrentDataBlock();

LIBRARY_API int udaGetCurrentDataBlockIndex();

LIBRARY_API int udaGrowDataBlocks();

LIBRARY_API int udaGetNewDataHandle();

LIBRARY_API void udaFreeDataBlocks();

LIBRARY_API void udaSetPrivateFlag(unsigned int flag);

LIBRARY_API void udaResetPrivateFlag(unsigned int flag);

LIBRARY_API void udaSetClientFlag(unsigned int flag);

LIBRARY_API void udaResetClientFlag(unsigned int flag);

LIBRARY_API void udaSetProperty(const char* property);

LIBRARY_API int udaGetProperty(const char* property);

LIBRARY_API void udaResetProperty(const char* property);

LIBRARY_API void udaResetProperties();

LIBRARY_API CLIENT_BLOCK udaSaveProperties();

LIBRARY_API void udaRestoreProperties(CLIENT_BLOCK cb);

LIBRARY_API CLIENT_BLOCK* udaGetProperties(int handle);

LIBRARY_API CLIENT_BLOCK* udaGetDataProperties(int handle);

#ifndef __APPLE__

LIBRARY_API int udaGetMemoryFree();

LIBRARY_API int udaGetMemoryUsed();

#endif

LIBRARY_API void udaPutErrorModel(int handle, int model, int param_n, const float* params);

LIBRARY_API void udaPutDimErrorModel(int handle, int ndim, int model, int param_n, const float* params);

LIBRARY_API void udaPutServer(const char* host, int port);

LIBRARY_API void udaPutServerHost(const char* host);

LIBRARY_API void udaPutServerPort(int port);

LIBRARY_API void udaPutServerSocket(int socket);

LIBRARY_API void udaGetServer(const char** host, int* port, int* socket);

LIBRARY_API UDA_ERROR_STACK* udaGetServerErrorStack();

LIBRARY_API int udaGetErrorCode(int handle);

LIBRARY_API const char* udaGetErrorMsg(int handle);

LIBRARY_API int udaGetSourceStatus(int handle);

LIBRARY_API int udaGetSignalStatus(int handle);

LIBRARY_API int udaGetDataStatus(int handle);

LIBRARY_API int udaGetLastHandle();

LIBRARY_API int udaGetDataNum(int handle);

LIBRARY_API int udaGetRank(int handle);

LIBRARY_API int udaGetOrder(int handle);

LIBRARY_API unsigned int udaGetCachePermission(int handle);

LIBRARY_API unsigned int udaGetTotalDataBlockSize(int handle);

LIBRARY_API int udaGetDataType(int handle);

LIBRARY_API int udaGetDataOpaqueType(int handle);

LIBRARY_API void* udaGetDataOpaqueBlock(int handle);

LIBRARY_API int udaGetDataOpaqueCount(int handle);

LIBRARY_API void udaGetErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int udaGetErrorType(int handle);

LIBRARY_API int udaGetDataTypeId(const char* type);

LIBRARY_API int udaGetDataTypeSize(int type);

LIBRARY_API void udaGetErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int udaGetErrorAsymmetry(int handle);

LIBRARY_API int udaGetErrorModelId(const char* model);

LIBRARY_API char* udaGetSyntheticData(int handle);

LIBRARY_API char* udaGetSyntheticDimData(int handle, int ndim);

LIBRARY_API void udaSetSyntheticData(int handle, char* data);

LIBRARY_API void udaSetSyntheticDimData(int handle, int ndim, char* data);

LIBRARY_API char* udaGetSyntheticData(int handle);

LIBRARY_API char* udaGetData(int handle);

LIBRARY_API void udaGetDataTdi(int handle, char* data);

LIBRARY_API char* udaGetAsymmetricError(int handle, int above);

LIBRARY_API char* udaGetDataErrLo(int handle);

LIBRARY_API char* udaGetDataErrHi(int handle);

LIBRARY_API int udaGetDataErrAsymmetry(int handle);

LIBRARY_API void udaSetDataErrAsymmetry(int handle, int asymmetry);

LIBRARY_API void udaSetDataErrType(int handle, int type);

LIBRARY_API void udaSetDataErrLo(int handle, char* errlo);

LIBRARY_API char* udaGetDimErrLo(int handle, int ndim);

LIBRARY_API char* udaGetDimErrHi(int handle, int ndim);

LIBRARY_API int udaGetDimErrAsymmetry(int handle, int ndim);

LIBRARY_API void udaSetDimErrAsymmetry(int handle, int ndim, int asymmetry);

LIBRARY_API void udaSetDimErrType(int handle, int ndim, int type);

LIBRARY_API void udaSetDimErrLo(int handle, int ndim, char* errlo);

LIBRARY_API char* udaGetError(int handle);

LIBRARY_API void udaGetDoubleData(int handle, double* fp);

LIBRARY_API void udaGetFloatData(int handle, float* fp);

LIBRARY_API void udaGetGenericData(int handle, void* data);

LIBRARY_API void udaGetFloatAsymmetricError(int handle, int above, float* fp);

LIBRARY_API void udaGetFloatError(int handle, float* fp);

LIBRARY_API void udaGetDBlock(int handle, DATA_BLOCK* db);

LIBRARY_API DATA_BLOCK* udaGetDataBlock(int handle);

LIBRARY_API const char* udaGetDataLabel(int handle);

LIBRARY_API void udaGetDataLabelTdi(int handle, char* label);

LIBRARY_API const char* udaGetDataUnits(int handle);

LIBRARY_API void udaGetDataUnitsTdi(int handle, char* units);

LIBRARY_API const char* udaGetDataDesc(int handle);

LIBRARY_API void udaGetDataDescTdi(int handle, char* desc);

LIBRARY_API int udaGetDimNum(int handle, int ndim);

LIBRARY_API int udaGetDimType(int handle, int ndim);

LIBRARY_API int udaGetDimErrorType(int handle, int ndim);

LIBRARY_API int udaGetDimErrorAsymmetry(int handle, int ndim);

LIBRARY_API void udaGetDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params);

LIBRARY_API char* udaGetSyntheticDimData(int handle, int ndim);

LIBRARY_API char* udaGetDimData(int handle, int ndim);

LIBRARY_API const char* udaGetDimLabel(int handle, int ndim);

LIBRARY_API const char* udaGetDimUnits(int handle, int ndim);

LIBRARY_API void udaGetDimLabelTdi(int handle, int ndim, char* label);

LIBRARY_API void udaGetDimUnitsTdi(int handle, int ndim, char* units);

LIBRARY_API void udaGetDoubleDimData(int handle, int ndim, double* fp);

LIBRARY_API void udaGetFloatDimData(int handle, int ndim, float* fp);

LIBRARY_API void udaGetGenericDimData(int handle, int ndim, void* data);

LIBRARY_API DIMS* udaGetDimBlock(int handle, int ndim);

LIBRARY_API char* udaGetDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char* udaGetDimError(int handle, int ndim);

LIBRARY_API void udaGetFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

LIBRARY_API void udaGetFloatDimError(int handle, int ndim, float* fp);

LIBRARY_API DATA_SYSTEM* udaGetDataSystem(int handle);

LIBRARY_API SYSTEM_CONFIG* udaGetSystemConfig(int handle);

LIBRARY_API DATA_SOURCE* udaGetDataSource(int handle);

LIBRARY_API SIGNAL* udaGetSignal(int handle);

LIBRARY_API SIGNAL_DESC* udaGetSignalDesc(int handle);

LIBRARY_API const char* udaGetFileFormat(int handle);

// LIBRARY_API void udaInitDataBlock(DATA_BLOCK* str);

// LIBRARY_API void udaInitRequestBlock(REQUEST_BLOCK* str);

LIBRARY_API int udaDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int udaGetDataCheckSum(int handle);

LIBRARY_API int udaGetDimDataCheckSum(int handle, int ndim);

LIBRARY_API void udaLockThread();

LIBRARY_API void udaUnlockThread();

LIBRARY_API void udaFreeThread();

LIBRARY_API int udaGetThreadLastHandle();

LIBRARY_API void udaPutThreadLastHandle(int handle);

LIBRARY_API int udaGetMaxThreadCount();

LIBRARY_API SERVER_BLOCK udaGetThreadServerBlock();

LIBRARY_API CLIENT_BLOCK udaGetThreadClientBlock();

LIBRARY_API void udaPutThreadServerBlock(SERVER_BLOCK* str);

LIBRARY_API void udaPutThreadClientBlock(CLIENT_BLOCK* str);

LIBRARY_API int udaSetDataTree(int handle);

// Return a specific data tree

LIBRARY_API NTREE* udaGetDataTree(int handle);

// Return a user defined data structure definition

LIBRARY_API USERDEFINEDTYPE* udaGetUserDefinedType(int handle);

LIBRARY_API USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);

LIBRARY_API LOGMALLOCLIST* udaGetLogMallocList(int handle);

// LIBRARY_API NTREE* udaFindNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_ACCAPI_H
