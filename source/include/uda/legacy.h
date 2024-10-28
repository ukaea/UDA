#pragma once

#ifndef UDA_LEGACY_H
#define UDA_LEGACY_H

// to delete just used for debugging...
// #include "client.h"
// #include "clientAPI.h"
// #include "accAPI.h"
// #include "udaGetAPI.h"
// #include "udaPutAPI.h"
// #include "udaPlugin.h"
// #include "clientMDS.h"
// #include "accessors.h"

#warning "using UDA legacy name mappings"

// TODO: address embedded todos...
// TODO: should these all be C functions instead of c++?
//  - change blah() to blah(void)
//  - are inline functions in header bad?

/*
 * definitions from pluginStructs.h
 */

#define UDA_PLUGIN_INTERFACE UDA_PLUGIN_INTERFACE

/*
 * definitions from udaGetAPI.h
 */
inline int idamGetAPI(const char* data_object, const char* data_source)
{
    return udaGetAPI(data_object, data_source);
}

inline int idamGetBatchAPI(const char** uda_signals, const char** sources, int count, int* handles)
{
    return udaGetBatchAPI(uda_signals, sources, count, handles);
}

inline int idamGetAPIWithHost(const char* data_object, const char* data_source, const char* host, int port)
{
    return udaGetAPIWithHost(data_object, data_source, host, port);
}

inline int idamGetBatchAPIWithHost(const char** uda_signals, const char** sources, int count, int* handles,
                                   const char* host, int port)
{
    return udaGetBatchAPIWithHost(uda_signals, sources, count, handles, host, port);
}

/*
 * definitions from udaPutAPI.h*
 */

inline int idamPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList)
{
    return udaPutListAPI(putInstruction, inPutDataBlockList);
}

inline int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData)
{
    return udaPutAPI(putInstruction, inPutData);
}

/*
 * definitions from client.h
 */

inline void idamFree(int handle)
{
    return udaFree(handle);
}

inline void idamFreeAll()
{
    return udaFreeAll();
}

inline const char* idamGetBuildDate()
{
    return udaGetBuildDate();
}

inline const char* getIdamServerHost()
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

inline const char* getIdamClientDOI()
{
    return udaGetClientDOI();
}

inline const char* getIdamServerDOI()
{
    return udaGetServerDOI();
}

inline const char* getIdamClientOSName()
{
    return udaGetClientOSName();
}

inline const char* getIdamServerOSName()
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

inline const char* getIdamServerErrorMsg()
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

inline const char* getIdamServerErrorStackRecordLocation(int record)
{
    return udaGetServerErrorStackRecordLocation(record);
}

inline const char* getIdamServerErrorStackRecordMsg(int record)
{
    return udaGetServerErrorStackRecordMsg(record);
}

// TODO: where did these come from?

#  define setUserDefinedTypeList udaSetUserDefinedTypeList
#  define setLogMallocList udaSetLogMallocList

/*
 * definitions from clientMDS.h
 */

inline int idamClientMDS(const char* server, const char* tree, const char* node, int treenum)
{
    return udaClientMDS(server, tree, node, treenum);
}

/*
 * definitions from accAPI.h
 */

/*
 * NO IDAM KEYWORD
 */
// LIBRARY_API UDA_ERROR_STACK* getUdaServerErrorStack();

inline DATA_BLOCK* acc_getCurrentDataBlock()
{
    return udaGetCurrentDataBlock();
}

inline int acc_getCurrentDataBlockIndex()
{
    return udaGetCurrentDataBlockIndex();
}

inline void acc_free_data_blocks()
{
    return udaFreeDataBlocks();
}

inline char* acc_getSyntheticData(int handle)
{
    return udaGetSyntheticData(handle);
    ;
}

inline char* acc_getSyntheticDimData(int handle, int ndim)
{
    return udaGetSyntheticDimData(handle, ndim);
}

inline void acc_setSyntheticData(int handle, char* data)
{
    return udaSetSyntheticData(handle, data);
}

inline void acc_setSyntheticDimData(int handle, int ndim, char* data)
{
    return udaSetSyntheticDimData(handle, ndim, data);
}

inline int acc_growIdamDataBlocks()
{
    return udaGrowDataBlocks();
}

inline int acc_getIdamNewDataHandle()
{
    return udaGetNewDataHandle();
}

inline void setIdamPrivateFlag(unsigned int flag)
{
    return udaSetPrivateFlag(flag);
}

