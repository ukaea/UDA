#ifndef IDAM_CLIENT_ACCAPI_C_H
#define IDAM_CLIENT_ACCAPI_C_H

#include <stdio.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUMIDAMCLIENTTHREADS 30

#ifdef FATCLIENT
#  define acc_getCurrentDataBlock               acc_getCurrentDataBlockFat
#  define acc_getCurrentDataBlockIndex          acc_getCurrentDataBlockIndexFat
#  define acc_growIdamDataBlocks                acc_growIdamDataBlocksFat
#  define acc_getIdamNewDataHandle              acc_getIdamNewDataHandleFat
#  define acc_freeDataBlocks                    acc_freeDataBlocks
#  define setIdamPrivateFlag                    setIdamPrivateFlag
#  define resetIdamPrivateFlag                  resetIdamPrivateFlag
#  define setIdamClientFlag                     setIdamClientFlag
#  define resetIdamClientFlag                   resetIdamClientFlag
#  define setIdamProperty                       setIdamProperty
#  define getIdamProperty                       getIdamProperty
#  define resetIdamProperty                     resetIdamPropertyFat
#  define resetIdamProperties                   resetIdamPropertiesFat
#  define saveIdamProperties                    saveIdamPropertiesFat
#  define restoreIdamProperties                 restoreIdamPropertiesFat
#  define getIdamProperties                     getIdamPropertiesFat
#  define getIdamDataProperties                 getIdamDataPropertiesFat
#  define putIdamErrorFileHandle                putIdamErrorFileHandleFat
#  define putIdamDebugFileHandle                putIdamDebugFileHandleFat
#  define getIdamMemoryFree                     getIdamMemoryFreeFat
#  define getIdamMemoryUsed                     getIdamMemoryUsedFat
#  define putIdamErrorModel                     putIdamErrorModelFat
#  define putIdamDimErrorModel                  putIdamDimErrorModelFat
#  define putIdamServer                         putIdamServerFat
#  define putIdamServerHost                     putIdamServerHostFat
#  define putIdamServerPort                     putIdamServerPortFat
#  define putIdamServerSocket                   putIdamServerSocketFat
#  define getIdamServer                         getIdamServerFat
#  define getIdamServerErrorStack               getIdamServerErrorStackFat
#  define getIdamErrorCode                      getIdamErrorCodeFat
#  define getIdamErrorMsg                       getIdamErrorMsgFat
#  define getIdamSourceStatus                   getIdamSourceStatusFat
#  define getIdamSignalStatus                   getIdamSignalStatusFat
#  define getIdamDataStatus                     getIdamDataStatusFat
#  define getIdamLastHandle                     getIdamLastHandleFat
#  define getIdamDataNum                        getIdamDataNumFat
#  define getIdamRank                           getIdamRankFat
#  define getIdamOrder                          getIdamOrderFat
#  define getIdamCachePermission                getIdamCachePermissionFat
#  define getIdamTotalDataBlockSize             getIdamTotalDataBlockSizeFat
#  define getIdamDataType                       getIdamDataTypeFat
#  define getIdamDataOpaqueType                 getIdamDataOpaqueTypeFat
#  define getIdamDataOpaqueBlock                getIdamDataOpaqueBlockFat
#  define getIdamDataOpaqueCount                getIdamDataOpaqueCountFat
#  define getIdamErrorModel                     getIdamErrorModelFat
#  define getIdamErrorType                      getIdamErrorTypeFat
#  define getIdamDataTypeId                     getIdamDataTypeIdFat
#  define getIdamDataTypeSize                   getIdamDataTypeSizeFat
#  define getIdamErrorModel                     getIdamErrorModelFat
#  define getIdamErrorAsymmetry                 getIdamErrorAsymmetryFat
#  define getIdamErrorModelId                   getIdamErrorModelIdFat
#  define acc_getSyntheticData                  acc_getSyntheticDataFat
#  define acc_getSyntheticDimData               acc_getSyntheticDimDataFat
#  define acc_setSyntheticData                  acc_setSyntheticDataFat
#  define acc_setSyntheticDimData               acc_setSyntheticDimDataFat
#  define getIdamSyntheticData                  getIdamSyntheticDataFat
#  define getIdamData                           getIdamDataFat
#  define getIdamDataTdi                        getIdamDataTdiFat
#  define getIdamAsymmetricError                getIdamAsymmetricErrorFat
#  define getIdamDataErrLo                      getIdamDataErrLoFat
#  define getIdamDataErrHi                      getIdamDataErrHiFat
#  define getIdamDataErrAsymmetry               getIdamDataErrAsymmetryFat
#  define acc_setIdamDataErrAsymmetry           acc_setIdamDataErrAsymmetryFat
#  define acc_setIdamDataErrType                acc_setIdamDataErrTypeFat
#  define acc_setIdamDataErrLo                  acc_setIdamDataErrLoFat
#  define getIdamDimErrLo                       getIdamDimErrLoFat
#  define getIdamDimErrHi                       getIdamDimErrHiFat
#  define getIdamDimErrAsymmetry                getIdamDimErrAsymmetryFat
#  define acc_setIdamDimErrAsymmetry            acc_setIdamDimErrAsymmetryFat
#  define acc_setIdamDimErrType                 acc_setIdamDimErrTypeFat
#  define acc_setIdamDimErrLo                   acc_setIdamDimErrLoFat
#  define getIdamError                          getIdamErrorFat
#  define getIdamDoubleData                     getIdamDoubleDataFat
#  define getIdamFloatData                      getIdamFloatDataFat
#  define getIdamGenericData                    getIdamGenericDataFat
#  define getIdamFloatAsymmetricError           getIdamFloatAsymmetricErrorFat
#  define getIdamFloatError                     getIdamFloatErrorFat
#  define getIdamDBlock                         getIdamDBlockFat
#  define getIdamDataBlock                      getIdamDataBlockFat
#  define getIdamDataLabel                      getIdamDataLabelFat
#  define getIdamDataLabelTdi                   getIdamDataLabelTdiFat
#  define getIdamDataUnits                      getIdamDataUnitsFat
#  define getIdamDataUnitsTdi                   getIdamDataUnitsTdiFat
#  define getIdamDataDesc                       getIdamDataDescFat
#  define getIdamDataDescTdi                    getIdamDataDescTdiFat
#  define getIdamDimNum                         getIdamDimNumFat
#  define getIdamDimType                        getIdamDimTypeFat
#  define getIdamDimErrorType                   getIdamDimErrorTypeFat
#  define getIdamDimErrorAsymmetry              getIdamDimErrorAsymmetryFat
#  define getIdamDimErrorModel                  getIdamDimErrorModelFat
#  define getIdamSyntheticDimData               getIdamSyntheticDimDataFat
#  define getIdamDimData                        getIdamDimDataFat
#  define getIdamDimLabel                       getIdamDimLabelFat
#  define getIdamDimUnits                       getIdamDimUnitsFat
#  define getIdamDimLabelTdi                    getIdamDimLabelTdiFat
#  define getIdamDimUnitsTdi                    getIdamDimUnitsTdiFat
#  define getIdamDoubleDimData                  getIdamDoubleDimDataFat
#  define getIdamFloatDimData                   getIdamFloatDimDataFat
#  define getIdamGenericDimData                 getIdamGenericDimDataFat
#  define getIdamDimBlock                       getIdamDimBlockFat
#  define getIdamDimAsymmetricError             getIdamDimAsymmetricErrorFat
#  define getIdamDimError                       getIdamDimErrorFat
#  define getIdamFloatDimAsymmetricError        getIdamFloatDimAsymmetricErrorFat
#  define getIdamFloatDimError                  getIdamFloatDimErrorFat
#  define getIdamDataSystem                     getIdamDataSystemFat
#  define getIdamSystemConfig                   getIdamSystemConfigFat
#  define getIdamDataSource                     getIdamDataSourceFat
#  define getIdamSignal                         getIdamSignalFat
#  define getIdamSignalDesc                     getIdamSignalDescFat
#  define getIdamFileFormat                     getIdamFileFormatFat
#  define initIdamDataBlock                     initIdamDataBlockFat
#  define initIdamRequestBlock                  initIdamRequestBlockFat
#  define idamDataCheckSum                      idamDataCheckSumFat
#  define getIdamDataCheckSum                   getIdamDataCheckSumFat
#  define getIdamDimDataCheckSum                getIdamDimDataCheckSumFat
#  define lockIdamThread                        lockIdamThreadFat
#  define unlockIdamThread                      unlockIdamThreadFat
#  define freeIdamThread                        freeIdamThreadFat
#  define getIdamThreadLastHandle               getIdamThreadLastHandleFat
#  define putIdamThreadLastHandle               putIdamThreadLastHandleFat
#  define getIdamMaxThreadCount                 getIdamMaxThreadCountFat
#  define getIdamThreadServerBlock              getIdamThreadServerBlockFat
#  define getIdamThreadClientBlock              getIdamThreadClientBlockFat
#  define putIdamThreadServerBlock              putIdamThreadServerBlockFat
#  define putIdamThreadClientBlock              putIdamThreadClientBlockFat
#  define setIdamDataTree                       setIdamDataTreeFat
#  define getIdamDataTree                       getIdamDataTreeFat
#  define getIdamUserDefinedType                getIdamUserDefinedTypeFat
#  define getIdamUserDefinedTypeList            getIdamUserDefinedTypeListFat
#  define getIdamLogMallocList                  getIdamLogMallocListFat
#  define findIdamNTreeStructureDefinition      findIdamNTreeStructureDefinitionFat
#endif

