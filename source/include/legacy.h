#pragma once

#ifndef UDA_LEGACY_H
#  define UDA_LEGACY_H

#  warning "using UDA legacy name mappings"

/*
 * definitions from udaGetAPI.h
 */
inline int idamGetAPI(const char* data_object, const char* data_source)
{
    return udaGetAPI(data_object, data_source);
}

int idamGetBatchAPI(const char** uda_signals, const char** sources, int count, int* handles)
{
    return udaGetBatchAPI(uda_signals, sources, count, handles);
}

int idamGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port)
{
    return udaGetAPIWithHost(data_object, data_source, host, port);
}

int idamGetBatchAPIWithHost(const char** uda_signals, const char** sources, int count, int* handles, const char* host,
                            int port)
{
    return udGaetBatchAPIWithHost(uda_signals, sources, count, handles, host, port);
}

/*
 * definitions from client.h
 */

inline void idamFree(int handle)
{
    udaFree();
}

inline void idamFreeAll()
{
    udaFreeAll();
}

inline const char* idamGetBuildVersion()
{
    return udaGetBuildVersion();
}

inline const char* idamGetBuildDate()
{
    return udaGetBuildDate();
}

inline char* getIdamServerHost()
{
    return udaGetServerHost();
}
inline int getIdamServerPort()
{
    return udaGetServerPort();
}

inline int getIdamServerSocket()
{
    return udaGetServerSocket();
}

inline char* getIdamClientDOI()
{
    return udaGetClientDOI();
}

inline char* getIdamServerDOI()
{
    return udaGetServerDOI();
}

inline char* getIdamClientOSName()
{
    return udaGetClientOSName();
}

inline char* getIdamServerOSName()
{
    return udaGetServerOSName();
}

inline int getIdamClientVersion()
{
    return udaGetClientVersion();
}

inline int getIdamServerVersion()
{
    return udaGetServerVersion();
}

inline int getIdamServerErrorCode()
{
    return udaGetServerErrorCode();
}

inline char* getIdamServerErrorMsg()
{
    return udaGetServerErrorMsg();
}

inline int getIdamServerErrorStackSize()
{
    return udaGetServerErrorStackSize();
}

inline int getIdamServerErrorStackRecordType(int record)
{
    return udaGetServerErrorStackRecordType(record);
}

inline int getIdamServerErrorStackRecordCode(int record)
{
    return udaGetServerErrorStackRecordCode(record);
}

inline char* getIdamServerErrorStackRecordLocation(int record)
{
    return udaGetServerErrorStackRecordLocation(record);
}

inline char* getIdamServerErrorStackRecordMsg(int record)
{
    return udaGetServerErrorStackRecordMsg(record);
}

#  define setUserDefinedTypeList udaSetUserDefinedTypeList
#  define setLogMallocList udaSetLogMallocList

/*
 * defines from accAPI.h
 */

/*
 * NO IDAM KEYWORD
 */
// LIBRARY_API DATA_BLOCK* acc_getCurrentDataBlock(CLIENT_FLAGS* client_flags);
// LIBRARY_API int acc_getCurrentDataBlockIndex(CLIENT_FLAGS* client_flags);
// LIBRARY_API void acc_freeDataBlocks();
// LIBRARY_API UDA_ERROR_STACK* getUdaServerErrorStack();
// LIBRARY_API char* acc_getSyntheticData(int handle);
// LIBRARY_API char* acc_getSyntheticDimData(int handle, int ndim);
// LIBRARY_API void acc_setSyntheticData(int handle, char* data);
// LIBRARY_API void acc_setSyntheticDimData(int handle, int ndim, char* data);

inline int acc_growIdamDataBlocks()
{
    return acc_growUdaDataBlocks(); 
}

inline int acc_getIdamNewDataHandle()
{
    return acc_getUdaNewDataHandle(); 
}


inline void setIdamPrivateFlag(unsigned int flag)
{
    return setUdaPrivateFlag(flag); 
}

inline void resetIdamPrivateFlag(unsigned int flag)
{
    return resetUdaPrivateFlag(flag); 
}

inline void setIdamClientFlag(unsigned int flag)
{
    return setUdaClientFlag(unsigned int flag)
}

inline void resetIdamClientFlag(unsigned int flag)
{
    return resetUdaClientFlag(unsigned int flag)
}

inline void setIdamProperty(const char* property)
{
    return setUdaProperty(const char* property)
}