inline void resetIdamPrivateFlag(unsigned int flag)
{
    return udaResetPrivateFlag(flag);
}

inline void setIdamClientFlag(unsigned int flag)
{
    return udaSetClientFlag(flag);
}

inline void resetIdamClientFlag(unsigned int flag)
{
    return udaResetClientFlag(flag);
}

inline void setIdamProperty(const char* property)
{
    return udaSetProperty(property);
}

inline int getIdamProperty(const char* property)
{
    return udaGetProperty(property);
}

inline void resetIdamProperty(const char* property)
{
    return udaResetProperty(property);
}

inline void resetIdamProperties()
{
    return udaResetProperties();
}

inline CLIENT_BLOCK saveIdamProperties()
{
    return udaSaveProperties();
}

inline void restoreIdamProperties(CLIENT_BLOCK cb)
{
    return udaRestoreProperties(cb);
}

inline CLIENT_BLOCK* getIdamProperties(int handle)
{
    return udaGetProperties(handle);
}

inline CLIENT_BLOCK* getIdamDataProperties(int handle)
{
    return udaGetDataProperties(handle);
}

#  ifndef __APPLE__

inline int getIdamMemoryFree()
{
    return udaGetMemoryFree();
}

inline int getIdamMemoryUsed()
{
    return udaGetMemoryUsed();
}

#  endif

inline void putIdamErrorModel(int handle, int model, int param_n, const float* params)
{
    return udaPutErrorModel(handle, model, param_n, params);
}

inline void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, const float* params)
{
    return udaPutDimErrorModel(handle, ndim, model, param_n, params);
}

inline void putIdamServer(const char* host, int port)
{
    return udaPutServer(host, port);
}

inline void putIdamServerHost(const char* host)
{
    return udaPutServerHost(host);
}

inline void putIdamServerPort(int port)
{
    return udaPutServerPort(port);
}

inline void putIdamServerSocket(int socket)
{
    return udaPutServerSocket(socket);
}

inline void getIdamServer(const char** host, int* port, int* socket)
{
    return udaGetServer(host, port, socket);
}

inline int getIdamErrorCode(int handle)
{
    return udaGetErrorCode(handle);
}

inline const char* getIdamErrorMsg(int handle)
{
    return udaGetErrorMsg(handle);
}

inline int getIdamSourceStatus(int handle)
{
    return udaGetSourceStatus(handle);
}

inline int getIdamSignalStatus(int handle)
{
    return udaGetSignalStatus(handle);
}

inline int getIdamDataStatus(int handle)
{
    return udaGetDataStatus(handle);
}

inline int getIdamLastHandle()
{
    return udaGetLastHandle();
}

inline int getIdamDataNum(int handle)
{
    return udaGetDataNum(handle);
}

inline int getIdamRank(int handle)
{
    return udaGetRank(handle);
}

inline int getIdamOrder(int handle)
{
    return udaGetOrder(handle);
}

inline unsigned int getIdamCachePermission(int handle)
{
    return udaGetCachePermission(handle);
}

inline unsigned int getIdamTotalDataBlockSize(int handle)
{
    return udaGetTotalDataBlockSize(handle);
}

inline int getIdamDataType(int handle)
{
    return udaGetDataType(handle);
}

inline int getIdamDataOpaqueType(int handle)
{
    return udaGetDataOpaqueType(handle);
}

inline void* getIdamDataOpaqueBlock(int handle)
{
    return udaGetDataOpaqueBlock(handle);
}

inline int getIdamDataOpaqueCount(int handle)
{
    return udaGetDataOpaqueCount(handle);
}

inline void getIdamErrorModel(int handle, int* model, int* param_n, float* params)
{
    return udaGetErrorModel(handle, model, param_n, params);
}

inline int getIdamErrorType(int handle)
{
    return udaGetErrorType(handle);
}

inline int getIdamDataTypeId(const char* type)
{
    return udaGetDataTypeId(type);
}

inline int getIdamDataTypeSize(int type)
{
    return udaGetDataTypeSize(type);
}

/*
 * redefinition ?
 */

// TODO check if something was lost in renaming here?

// inline void getIdamErrorModel(int handle, int* model, int* param_n, float* params)
// {
//     return udaGetErrorModel(handle, model, param_n, params);
// }

inline int getIdamErrorAsymmetry(int handle)
{
    return udaGetErrorAsymmetry(handle);
}

inline int getIdamErrorModelId(const char* model)
{
    return udaGetErrorModelId(model);
}

