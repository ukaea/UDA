#ifndef IDAM_CLIENT_ACCAPI_C_H
#define IDAM_CLIENT_ACCAPI_C_H

#include <stdio.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUMIDAMCLIENTTHREADS 30

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