inline int getIdamProperty(const char* property)
{
    return getUdaProperty(const char* property)
}

inline void resetIdamProperty(const char* property)
{
    return resetUdaProperty(const char* property)
}

inline void resetIdamProperties()
{
    return resetUdaProperties(); 
}

inline CLIENT_BLOCK saveIdamProperties()
{
    return saveUdaProperties(); 
}

inline void restoreIdamProperties(CLIENT_BLOCK cb)
{
    return restoreUdaProperties(CLIENT_BLOCK cb)
}

inline CLIENT_BLOCK* getIdamProperties(int handle)
{
    return getUdaProperties(handle); 
}

inline CLIENT_BLOCK* getIdamDataProperties(int handle)
{
    return getUdaDataProperties(handle); 
}

#ifndef __APPLE__

inline int getIdamMemoryFree()
{
    return getUdaMemoryFree()
}

inline int getIdamMemoryUsed()
{
    return getUdaMemoryUsed()
}

#endif

inline void putIdamErrorModel(int handle, int model, int param_n, const float* params)
{
    return putUdaErrorModel(handle, model, param_n, params)
}

inline void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, const float* params)
{
    return putUdaDimErrorModel(handle, ndim, model, param_n, params)
}

inline void putIdamServer(const char* host, int port)
{
    return putUdaServer(host, port)
}

inline void putIdamServerHost(const char* host)
{
    return putUdaServerHost(host); 
}

inline void putIdamServerPort(int port)
{
    return putUdaServerPort(port); 
}

inline void putIdamServerSocket(int socket)
{
    return putUdaServerSocket(socket); 
}

inline void getIdamServer(const char** host, int* port, int* socket)
{
    return getUdaServer(host, port, socket)
}

inline int getIdamErrorCode(int handle)
{
    return getUdaErrorCode(handle); 
}

inline const char* getIdamErrorMsg(int handle)
{
    return getUdaErrorMsg(handle); 
}

inline int getIdamSourceStatus(int handle)
{
    return getUdaSourceStatus(handle); 
}

inline int getIdamSignalStatus(int handle)
{
    return getUdaSignalStatus(handle); 
}

inline int getIdamDataStatus(int handle)
{
    return getUdaDataStatus(handle); 
}

inline int getIdamLastHandle()
{
    return getUdaLastHandle(); 
}

inline int getIdamDataNum(int handle)
{
    return getUdaDataNum(handle); 
}

inline int getIdamRank(int handle)
{
    return getUdaRank(handle); 
}

inline int getIdamOrder(int handle)
{
    return getUdaOrder(handle); 
}

inline unsigned int getIdamCachePermission(int handle)
{
    return getUdaCachePermission(handle); 
}

inline unsigned int getIdamTotalDataBlockSize(int handle)
{
    return getUdaTotalDataBlockSize(handle); 
}

inline int getIdamDataType(int handle)
{
    return getUdaDataType(handle); 
}

inline int getIdamDataOpaqueType(int handle)
{
    return getUdaDataOpaqueType(handle); 
}

inline void* getIdamDataOpaqueBlock(int handle)
{
    return getUdaDataOpaqueBlock(handle); 
}

inline int getIdamDataOpaqueCount(int handle)
{
    return getUdaDataOpaqueCount(handle); 
}

inline void getIdamErrorModel(int handle, int* model, int* param_n, float* params)
{
    return getUdaErrorModel(handle, model, param_n, params)
}

inline int getIdamErrorType(int handle)
{
    return getUdaErrorType(handle); 
}

inline int getIdamDataTypeId(const char* type)
{
    return getUdaDataTypeId(type); 
}

inline int getIdamDataTypeSize(int type)
{
    return getUdaDataTypeSize(type); 
}

inline void getIdamErrorModel(int handle, int* model, int* param_n, float* params)
{
    return getUdaErrorModel(handle, model, param_n, params)
}

inline int getIdamErrorAsymmetry(int handle)
{
    return getUdaErrorAsymmetry(handle); 
}

inline int getIdamErrorModelId(const char* model)
{
    return getUdaErrorModelId(model); 
}

inline char* getIdamSyntheticData(int handle)
{
    return getUdaSyntheticData(handle); 
}

inline char* getIdamData(int handle)
{
    return getUdaData(handle); 
}

inline void getIdamDataTdi(int handle, char* data)
{
    return getUdaDataTdi(handle, data)
}

inline char* getIdamAsymmetricError(int handle, int above)
{
    return getUdaAsymmetricError(handle, above)
}

