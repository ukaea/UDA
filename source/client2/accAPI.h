#ifndef UDA_CLIENT_ACCAPI_H
#define UDA_CLIENT_ACCAPI_H

#ifdef __cplusplus
#  include <cstdio>
#  include <cstdbool>
#else
#  include <stdio.h>
#  include <stdbool.h>
#endif

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ClientFlags {
    int get_dimdble;         // (Server Side) Return Dimensional Data in Double Precision
    int get_timedble;        // (Server Side) Server Side cast of time dimension to double precision if in compresed format
    int get_scalar;          // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
    int get_bytes;           // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
// calibration factor applied
    int get_meta;            // (Server Side) return All Meta Data
    int get_asis;            // (Server Side) Apply no XML based corrections to Data or Dimensions
    int get_uncal;           // (Server Side) Apply no XML based Calibrations to Data
    int get_notoff;          // (Server Side) Apply no XML based Timing Corrections to Data
    int get_nodimdata;

    int get_datadble;        // (Client Side) Return Data in Double Precision
    int get_bad;             // (Client Side) return data with BAD Status value
    int get_synthetic;       // (Client Side) Return Synthetic Data if available instead of Original data

    uint32_t flags;

    int user_timeout;
    int alt_rank;
} CLIENT_FLAGS;

#define MIN_STATUS          (-1)        // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD     (-17000)    // Error Code if Status is Bad

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API DATA_BLOCK* udaGetCurrentDataBlock();

LIBRARY_API int udaGetIdamNewDataHandle();

LIBRARY_API void udaFreeDataBlocks();

LIBRARY_API void udaSetPrivateFlag(unsigned int flag, unsigned int* private_flags);

LIBRARY_API void udaResetPrivateFlag(unsigned int flag, unsigned int* private_flags);

LIBRARY_API void udaSetClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void udaResetClientFlag(CLIENT_FLAGS* client_flags, unsigned int flag);

LIBRARY_API void udaSetProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API int udaGetProperty(const char* property, const CLIENT_FLAGS* client_flags, int user_timeout, int alt_rank);

LIBRARY_API void udaResetProperty(const char* property, CLIENT_FLAGS* client_flags);

LIBRARY_API void udaResetProperties();

LIBRARY_API CLIENT_BLOCK udaSaveProperties(const CLIENT_FLAGS* client_flags, int alt_rank);

LIBRARY_API void udaRestoreProperties(CLIENT_BLOCK cb, CLIENT_FLAGS* client_flags, int* alt_rank);

LIBRARY_API const CLIENT_BLOCK* udaGetProperties(int handle);

LIBRARY_API CLIENT_BLOCK* udaGetDataProperties(int handle);

LIBRARY_API void udaPutErrorFileHandle(FILE* fh);

LIBRARY_API void udaPutDebugFileHandle(FILE* fh);

#ifndef __APPLE__

LIBRARY_API int getIdamMemoryFree();

LIBRARY_API int getIdamMemoryUsed();

#endif

LIBRARY_API void udaPutErrorModel(int handle, int model, int param_n, const float* params);

LIBRARY_API void udaPutDimErrorModel(int handle, int ndim, int model, int param_n, const float* params);

LIBRARY_API void udaPutServer(const char* host, int port, bool* env_host, bool* env_port);

LIBRARY_API void udaPutServerHost(const char* host, bool* env_host);

LIBRARY_API void udaPutServerPort(int port, bool* env_port);

LIBRARY_API void udaPutServerSocket(int socket);

LIBRARY_API void udaGetServer(const char** host, int* port, int* socket);

LIBRARY_API UDA_ERROR_STACK* udaGetServerErrorStack();

LIBRARY_API int udaGetErrorCode(int handle);

LIBRARY_API const char* udaGetErrorMsg(int handle);

LIBRARY_API int udGetSourceStatus(int handle);

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

LIBRARY_API int udaGetErrorAsymmetry(int handle);

LIBRARY_API int udaGetErrorModelId(const char* model);

LIBRARY_API char* udaGetSyntheticDimData(int handle, int ndim);

LIBRARY_API void udaSetSyntheticData(int handle, char* data);

LIBRARY_API void udaSetSyntheticDimData(int handle, int ndim, char* data);

LIBRARY_API char* getIdamSyntheticData(int handle);

LIBRARY_API char* udaGetData(int handle);

LIBRARY_API void udaGetDataTdi(int handle, char* data);

LIBRARY_API char* udaGetAsymmetricError(int handle, bool above);

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

LIBRARY_API char* getIdamSyntheticDimData(int handle, int ndim);

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

LIBRARY_API void udaInitDataBlock(DATA_BLOCK* str);

LIBRARY_API void udaInitRequestBlock(REQUEST_BLOCK* str);

LIBRARY_API int udaDataCheckSum(void* data, int data_n, int type);

LIBRARY_API int udaGetDataCheckSum(int handle);

LIBRARY_API int udaGetDimDataCheckSum(int handle, int ndim);

LIBRARY_API int udaGetThreadLastHandle();

LIBRARY_API void udaPutThreadLastHandle(int handle);

LIBRARY_API int udaGetMaxThreadCount();

LIBRARY_API SERVER_BLOCK udaGetThreadServerBlock();

LIBRARY_API CLIENT_BLOCK udaGetThreadClientBlock();

LIBRARY_API void udaPutThreadServerBlock(SERVER_BLOCK *str);

LIBRARY_API void udaPutThreadClientBlock(CLIENT_BLOCK *str);

LIBRARY_API int udaSetDataTree(int handle);

// Return a specific data tree

LIBRARY_API NTREE* udaGetDataTree(int handle);

// Return a user defined data structure definition

LIBRARY_API USERDEFINEDTYPE* udaGetUserDefinedType(int handle);

LIBRARY_API USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);

LIBRARY_API LOGMALLOCLIST* udaGetLogMallocList(int handle);

LIBRARY_API NTREE* udaFindNTreeStructureDefinition(NTREE* node, const char* target, NTREE* full_ntree);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_ACCAPI_H