DATA_BLOCK* acc_getCurrentDataBlock();

int acc_getCurrentDataBlockIndex();

int acc_growIdamDataBlocks();

int acc_getIdamNewDataHandle();

void acc_freeDataBlocks();

void setIdamPrivateFlag(unsigned int flag);

void resetIdamPrivateFlag(unsigned int flag);

void setIdamClientFlag(unsigned int flag);

void resetIdamClientFlag(unsigned int flag);

void setIdamProperty(const char* property);

int getIdamProperty(const char* property);

void resetIdamProperty(const char* property);

void resetIdamProperties();

CLIENT_BLOCK saveIdamProperties();

void restoreIdamProperties(CLIENT_BLOCK cb);

CLIENT_BLOCK* getIdamProperties(int handle);

CLIENT_BLOCK* getIdamDataProperties(int handle);

void putIdamErrorFileHandle(FILE* fh);

void putIdamDebugFileHandle(FILE* fh);

#ifndef __APPLE__

int getIdamMemoryFree();

int getIdamMemoryUsed();

#endif

void putIdamErrorModel(int handle, int model, int param_n, float* params);

void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, float* params);

void putIdamServer(const char* host, int port);

void putIdamServerHost(const char* host);

void putIdamServerPort(int port);

void putIdamServerSocket(int socket);