inline char* getIdamDataErrLo(int handle)
{
    return getUdaDataErrLo(handle); 
}

inline char* getIdamDataErrHi(int handle)
{
    return getUdaDataErrHi(handle); 
}

inline int getIdamDataErrAsymmetry(int handle)
{
    return getUdaDataErrAsymmetry(handle); 
}

inline void acc_setIdamDataErrAsymmetry(int handle, int asymmetry)
{
    return acc_setUdaDataErrAsymmetry(handle, asymmetry)
}

inline void acc_setIdamDataErrType(int handle, int type)
{
    return acc_setUdaDataErrType(handle, type)
}

inline void acc_setIdamDataErrLo(int handle, char* errlo)
{
    return acc_setUdaDataErrLo(handle, errlo)
}

inline char* getIdamDimErrLo(int handle, int ndim)
{
    return getUdaDimErrLo(handle, ndim)
}

inline char* getIdamDimErrHi(int handle, int ndim)
{
    return getUdaDimErrHi(handle, ndim)
}

inline int getIdamDimErrAsymmetry(int handle, int ndim)
{
    return getUdaDimErrAsymmetry(int handle, int ndim)
}

inline void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry)
{
    return acc_setUdaDimErrAsymmetry(int handle, int ndim, int asymmetry)
}

inline void acc_setIdamDimErrType(int handle, int ndim, int type)
{
    return acc_setUdaDimErrType(int handle, int ndim, int type)
}

inline void acc_setIdamDimErrLo(int handle, int ndim, char* errlo)
{
    return acc_setUdaDimErrLo(int handle, int ndim, char* errlo)
}

inline char* getIdamError(int handle)
{
    return getUdaError(handle); 
}

inline void getIdamDoubleData(int handle, double* fp)
{
    return getUdaDoubleData(handle, fp)
}

inline void getIdamFloatData(int handle, float* fp)
{
    return getUdaFloatData(handle, fp)
}

inline void getIdamGenericData(int handle, void* data)
{
    return getUdaGenericData(handle, data)
}

inline void getIdamFloatAsymmetricError(int handle, int above, float* fp)
{
    return getUdaFloatAsymmetricError(int handle, int above, float* fp)
}

inline void getIdamFloatError(int handle, float* fp)
{
    return getUdaFloatError(handle, fp)
}

inline void getIdamDBlock(int handle, DATA_BLOCK* db)
{
    return getUdaDBlock(handle, db)
}

inline DATA_BLOCK* getIdamDataBlock(int handle)
{
    return getUdaDataBlock(handle); 
}

inline const char* getIdamDataLabel(int handle)
{
    return getUdaDataLabel(handle); 
}

inline void getIdamDataLabelTdi(int handle, char* label)
{
    return getUdaDataLabelTdi(handle, label)
}

inline const char* getIdamDataUnits(int handle)
{
    return getUdaDataUnits(handle); 
}

inline void getIdamDataUnitsTdi(int handle, char* units)
{
    return getUdaDataUnitsTdi(handle, units)
}

inline const char* getIdamDataDesc(int handle)
{
    return getUdaDataDesc(handle); 
}

inline void getIdamDataDescTdi(int handle, char* desc)
{
    return getUdaDataDescTdi(handle, desc)
}

inline int getIdamDimNum(int handle, int ndim)
{
    return getUdaDimNum(handle, ndim)
}

inline int getIdamDimType(int handle, int ndim)
{
    return getUdaDimType(handle, ndim)
}

inline int getIdamDimErrorType(int handle, int ndim)
{
    return getUdaDimErrorType(handle, ndim)
}

inline int getIdamDimErrorAsymmetry(int handle, int ndim)
{
    return getUdaDimErrorAsymmetry(handle, ndim)
}

inline void getIdamDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params)
{
    return getUdaDimErrorModel(handle, ndim, model, param_n, params)
}

inline char* getIdamSyntheticDimData(int handle, int ndim)
{
    return getUdaSyntheticDimData(handle, ndim)
}

inline char* getIdamDimData(int handle, int ndim)
{
    return getUdaDimData(handle, ndim)
}

inline const char* getIdamDimLabel(int handle, int ndim)
{
    return getUdaDimLabel(handle, ndim)
}

inline const char* getIdamDimUnits(int handle, int ndim)
{
    return getUdaDimUnits(handle, ndim)
}