inline char* getIdamSyntheticData(int handle)
{
    return udaGetSyntheticData(handle);
}

inline char* getIdamData(int handle)
{
    return udaGetData(handle);
}

inline void getIdamDataTdi(int handle, char* data)
{
    return udaGetDataTdi(handle, data);
}

inline char* getIdamAsymmetricError(int handle, int above)
{
    return udaGetAsymmetricError(handle, above);
}

inline char* getIdamDataErrLo(int handle)
{
    return udaGetDataErrLo(handle);
}

inline char* getIdamDataErrHi(int handle)
{
    return udaGetDataErrHi(handle);
}

inline int getIdamDataErrAsymmetry(int handle)
{
    return udaGetDataErrAsymmetry(handle);
}

inline void acc_setIdamDataErrAsymmetry(int handle, int asymmetry)
{
    return udaSetDataErrAsymmetry(handle, asymmetry);
}

inline void acc_setIdamDataErrType(int handle, int type)
{
    return udaSetDataErrType(handle, type);
}

inline void acc_setIdamDataErrLo(int handle, char* errlo)
{
    return udaSetDataErrLo(handle, errlo);
}

inline char* getIdamDimErrLo(int handle, int ndim)
{
    return udaGetDimErrLo(handle, ndim);
}

inline char* getIdamDimErrHi(int handle, int ndim)
{
    return udaGetDimErrHi(handle, ndim);
}

inline int getIdamDimErrAsymmetry(int handle, int ndim)
{
    return udaGetDimErrAsymmetry(handle, ndim);
}

inline void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry)
{
    return udaSetDimErrAsymmetry(handle, ndim, asymmetry);
}

inline void acc_setIdamDimErrType(int handle, int ndim, int type)
{
    return udaSetDimErrType(handle, ndim, type);
}

inline void acc_setIdamDimErrLo(int handle, int ndim, char* errlo)
{
    return udaSetDimErrLo(handle, ndim, errlo);
}

inline char* getIdamError(int handle)
{
    return udaGetError(handle);
}

inline void getIdamDoubleData(int handle, double* fp)
{
    return udaGetDoubleData(handle, fp);
}

inline void getIdamFloatData(int handle, float* fp)
{
    return udaGetFloatData(handle, fp);
}

inline void getIdamGenericData(int handle, void* data)
{
    return udaGetGenericData(handle, data);
}

inline void getIdamFloatAsymmetricError(int handle, int above, float* fp)
{
    return udaGetFloatAsymmetricError(handle, above, fp);
}

inline void getIdamFloatError(int handle, float* fp)
{
    return udaGetFloatError(handle, fp);
}

inline void getIdamDBlock(int handle, DATA_BLOCK* db)
{
    return udaGetDBlock(handle, db);
}

inline DATA_BLOCK* getIdamDataBlock(int handle)
{
    return udaGetDataBlock(handle);
}

inline const char* getIdamDataLabel(int handle)
{
    return udaGetDataLabel(handle);
}

inline void getIdamDataLabelTdi(int handle, char* label)
{
    return udaGetDataLabelTdi(handle, label);
}

inline const char* getIdamDataUnits(int handle)
{
    return udaGetDataUnits(handle);
}

inline void getIdamDataUnitsTdi(int handle, char* units)
{
    return udaGetDataUnitsTdi(handle, units);
}

inline const char* getIdamDataDesc(int handle)
{
    return udaGetDataDesc(handle);
}

inline void getIdamDataDescTdi(int handle, char* desc)
{
    return udaGetDataDescTdi(handle, desc);
}

inline int getIdamDimNum(int handle, int ndim)
{
    return udaGetDimNum(handle, ndim);
}

inline int getIdamDimType(int handle, int ndim)
{
    return udaGetDimType(handle, ndim);
}

inline int getIdamDimErrorType(int handle, int ndim)
{
    return udaGetDimErrorType(handle, ndim);
}

inline int getIdamDimErrorAsymmetry(int handle, int ndim)
{
    return udaGetDimErrorAsymmetry(handle, ndim);
}

inline void getIdamDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params)
{
    return udaGetDimErrorModel(handle, ndim, model, param_n, params);
}

inline char* getIdamSyntheticDimData(int handle, int ndim)
{
    return udaGetSyntheticDimData(handle, ndim);
}

inline char* getIdamDimData(int handle, int ndim)
{
    return udaGetDimData(handle, ndim);
}

