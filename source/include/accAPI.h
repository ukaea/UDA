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

LIBRARY_API DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_growUdaDataBlocks(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_getUdaNewDataHandle(CLIENT_FLAGS* client_flags);

LIBRARY_API void acc_freeDataBlocks();

LIBRARY_API void setUdaPrivateFlag(unsigned int flag);

LIBRARY_API void resetUdaPrivateFlag(unsigned int flag);

LIBRARY_API void setUdaClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void resetUdaClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void setUdaProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API int getUdaProperty(const char* property, const CLIENT_FLAGS* client_flags);

LIBRARY_API void resetUdaProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API void resetUdaProperties(CLIENT_FLAGS* client_flags);

LIBRARY_API CLIENT_BLOCK saveUdaProperties(const CLIENT_FLAGS* client_flags);

LIBRARY_API void restoreUdaProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags);

LIBRARY_API CLIENT_BLOCK* getUdaProperties(int handle);

LIBRARY_API CLIENT_BLOCK* getUdaDataProperties(int handle);

#ifndef __APPLE__

LIBRARY_API int getUdaMemoryFree();

LIBRARY_API int getUdaMemoryUsed();

#endif

LIBRARY_API void putUdaErrorModel(int handle, int model, int param_n, const float* params);

LIBRARY_API void putUdaDimErrorModel(int handle, int ndim, int model, int param_n, const float* params);

LIBRARY_API void putUdaServer(const char* host, int port);

LIBRARY_API void putUdaServerHost(const char* host);

LIBRARY_API void putUdaServerPort(int port);

LIBRARY_API void putUdaServerSocket(int socket);

LIBRARY_API void getUdaServer(const char** host, int* port, int* socket);

LIBRARY_API UDA_ERROR_STACK* getUdaServerErrorStack();

LIBRARY_API int getUdaErrorCode(int handle);

LIBRARY_API const char* getUdaErrorMsg(int handle);

LIBRARY_API int getUdaSourceStatus(int handle);

LIBRARY_API int getUdaSignalStatus(int handle);

LIBRARY_API int getUdaDataStatus(int handle);

LIBRARY_API int getUdaLastHandle(CLIENT_FLAGS* client_flags);

LIBRARY_API int getUdaDataNum(int handle);

LIBRARY_API int getUdaRank(int handle);

LIBRARY_API int getUdaOrder(int handle);

LIBRARY_API unsigned int getUdaCachePermission(int handle);

LIBRARY_API unsigned int getUdaTotalDataBlockSize(int handle);

LIBRARY_API int getUdaDataType(int handle);

LIBRARY_API int getUdaDataOpaqueType(int handle);

LIBRARY_API void* getUdaDataOpaqueBlock(int handle);

LIBRARY_API int getUdaDataOpaqueCount(int handle);

LIBRARY_API void getUdaErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int getUdaErrorType(int handle);

LIBRARY_API int getUdaDataTypeId(const char* type);

LIBRARY_API int getUdaDataTypeSize(int type);

LIBRARY_API void getUdaErrorModel(int handle, int* model, int* param_n, float* params);

LIBRARY_API int getUdaErrorAsymmetry(int handle);

LIBRARY_API int getUdaErrorModelId(const char* model);

LIBRARY_API char* acc_getSyntheticData(int handle);

LIBRARY_API char* acc_getSyntheticDimData(int handle, int ndim);

LIBRARY_API void acc_setSyntheticData(int handle, char* data);

LIBRARY_API void acc_setSyntheticDimData(int handle, int ndim, char* data);

LIBRARY_API char* getUdaSyntheticData(int handle);

LIBRARY_API char* getUdaData(int handle);

LIBRARY_API void getUdaDataTdi(int handle, char* data);

LIBRARY_API char* getUdaAsymmetricError(int handle, int above);

LIBRARY_API char* getUdaDataErrLo(int handle);

LIBRARY_API char* getUdaDataErrHi(int handle);

LIBRARY_API int getUdaDataErrAsymmetry(int handle);

LIBRARY_API void acc_setUdaDataErrAsymmetry(int handle, int asymmetry);

LIBRARY_API void acc_setUdaDataErrType(int handle, int type);

LIBRARY_API void acc_setUdaDataErrLo(int handle, char* errlo);

LIBRARY_API char* getUdaDimErrLo(int handle, int ndim);

LIBRARY_API char* getUdaDimErrHi(int handle, int ndim);

LIBRARY_API int getUdaDimErrAsymmetry(int handle, int ndim);

LIBRARY_API void acc_setUdaDimErrAsymmetry(int handle, int ndim, int asymmetry);

LIBRARY_API void acc_setUdaDimErrType(int handle, int ndim, int type);

