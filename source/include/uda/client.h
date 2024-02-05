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

#define MIN_STATUS          (-1)        // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD     (-17000)    // Error Code if Status is Bad

typedef struct LogMallocList LOGMALLOCLIST;
typedef struct UserDefinedTypeList USERDEFINEDTYPELIST;

#ifdef FATCLIENT
#  define idamGetAPI idamGetAPIFat
#  define idamGetBatchAPI idamGetBatchAPIFat
#  define idamGetAPIWithHost idamGetAPIWithHostFat
#  define idamGetBatchAPIWithHost idamGetBatchAPIWithHostFat
#endif

LIBRARY_API int idamGetAPI(const char* data_object, const char* data_source);

LIBRARY_API int idamGetBatchAPI(const char** uda_signals, const char** sources, int count, int* handles);

LIBRARY_API int idamGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port);

LIBRARY_API int idamGetBatchAPIWithHost(const char** uda_signals, const char** sources, int count, int* handles,
                                        const char* host, int port);

LIBRARY_API int idamPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList);

LIBRARY_API int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

LIBRARY_API int idamClientAPI(const char *file, const char *signal, int pass, int exp_number);

LIBRARY_API int idamClientFileAPI(const char *file, const char *signal, const char *format);

LIBRARY_API int idamClientFileAPI2(const char *file, const char *format, const char *owner, const char *signal,
                                   int exp_number, int pass);

LIBRARY_API int idamClientTestAPI(const char *file, const char *signal, int pass, int exp_number);

LIBRARY_API int idamClientMDS(const char *server, const char *tree, const char *node, int treenum);

LIBRARY_API void udaFree(int handle);

LIBRARY_API void udaFreeAll();

/**
 * Get the version of the client c-library.
 */
LIBRARY_API const char *udaGetBuildVersion();

/**
 * Get the date that the client c-library was built.
 */
LIBRARY_API const char *udaGetBuildDate();

LIBRARY_API const char *getIdamServerHost();

LIBRARY_API int getIdamServerPort();

LIBRARY_API int getIdamServerSocket();

LIBRARY_API const char *getIdamClientDOI();

LIBRARY_API const char *getIdamServerDOI();

LIBRARY_API const char *getIdamClientOSName();

LIBRARY_API const char *getIdamServerOSName();

LIBRARY_API int getIdamClientVersion();

LIBRARY_API int getIdamServerVersion();

LIBRARY_API int getIdamServerErrorCode();

LIBRARY_API const char *getIdamServerErrorMsg();

LIBRARY_API int getIdamServerErrorStackSize();

LIBRARY_API int getIdamServerErrorStackRecordType(int record);

LIBRARY_API int getIdamServerErrorStackRecordCode(int record);

LIBRARY_API const char *getIdamServerErrorStackRecordLocation(int record);

LIBRARY_API const char *getIdamServerErrorStackRecordMsg(int record);

LIBRARY_API void closeAllConnections();

LIBRARY_API int udaNumErrors(void);
LIBRARY_API const char *udaGetErrorMessage(int err_num);
LIBRARY_API int udaGetErrorCode(int err_num);
LIBRARY_API const char *udaGetErrorLocation(int err_num);

LIBRARY_API PUTDATA_BLOCK* udaNewPutDataBlock(UDA_TYPE data_type, int count, int rank, int* shape, const char* data);
LIBRARY_API void udaFreePutDataBlock(PUTDATA_BLOCK* putdata_block);

LIBRARY_API bool udaHasMetaData(int handle);

LIBRARY_API LOGMALLOCLIST *getIdamLogMallocList(int handle);
LIBRARY_API USERDEFINEDTYPELIST *getIdamUserDefinedTypeList(int handle);

#define UDA_NUM_CLIENT_THREADS 30

LIBRARY_API void setIdamPrivateFlag(unsigned int flag);

LIBRARY_API void resetIdamPrivateFlag(unsigned int flag);

LIBRARY_API void setIdamClientFlag(unsigned int flag);

LIBRARY_API void resetIdamClientFlag(unsigned int flag);

LIBRARY_API void setIdamProperty(const char *property);

LIBRARY_API int getIdamProperty(const char *property);

LIBRARY_API void resetIdamProperty(const char *property);

LIBRARY_API void resetIdamProperties();

#ifndef __APPLE__

LIBRARY_API int getIdamMemoryFree();

LIBRARY_API int getIdamMemoryUsed();

#endif

LIBRARY_API void putIdamErrorModel(int handle, int model, int param_n, const float *params);

LIBRARY_API void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, const float *params);

LIBRARY_API void putIdamServer(const char *host, int port);

LIBRARY_API void putIdamServerHost(const char *host);

LIBRARY_API void putIdamServerPort(int port);

LIBRARY_API void putIdamServerSocket(int socket);

LIBRARY_API void getIdamServer(const char **host, int *port, int *socket);

LIBRARY_API int getIdamErrorCode(int handle);

LIBRARY_API const char *getIdamErrorMsg(int handle);

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

LIBRARY_API void *getIdamDataOpaqueBlock(int handle);