inline const char* getIdamDimLabel(int handle, int ndim)
{
    return udaGetDimLabel(handle, ndim);
}

inline const char* getIdamDimUnits(int handle, int ndim)
{
    return udaGetDimUnits(handle, ndim);
}

inline void getIdamDimLabelTdi(int handle, int ndim, char* label)
{
    return udaGetDimLabelTdi(handle, ndim, label);
}

inline void getIdamDimUnitsTdi(int handle, int ndim, char* units)
{
    return udaGetDimUnitsTdi(handle, ndim, units);
}

inline void getIdamDoubleDimData(int handle, int ndim, double* fp)
{
    return udaGetDoubleDimData(handle, ndim, fp);
}

inline void getIdamFloatDimData(int handle, int ndim, float* fp)
{
    return udaGetFloatDimData(handle, ndim, fp);
}

inline void getIdamGenericDimData(int handle, int ndim, void* data)
{
    return udaGetGenericDimData(handle, ndim, data);
}

inline DIMS* getIdamDimBlock(int handle, int ndim)
{
    return udaGetDimBlock(handle, ndim);
}

inline char* getIdamDimAsymmetricError(int handle, int ndim, int above)
{
    return udaGetDimAsymmetricError(handle, ndim, above);
}

inline char* getIdamDimError(int handle, int ndim)
{
    return udaGetDimError(handle, ndim);
}

inline void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp)
{
    return udaGetFloatDimAsymmetricError(handle, ndim, above, fp);
}

inline void getIdamFloatDimError(int handle, int ndim, float* fp)
{
    return udaGetFloatDimError(handle, ndim, fp);
}

inline DATA_SYSTEM* getIdamDataSystem(int handle)
{
    return udaGetDataSystem(handle);
}

inline SystemConfig* getIdamSystemConfig(int handle)
{
    return udaGetSystemConfig(handle);
}

inline DataSource* getIdamDataSource(int handle)
{
    return udaGetDataSource(handle);
}

inline Signal* getIdamSignal(int handle)
{
    return udaGetSignal(handle);
}

inline SignalDesc* getIdamSignalDesc(int handle)
{
    return udaGetSignalDesc(handle);
}

inline const char* getIdamFileFormat(int handle)
{
    return udaGetFileFormat(handle);
}

inline void initIdamDataBlock(DATA_BLOCK* str)
{
    return init_data_block(str);
}

inline void initIdamRequestBlock(REQUEST_BLOCK* str)
{
    return init_request_block(str);
}

/*
 * DELETE ?
 */
inline int idamDataCheckSum(void* data, int data_n, int type)
{
    return udaDataCheckSum(data, data_n, type);
}

inline int getIdamDataCheckSum(int handle)
{
    return udaGetDataCheckSum(handle);
}

inline int getIdamDimDataCheckSum(int handle, int ndim)
{
    return udaGetDimDataCheckSum(handle, ndim);
}

inline void lockIdamThread()
{
    return udaLockThread();
}

inline void unlockIdamThread()
{
    return udaUnlockThread();
}

inline void freeIdamThread()
{
    return udaFreeThread();
}

inline int getIdamThreadLastHandle()
{
    return udaGetThreadLastHandle();
}

inline void putIdamThreadLastHandle(int handle)
{
    return udaPutThreadLastHandle(handle);
}

inline int getIdamMaxThreadCount()
{
    return udaGetMaxThreadCount();
}

inline void putIdamThreadServerBlock(ServerBlock* str)
{
    return udaPutThreadServerBlock(str);
}

inline void putIdamThreadClientBlock(CLIENT_BLOCK* str)
{
    return udaPutThreadClientBlock(str);
}

inline int setIdamDataTree(int handle)
{
    return udaSetDataTree(handle);
}

inline NTREE* getIdamDataTree(int handle)
{
    return udaGetDataTree(handle);
}

inline USERDEFINEDTYPE* getIdamUserDefinedType(int handle)
{
    return udaGetUserDefinedType(handle);
}

inline USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle)
{
    return udaGetUserDefinedTypeList(handle);
}

inline LOGMALLOCLIST* getIdamLogMallocList(int handle)
{
    return udaGetLogMallocList(handle);
}

inline NTREE* udaFindIdamNTreeStructureDefinition(NTREE* node, const char* target)
{
    return findNTreeStructureDefinition(node, target);
}

/*
 * definitions from clientAPI.h
 */