inline void getIdamDimLabelTdi(int handle, int ndim, char* label)
{
    return getUdaDimLabelTdi(handle, ndim, label)
}

inline void getIdamDimUnitsTdi(int handle, int ndim, char* units)
{
    return getUdaDimUnitsTdi(handle, ndim, units)
}

inline void getIdamDoubleDimData(int handle, int ndim, double* fp)
{
    return getUdaDoubleDimData(handle, ndim, fp)
}

inline void getIdamFloatDimData(int handle, int ndim, float* fp)
{
    return getUdaFloatDimData(handle, ndim, fp)
}

inline void getIdamGenericDimData(int handle, int ndim, void* data)
{
    return getUdaGenericDimData(handle, ndim, data)
}

inline DIMS* getIdamDimBlock(int handle, int ndim)
{
    return getUdaDimBlock(handle, ndim)
}

inline char* getIdamDimAsymmetricError(int handle, int ndim, int above)
{
    return getUdaDimAsymmetricError(handle, ndim, above)
}

inline char* getIdamDimError(int handle, int ndim)
{
    return getUdaDimError(handle, ndim)
}

inline void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp)
{
    return getUdaFloatDimAsymmetricError(handle, ndim, above, fp)
}

inline void getIdamFloatDimError(int handle, int ndim, float* fp)
{
    return getUdaFloatDimError(handle, ndim, fp)
}

inline DATA_SYSTEM* getIdamDataSystem(int handle)
{
    return getUdaDataSystem(handle); 
}

inline SYSTEM_CONFIG* getIdamSystemConfig(int handle)
{
    return getUdaSystemConfig(handle); 
}

inline DATA_SOURCE* getIdamDataSource(int handle)
{
    return getUdaDataSource(handle); 
}

inline SIGNAL* getIdamSignal(int handle)
{
    return getUdaSignal(handle); 
}

inline SIGNAL_DESC* getIdamSignalDesc(int handle)
{
    return getUdaSignalDesc(handle); 
}

inline const char* getIdamFileFormat(int handle)
{
    return getUdaFileFormat(handle); 
}

inline void initIdamDataBlock(DATA_BLOCK* str)
{
    return initUdaDataBlock(str); 
}

inline void initIdamRequestBlock(REQUEST_BLOCK* str)
{
    return initUdaRequestBlock(str); 
}

/*
 * DELETE
 * /
inline int idamDataCheckSum(void* data, int data_n, int type)
{
    udaDataCheckSum(data, data_n, type);
}

inline int getIdamDataCheckSum(int handle)
{
    return getUdaDataCheckSum(handle); 
}

inline int getIdamDimDataCheckSum(int handle, int ndim)
{
    return getUdaDimDataCheckSum(handle, ndim)
}

inline void lockIdamThread()
{
    return lockUdaThread(); 
}

inline void unlockIdamThread()
{
    return unlockUdaThread();
]

inline void freeIdamThread()
{
    return freeUdaThread(); 
}

inline int getIdamThreadLastHandle()
{
    return getUdaThreadLastHandle()
}

inline void putIdamThreadLastHandle(int handle)
{
    return putUdaThreadLastHandle(handle); 
}

inline int getIdamMaxThreadCount()
{
    return getUdaMaxThreadCount()
}

inline SERVER_BLOCK getIdamThreadServerBlock()
{
    return getUdaThreadServerBlock()
}

inline CLIENT_BLOCK getIdamThreadClientBlock()
{
    return getUdaThreadClientBlock()
}

inline void putIdamThreadServerBlock(SERVER_BLOCK* str)
{
    return putUdaThreadServerBlock(str); 
}

inline void putIdamThreadClientBlock(CLIENT_BLOCK* str)
{
    return putUdaThreadClientBlock(str); 
}

inline int setIdamDataTree(int handle)
{
    return setUdaDataTree(handle); 
}

// Return a specific data tree

inline NTREE* getIdamDataTree(int handle)
{
    return getUdaDataTree(handle); 
}

// Return a user defined data structure definition

inline USERDEFINEDTYPE* getIdamUserDefinedType(int handle)
{
    return getUdaUserDefinedType(handle); 
}

inline USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle)
{
    return getUdaUserDefinedTypeList(handle); 
}

inline LOGMALLOCLIST* getIdamLogMallocList(int handle)
{
    return getUdaLogMallocList(handle); 
}

inline NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target)
{
    return findUdaNTreeStructureDefinition(node, target)
}


#endif // UDA_LEGACY_H
