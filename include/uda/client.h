#ifndef UDA_CLIENT_H
#define UDA_CLIENT_H

#include <uda/export.h>
#include <uda/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NO_SOCKET_CONNECTION (-10000)
#define PROBLEM_OPENING_LOGS (-11000)
#define FILE_FORMAT_NOT_SUPPORTED (-12000)
#define ERROR_ALLOCATING_DATA_BOCK_HEAP (-13000)
#define SERVER_BLOCK_ERROR (-14000)
#define SERVER_SIDE_ERROR (-14001)
#define DATA_BLOCK_RECEIPT_ERROR (-15000)
#define ERROR_CONDITION_UNKNOWN (-16000)

#define MIN_STATUS (-1)          // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD (-17000) // Error Code if Status is Bad

#ifdef FATCLIENT
#  define udaGetAPI udaGetAPIFat
#  define udaGetBatchAPI udaGetBatchAPIFat
#  define udaGetAPIWithHost udaGetAPIWithHostFat
#  define udaGetBatchAPIWithHost udaGetBatchAPIWithHostFat
#endif

LIBRARY_API int udaGetAPI(const char* data_object, const char* data_source);

LIBRARY_API int udaGetBatchAPI(const char** uda_signals, const char** sources, int count, int* handles);

LIBRARY_API int udaGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port);

LIBRARY_API int udaGetBatchAPIWithHost(const char** uda_signals, const char** sources, int count, int* handles,
                                       const char* host, int port);

LIBRARY_API int udaPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList);

LIBRARY_API int udaPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

LIBRARY_API void udaFree(int handle);

LIBRARY_API void udaFreeAll();

/**
 * Get the date that the client c-library was built.
 */
LIBRARY_API const char* udaGetBuildDate();

LIBRARY_API const char* udaGetServerHost();

LIBRARY_API int udaGetServerPort();

LIBRARY_API void udaGetClientVersionString(char* version_string);

LIBRARY_API int udaGetClientVersion();

LIBRARY_API int udaGetClientVersionMajor();

LIBRARY_API int udaGetClientVersionMinor();

LIBRARY_API int udaGetClientVersionBugfix();

LIBRARY_API int udaGetClientVersionDelta();

LIBRARY_API void udaGetServerVersionString(char* version_string);

LIBRARY_API int udaGetServerVersion();

LIBRARY_API int udaGetServerVersionMajor();

LIBRARY_API int udaGetServerVersionMinor();

LIBRARY_API int udaGetServerVersionBugfix();

LIBRARY_API int udaGetServerVersionDelta();

LIBRARY_API int udaGetServerErrorCode();

LIBRARY_API const char* udaGetServerErrorMsg();

LIBRARY_API int udaGetServerErrorStackSize();

LIBRARY_API int udaGetServerErrorStackRecordType(int record);

LIBRARY_API int udaGetServerErrorStackRecordCode(int record);

LIBRARY_API const char* udaGetServerErrorStackRecordLocation(int record);

LIBRARY_API const char* udaGetServerErrorStackRecordMsg(int record);

LIBRARY_API void udaCloseAllConnections();

LIBRARY_API int udaNumErrors(void);

LIBRARY_API const char* udaGetErrorMessage(int err_num);

LIBRARY_API int udaGetErrorCode(int err_num);

LIBRARY_API const char* udaGetErrorLocation(int err_num);

LIBRARY_API PUTDATA_BLOCK* udaNewPutDataBlock(UDA_TYPE data_type, int count, int rank, int* shape, const char* data);

LIBRARY_API void udaFreePutDataBlock(PUTDATA_BLOCK* putdata_block);

LIBRARY_API LOGMALLOCLIST* udaGetLogMallocList(int handle);

LIBRARY_API USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API void udaSetPrivateFlag(unsigned int flag);

LIBRARY_API void udaResetPrivateFlag(unsigned int flag);

LIBRARY_API void udaSetClientFlag(unsigned int flag);

LIBRARY_API void udaResetClientFlag(unsigned int flag);

LIBRARY_API void udaSetProperty(const char* property);

LIBRARY_API int udaGetProperty(const char* property);

LIBRARY_API void udaResetProperty(const char* property);

LIBRARY_API void udaResetProperties();

#ifndef __APPLE__

LIBRARY_API int udaGetMemoryFree();

LIBRARY_API int udaGetMemoryUsed();

#endif

LIBRARY_API void udaPutErrorModel(int handle, int model, int param_n, const float* params);

LIBRARY_API void udaPutDimErrorModel(int handle, int ndim, int model, int param_n, const float* params);

LIBRARY_API void udaPutServer(const char* host, int port);

LIBRARY_API void udaPutServerHost(const char* host);

LIBRARY_API void udaPutServerPort(int port);

LIBRARY_API int udaGetErrorCode(int handle);

LIBRARY_API const char* udaGetErrorMsg(int handle);

LIBRARY_API int udaGetSourceStatus(int handle);

LIBRARY_API int udaGetSignalStatus(int handle);

LIBRARY_API int udaGetDataStatus(int handle);

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

LIBRARY_API char* udaGetDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char* udaGetDimError(int handle, int ndim);

LIBRARY_API void udaGetFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

LIBRARY_API void udaGetFloatDimError(int handle, int ndim, float* fp);

LIBRARY_API int udaDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int udaGetDataCheckSum(int handle);

LIBRARY_API int udaGetDimDataCheckSum(int handle, int ndim);

LIBRARY_API int udaGetThreadLastHandle();

LIBRARY_API void udaPutThreadLastHandle(int handle);

LIBRARY_API void udaLockThread();

LIBRARY_API void udaUnlockThread();

LIBRARY_API int udaGetMaxThreadCount();

LIBRARY_API int udaSetDataTree(int handle);

LIBRARY_API NTREE* udaGetDataTree(int handle);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H