inline int idamClientAPI(const char* file, const char* signal, int pass, int exp_number)
{
    return udaClientAPI(file, signal, pass, exp_number);
}

inline int idamClientFileAPI(const char* file, const char* signal, const char* format)
{
    return udaClientFileAPI(file, signal, format);
}

inline int idamClientFileAPI2(const char* file, const char* format, const char* owner, const char* signal,
                              int exp_number, int pass)
{
    return udaClientFileAPI2(file, format, owner, signal, exp_number, pass);
}

inline int idamClientTestAPI(const char* file, const char* signal, int pass, int exp_number)
{
    return udaClientTestAPI(file, signal, pass, exp_number);
}

/*
 * definitions from accessors.h
 */

// TODO: these are used by plugins and wrappers only so don't need to be shadowed?

// LIBRARY_API NTREE* findNTreeStructureComponent2(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
//                                                 const char** lastname);
// LIBRARY_API NTREE* findNTreeStructure2(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
//                                        const char** lastname);
// LIBRARY_API NTREE* findNTreeStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
// LIBRARY_API NTREE* findNTreeChildStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
// LIBRARY_API NTREE* findNTreeStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
// LIBRARY_API NTREE* findNTreeChildStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);
// LIBRARY_API NTREE* findNTreeStructureMalloc(NTREE* ntree, void* data);
// LIBRARY_API NTREE* findNTreeStructureDefinition(NTREE* ntree, const char* target);
// LIBRARY_API NTREE* findNTreeStructureComponentDefinition(NTREE* tree, const char* target);

inline NTREE* idam_findNTreeStructureClass(NTREE* tree, int cls)
{
    return udaFindNTreeStructureClass(tree, cls);
}

inline int idam_maxCountVlenStructureArray(NTREE* tree, const char* target, int reset)
{
    return udaMaxCountVlenStructureArray(tree, target, reset);
}

inline int idam_regulariseVlenStructures(LOGMALLOCLIST* logmalloclist, NTREE* tree,
                                         USERDEFINEDTYPELIST* userdefinedtypelist, const char* target,
                                         unsigned int count)
{
    return udaRegulariseVlenStructures(LOGMALLOCLIST * logmalloclist, NTREE * tree,
                                       USERDEFINEDTYPELIST * userdefinedtypelist, const char* target,
                                       unsigned int count);
}

inline int idam_regulariseVlenData(LOGMALLOCLIST* logmalloclist, NTREE* tree, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    return udaRegulariseVlenData(LOGMALLOCLIST * logmalloclist, NTREE * tree,
                                 USERDEFINEDTYPELIST * userdefinedtypelist);
}
//
// LIBRARY_API int getNodeStructureDataCount(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
// LIBRARY_API int getNodeStructureDataSize(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
// LIBRARY_API int getNodeStructureDataRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
// LIBRARY_API const char* getNodeStructureDataDataType(LOGMALLOCLIST* logmalloclist, NTREE* ntree);
// LIBRARY_API void* getNodeStructureData(NTREE* ntree);
// LIBRARY_API void printImage(const char* image, int imagecount);
// LIBRARY_API void defineField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset,
//                              unsigned short type_id);
// LIBRARY_API void defineCompoundField(COMPOUNDFIELD* field, const char* type, const char* name, char* desc, int
// offset,
//                                      int size);

/*
 * definitions from errorLog.h
 */

// inline int udaNumErrors()
// {
//     return udaNumErrors();
// }

// inline void udaErrorLog(CLIENT_BLOCK client_block, REQUEST_BLOCK request_block, UDA_ERROR_STACK* error_stack)
// {
//     return udaErrorLog(client_block, request_block, error_stack);
// }

inline void initUdaErrorStack()
{
    return initErrorStack();
}

inline void initErrorRecords(const UDA_ERROR_STACK* errorstack)
{
    return initErrorRecords(errorstack);
}

inline void printIdamErrorStack()
{
    return udaPrintErrorStack();
}

inline void addIdamError(int type, const char* location, int code, const char* msg)
{
    return add_error(type, location, code, msg);
}

inline void concatUdaError(UDA_ERROR_STACK* errorstackout)
{
    return udaConcatError(errorstackout);
}

inline void freeIdamErrorStack(UDA_ERROR_STACK* errorstack)
{
    return udaFreeErrorStack(errorstack);
}

inline void closeUdaError()
{
    return udaCloseError();
}

#endif // UDA_LEGACY_H