LIBRARY_API int getIdamDataOpaqueCount(int handle);

LIBRARY_API void getIdamErrorModel(int handle, int *model, int *param_n, float *params);

LIBRARY_API int getIdamErrorType(int handle);

LIBRARY_API int getIdamDataTypeId(const char *type);

LIBRARY_API int getIdamDataTypeSize(int type);

LIBRARY_API void getIdamErrorModel(int handle, int *model, int *param_n, float *params);

LIBRARY_API int getIdamErrorAsymmetry(int handle);

LIBRARY_API int getIdamErrorModelId(const char *model);

LIBRARY_API char *acc_getSyntheticData(int handle);

LIBRARY_API char *acc_getSyntheticDimData(int handle, int ndim);

LIBRARY_API void acc_setSyntheticData(int handle, char *data);

LIBRARY_API void acc_setSyntheticDimData(int handle, int ndim, char *data);

LIBRARY_API char *getIdamSyntheticData(int handle);

LIBRARY_API char *getIdamData(int handle);

LIBRARY_API void getIdamDataTdi(int handle, char *data);

LIBRARY_API char *getIdamAsymmetricError(int handle, int above);

LIBRARY_API char *getIdamDataErrLo(int handle);

LIBRARY_API char *getIdamDataErrHi(int handle);

LIBRARY_API int getIdamDataErrAsymmetry(int handle);

LIBRARY_API void acc_setIdamDataErrAsymmetry(int handle, int asymmetry);

LIBRARY_API void acc_setIdamDataErrType(int handle, int type);

LIBRARY_API void acc_setIdamDataErrLo(int handle, char *errlo);

LIBRARY_API char *getIdamDimErrLo(int handle, int ndim);

LIBRARY_API char *getIdamDimErrHi(int handle, int ndim);

LIBRARY_API int getIdamDimErrAsymmetry(int handle, int ndim);

LIBRARY_API void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry);

LIBRARY_API void acc_setIdamDimErrType(int handle, int ndim, int type);

LIBRARY_API void acc_setIdamDimErrLo(int handle, int ndim, char *errlo);

LIBRARY_API char *getIdamError(int handle);

LIBRARY_API void getIdamDoubleData(int handle, double *fp);

LIBRARY_API void getIdamFloatData(int handle, float *fp);

LIBRARY_API void getIdamGenericData(int handle, void *data);

LIBRARY_API void getIdamFloatAsymmetricError(int handle, int above, float *fp);

LIBRARY_API void getIdamFloatError(int handle, float *fp);

LIBRARY_API const char *getIdamDataLabel(int handle);

LIBRARY_API void getIdamDataLabelTdi(int handle, char *label);

LIBRARY_API const char *getIdamDataUnits(int handle);

LIBRARY_API void getIdamDataUnitsTdi(int handle, char *units);

LIBRARY_API const char *getIdamDataDesc(int handle);

LIBRARY_API void getIdamDataDescTdi(int handle, char *desc);

LIBRARY_API int getIdamDimNum(int handle, int ndim);

LIBRARY_API int getIdamDimType(int handle, int ndim);

LIBRARY_API int getIdamDimErrorType(int handle, int ndim);

LIBRARY_API int getIdamDimErrorAsymmetry(int handle, int ndim);

LIBRARY_API void getIdamDimErrorModel(int handle, int ndim, int *model, int *param_n, float *params);

LIBRARY_API char *getIdamSyntheticDimData(int handle, int ndim);

LIBRARY_API char *getIdamDimData(int handle, int ndim);

LIBRARY_API const char *getIdamDimLabel(int handle, int ndim);

LIBRARY_API const char *getIdamDimUnits(int handle, int ndim);

LIBRARY_API void getIdamDimLabelTdi(int handle, int ndim, char *label);

LIBRARY_API void getIdamDimUnitsTdi(int handle, int ndim, char *units);

LIBRARY_API void getIdamDoubleDimData(int handle, int ndim, double *fp);

LIBRARY_API void getIdamFloatDimData(int handle, int ndim, float *fp);

LIBRARY_API void getIdamGenericDimData(int handle, int ndim, void *data);

LIBRARY_API char *getIdamDimAsymmetricError(int handle, int ndim, int above);

LIBRARY_API char *getIdamDimError(int handle, int ndim);

LIBRARY_API void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float *fp);

LIBRARY_API void getIdamFloatDimError(int handle, int ndim, float *fp);

LIBRARY_API int idamDataCheckSum(void *data, int data_n, int type);

LIBRARY_API int getIdamDataCheckSum(int handle);

LIBRARY_API int getIdamDimDataCheckSum(int handle, int ndim);

LIBRARY_API int getIdamThreadLastHandle();

LIBRARY_API void putIdamThreadLastHandle(int handle);

LIBRARY_API int getIdamMaxThreadCount();

LIBRARY_API int setIdamDataTree(int handle);

LIBRARY_API NTREE *getIdamDataTree(int handle);

LIBRARY_API NTREE *findIdamNTreeStructureDefinition(NTREE *node, const char *target);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H