void getIdamServer(char** host, int* port, int* socket);

IDAMERRORSTACK* getIdamServerErrorStack();

int getIdamErrorCode(int handle);

char* getIdamErrorMsg(int handle);

int getIdamSourceStatus(int handle);

int getIdamSignalStatus(int handle);

int getIdamDataStatus(int handle);

int getIdamLastHandle();

int getIdamDataNum(int handle);

int getIdamRank(int handle);

int getIdamOrder(int handle);

unsigned int getIdamCachePermission(int handle);

unsigned int getIdamTotalDataBlockSize(int handle);

int getIdamDataType(int handle);

int getIdamDataOpaqueType(int handle);

void* getIdamDataOpaqueBlock(int handle);

int getIdamDataOpaqueCount(int handle);

void getIdamErrorModel(int handle, int* model, int* param_n, float* params);

int getIdamErrorType(int handle);

int getIdamDataTypeId(const char* type);

int getIdamDataTypeSize(int type);

void getIdamErrorModel(int handle, int* model, int* param_n, float* params);

int getIdamErrorAsymmetry(int handle);

int getIdamErrorModelId(const char* model);

char* acc_getSyntheticData(int handle);

char* acc_getSyntheticDimData(int handle, int ndim);

void acc_setSyntheticData(int handle, char* data);

void acc_setSyntheticDimData(int handle, int ndim, char* data);

char* getIdamSyntheticData(int handle);