LIBRARY_API void acc_setUdaDimErrLo(int handle, int ndim, char* errlo);

LIBRARY_API char* getUdaError(int handle);

LIBRARY_API void getUdaDoubleData(int handle, double* fp);

LIBRARY_API void getUdaFloatData(int handle, float* fp);

LIBRARY_API void getUdaGenericData(int handle, void* data);

LIBRARY_API void getUdaFloatAsymmetricError(int handle, int above, float* fp);

LIBRARY_API void getUdaFloatError(int handle, float* fp);

LIBRARY_API void getUdaDBlock(int handle, DATA_BLOCK* db);

LIBRARY_API DATA_BLOCK* getUdaDataBlock(int handle);

LIBRARY_API const char* getUdaDataLabel(int handle);

LIBRARY_API void getUdaDataLabelTdi(int handle, char* label);

LIBRARY_API const char* getUdaDataUnits(int handle);

LIBRARY_API void getUdaDataUnitsTdi(int handle, char* units);

LIBRARY_API const char* getUdaDataDesc(int handle);

LIBRARY_API void getUdaDataDescTdi(int handle, char* desc);

LIBRARY_API int getUdaDimNum(int handle, int ndim);

LIBRARY_API int getUdaDimType(int handle, int ndim);

LIBRARY_API int getUdaDimErrorType(int handle, int ndim);

LIBRARY_API int getUdaDimErrorAsymmetry(int handle, int ndim);

LIBRARY_API void getUdaDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params);

LIBRARY_API char* getUdaSyntheticDimData(int handle, int ndim);

LIBRARY_API char* getUdaDimData(int handle, int ndim);

LIBRARY_API const char* getUdaDimLabel(int handle, int ndim);

LIBRARY_API const char* getUdaDimUnits(int handle, int ndim);

LIBRARY_API void getUdaDimLabelTdi(int handle, int ndim, char* label);

LIBRARY_API void getUdaDimUnitsTdi(int handle, int ndim, char* units);

LIBRARY_API void getUdaDoubleDimData(int handle, int ndim, double* fp);

LIBRARY_API void getUdaFloatDimData(int handle, int ndim, float* fp);

LIBRARY_API void getUdaGenericDimData(int handle, int ndim, void* data);

LIBRARY_API DIMS* getUdaDimBlock(int handle, int ndim);

LIBRARY_API char* getUdaDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char* getUdaDimError(int handle, int ndim);

LIBRARY_API void getUdaFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

LIBRARY_API void getUdaFloatDimError(int handle, int ndim, float* fp);

LIBRARY_API DATA_SYSTEM* getUdaDataSystem(int handle);

LIBRARY_API SYSTEM_CONFIG* getUdaSystemConfig(int handle);

LIBRARY_API DATA_SOURCE* getUdaDataSource(int handle);

LIBRARY_API SIGNAL* getUdaSignal(int handle);

LIBRARY_API SIGNAL_DESC* getUdaSignalDesc(int handle);

LIBRARY_API const char* getUdaFileFormat(int handle);

LIBRARY_API void initUdaDataBlock(DATA_BLOCK* str);

LIBRARY_API void initUdaRequestBlock(REQUEST_BLOCK* str);

LIBRARY_API int idamDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int getUdaDataCheckSum(int handle);

LIBRARY_API int getUdaDimDataCheckSum(int handle, int ndim);

LIBRARY_API void lockUdaThread(CLIENT_FLAGS* client_flags);

LIBRARY_API void unlockUdaThread(CLIENT_FLAGS* client_flags);

LIBRARY_API void freeUdaThread(CLIENT_FLAGS* client_flags);

LIBRARY_API int getUdaThreadLastHandle();

LIBRARY_API void putUdaThreadLastHandle(int handle);

LIBRARY_API int getUdaMaxThreadCount();

LIBRARY_API SERVER_BLOCK getUdaThreadServerBlock();

LIBRARY_API CLIENT_BLOCK getUdaThreadClientBlock();

LIBRARY_API void putUdaThreadServerBlock(SERVER_BLOCK* str);

LIBRARY_API void putUdaThreadClientBlock(CLIENT_BLOCK* str);

LIBRARY_API int setUdaDataTree(int handle);

// Return a specific data tree

LIBRARY_API NTREE* getUdaDataTree(int handle);

// Return a user defined data structure definition

LIBRARY_API USERDEFINEDTYPE* getUdaUserDefinedType(int handle);

LIBRARY_API USERDEFINEDTYPELIST* getUdaUserDefinedTypeList(int handle);

LIBRARY_API LOGMALLOCLIST* getUdaLogMallocList(int handle);

LIBRARY_API NTREE* findUdaNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_ACCAPI_H
