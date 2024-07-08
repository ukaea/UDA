#ifndef UDA_CLIENT_ACCAPI_H
#define UDA_CLIENT_ACCAPI_H

#include <stdio.h>
#include <stdbool.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>
#include "udaClient.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_growIdamDataBlocks(CLIENT_FLAGS* client_flags);

LIBRARY_API int acc_getIdamNewDataHandle(CLIENT_FLAGS* client_flags);

LIBRARY_API void acc_freeDataBlocks();

LIBRARY_API void setIdamPrivateFlag(unsigned int flag);

LIBRARY_API void resetIdamPrivateFlag(unsigned int flag);

LIBRARY_API void setIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void resetIdamClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void setIdamProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API int getIdamProperty(const char* property, const CLIENT_FLAGS* client_flags);

LIBRARY_API void resetIdamProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API void resetIdamProperties(CLIENT_FLAGS* client_flags);

LIBRARY_API CLIENT_BLOCK saveIdamProperties(const CLIENT_FLAGS* client_flags);

LIBRARY_API void restoreIdamProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags);

LIBRARY_API CLIENT_BLOCK* getIdamProperties(int handle);

LIBRARY_API CLIENT_BLOCK* getIdamDataProperties(int handle);

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

LIBRARY_API UDA_ERROR_STACK* getUdaServerErrorStack();

LIBRARY_API int getIdamErrorCode(int handle);

LIBRARY_API const char* getIdamErrorMsg(int handle);

LIBRARY_API int getIdamSourceStatus(int handle);

LIBRARY_API int getIdamSignalStatus(int handle);

LIBRARY_API int getIdamDataStatus(int handle);

LIBRARY_API int getIdamLastHandle(CLIENT_FLAGS* client_flags);

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

LIBRARY_API void getIdamDBlock(int handle, DATA_BLOCK* db);

LIBRARY_API DATA_BLOCK* getIdamDataBlock(int handle);

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

LIBRARY_API DIMS* getIdamDimBlock(int handle, int ndim);

LIBRARY_API char* getIdamDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char* getIdamDimError(int handle, int ndim);

LIBRARY_API void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

LIBRARY_API void getIdamFloatDimError(int handle, int ndim, float* fp);

LIBRARY_API DATA_SYSTEM* getIdamDataSystem(int handle);

LIBRARY_API SYSTEM_CONFIG* getIdamSystemConfig(int handle);

LIBRARY_API DATA_SOURCE* getIdamDataSource(int handle);

LIBRARY_API SIGNAL* getIdamSignal(int handle);

LIBRARY_API SIGNAL_DESC* getIdamSignalDesc(int handle);

LIBRARY_API const char* getIdamFileFormat(int handle);

LIBRARY_API void initIdamDataBlock(DATA_BLOCK* str);

LIBRARY_API void initIdamRequestBlock(REQUEST_BLOCK* str);

LIBRARY_API int idamDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int getIdamDataCheckSum(int handle);

LIBRARY_API int getIdamDimDataCheckSum(int handle, int ndim);

LIBRARY_API void lockIdamThread(CLIENT_FLAGS* client_flags);

LIBRARY_API void unlockUdaThread(CLIENT_FLAGS* client_flags);

LIBRARY_API void freeIdamThread(CLIENT_FLAGS* client_flags);

LIBRARY_API int getIdamThreadLastHandle();

LIBRARY_API void putIdamThreadLastHandle(int handle);

LIBRARY_API int getIdamMaxThreadCount();

LIBRARY_API SERVER_BLOCK getIdamThreadServerBlock();

LIBRARY_API CLIENT_BLOCK getIdamThreadClientBlock();

LIBRARY_API void putIdamThreadServerBlock(SERVER_BLOCK *str);

LIBRARY_API void putIdamThreadClientBlock(CLIENT_BLOCK *str);

LIBRARY_API int setIdamDataTree(int handle);

// Return a specific data tree

LIBRARY_API NTREE* getIdamDataTree(int handle);

// Return a user defined data structure definition

LIBRARY_API USERDEFINEDTYPE* getIdamUserDefinedType(int handle);

LIBRARY_API USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle);

LIBRARY_API LOGMALLOCLIST* getIdamLogMallocList(int handle);

LIBRARY_API NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_ACCAPI_H