char* getIdamData(int handle);

void getIdamDataTdi(int handle, char* data);

char* getIdamAsymmetricError(int handle, int above);

char* getIdamDataErrLo(int handle);

char* getIdamDataErrHi(int handle);

int getIdamDataErrAsymmetry(int handle);

void acc_setIdamDataErrAsymmetry(int handle, int asymmetry);

void acc_setIdamDataErrType(int handle, int type);

void acc_setIdamDataErrLo(int handle, char* errlo);

char* getIdamDimErrLo(int handle, int ndim);

char* getIdamDimErrHi(int handle, int ndim);

int getIdamDimErrAsymmetry(int handle, int ndim);

void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry);

void acc_setIdamDimErrType(int handle, int ndim, int type);

void acc_setIdamDimErrLo(int handle, int ndim, char* errlo);

char* getIdamError(int handle);

void getIdamDoubleData(int handle, double* fp);

void getIdamFloatData(int handle, float* fp);

void getIdamGenericData(int handle, void* data);

void getIdamFloatAsymmetricError(int handle, int above, float* fp);

void getIdamFloatError(int handle, float* fp);

void getIdamDBlock(int handle, DATA_BLOCK* db);

DATA_BLOCK* getIdamDataBlock(int handle);

char* getIdamDataLabel(int handle);

void getIdamDataLabelTdi(int handle, char* label);

char* getIdamDataUnits(int handle);

void getIdamDataUnitsTdi(int handle, char* units);

char* getIdamDataDesc(int handle);

void getIdamDataDescTdi(int handle, char* desc);

int getIdamDimNum(int handle, int ndim);

int getIdamDimType(int handle, int ndim);

int getIdamDimErrorType(int handle, int ndim);

int getIdamDimErrorAsymmetry(int handle, int ndim);

void getIdamDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params);

char* getIdamSyntheticDimData(int handle, int ndim);

char* getIdamDimData(int handle, int ndim);

char* getIdamDimLabel(int handle, int ndim);

char* getIdamDimUnits(int handle, int ndim);

void getIdamDimLabelTdi(int handle, int ndim, char* label);

void getIdamDimUnitsTdi(int handle, int ndim, char* units);

void getIdamDoubleDimData(int handle, int ndim, double* fp);

void getIdamFloatDimData(int handle, int ndim, float* fp);

void getIdamGenericDimData(int handle, int ndim, void* data);

DIMS* getIdamDimBlock(int handle, int ndim);

char* getIdamDimAsymmetricError(int handle, int ndim, int above);

char* getIdamDimError(int handle, int ndim);

void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp);

void getIdamFloatDimError(int handle, int ndim, float* fp);

DATA_SYSTEM* getIdamDataSystem(int handle);

SYSTEM_CONFIG* getIdamSystemConfig(int handle);

DATA_SOURCE* getIdamDataSource(int handle);

SIGNAL* getIdamSignal(int handle);

SIGNAL_DESC* getIdamSignalDesc(int handle);

char* getIdamFileFormat(int handle);

void initIdamDataBlock(DATA_BLOCK* str);

void initIdamRequestBlock(REQUEST_BLOCK* str);

int idamDataCheckSum(void* data, int data_n, int type);

int getIdamDataCheckSum(int handle);

int getIdamDimDataCheckSum(int handle, int ndim);

void lockIdamThread();

void unlockIdamThread();

void freeIdamThread();

int getIdamThreadLastHandle();

void putIdamThreadLastHandle(int handle);

int getIdamMaxThreadCount();

SERVER_BLOCK getIdamThreadServerBlock();

CLIENT_BLOCK getIdamThreadClientBlock();

void putIdamThreadServerBlock(SERVER_BLOCK *str);

void putIdamThreadClientBlock(CLIENT_BLOCK *str);

int setIdamDataTree(int handle);

// Return a specific data tree

NTREE* getIdamDataTree(int handle);

// Return a user defined data structure definition

USERDEFINEDTYPE* getIdamUserDefinedType(int handle);

USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle);

LOGMALLOCLIST* getIdamLogMallocList(int handle);

NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENT_ACCAPI_C_H

