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

/**
 * Loads a client configuration from a specified config file.
 *
 * @param config_name The path of the configuration file to load.
 */
LIBRARY_API void udaLoadConfig(const char* config_name);

/**
 * The main UDA API function for data retrieval.
 *
 * Retrieves data from a data source using the specified data signal and source identifiers.
 * Various patterns for data_source are supported including pulse numbers, file paths, and server URLs.
 *
 * @param data_signal Identifies the data signal to be accessed from the source.
 * @param data_source Identifies the location of data (pulse number, file path, etc.).
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaGetAPI(const char* data_signal, const char* data_source);

/**
 * Batch version of the main UDA API function for retrieving multiple signals in a single call.
 *
 * @param data_signals Array of data signal identifiers.
 * @param data_sources Array of data source identifiers (must be the same length as data_signals).
 * @param count The number of signals to retrieve.
 * @param handles Output array to store the returned handles (must be pre-allocated to size count).
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaGetBatchAPI(const char** data_signals, const char** data_sources, int count, int* handles);

/**
 * Extended version of udaGetAPI that allows specifying a custom host and port.
 *
 * @param data_signal Identifies the data signal to be accessed from the source.
 * @param data_source Identifies the location of data (pulse number, file path, etc.).
 * @param host The hostname or IP address of the UDA server (nullptr to use default server).
 * @param port The port number of the UDA server (0 to use default port).
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaGetAPIWithHost(const char* data_signal, const char* data_source, const char* host, int port);

/**
 * Extended version of udaGetBatchAPI that allows specifying a custom host and port.
 *
 * @param data_signals Array of data signal identifiers.
 * @param data_sources Array of data source identifiers (must be the same length as data_signals).
 * @param count The number of signals to retrieve.
 * @param handles Output array to store the returned handles (must be pre-allocated to size count).
 * @param host The hostname or IP address of the UDA server (nullptr to use default server).
 * @param port The port number of the UDA server (0 to use default port).
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaGetBatchAPIWithHost(const char** data_signals, const char** data_sources, int count, int* handles,
                                       const char* host, int port);

/**
 * Makes a request to the server with a data block list to put.
 *
 * @param put_instruction Instruction string specifying the put operation.
 * @param put_data_block_list_in Array (list) of PUTDATA_BLOCK structures containing the data to be written.
 * @param count Number of data blocks in the array.
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaPutListAPI(const char* put_instruction, PUTDATA_BLOCK* put_data_block_list_in, size_t count);

/**
 * Makes a request to the server with single data block to put.
 *
 * @param put_instruction Instruction string specifying the put operation.
 * @param putdata_block_in PUTDATA_BLOCK structure containing the data to be written.
 * @return A reference ID handle used to identify the accessed data in subsequent API calls.
 *         Negative values indicate an error.
 */
LIBRARY_API int udaPutAPI(const char* put_instruction, PUTDATA_BLOCK* putdata_block_in);

/**
 * Creates a new data block structure for use with the put API.
 *
 * @param data_type The UDA data type for the data block.
 * @param count The number of elements in the data array.
 * @param rank The dimensionality of the data (1 for vector, 2 for matrix, etc.).
 * @param shape Array of dimensions, with length equal to rank.
 * @param data Pointer to the data to be included in the block.
 * @return A pointer to newly allocated PUTDATA_BLOCK structure.
 */
LIBRARY_API PUTDATA_BLOCK* udaNewPutDataBlock(UDA_TYPE data_type, int count, int rank, int* shape, const char* data);

/**
 * Frees a PUTDATA_BLOCK structure previously allocated with udaNewPutDataBlock.
 *
 * @param opaque_putdata_block Pointer to the PUTDATA_BLOCK structure to free.
 */
LIBRARY_API void udaFreePutDataBlock(PUTDATA_BLOCK* opaque_putdata_block);

LIBRARY_API void udaFree(int handle);

LIBRARY_API void udaFreeAll();

/**
 * Gets the date when the client library was built.
 *
 * @return String containing the build date.
 */
LIBRARY_API const char* udaGetClientBuildDate();

/**
 * Gets the client library version as a formatted string.
 *
 * @param version_string Pre-allocated char buffer to receive the version string.
 */
LIBRARY_API void udaGetClientVersionString(char* version_string);

/**
 * Gets the client library version as a single integer.
 *
 * @return The client version encoded as a single integer.
 */
LIBRARY_API int udaGetClientVersion();

/**
 * Gets the major version component of the client library.
 *
 * @return The major version number.
 */
LIBRARY_API int udaGetClientVersionMajor();

/**
 * Gets the minor version component of the client library.
 *
 * @return The minor version number.
 */
LIBRARY_API int udaGetClientVersionMinor();

/**
 * Gets the bugfix (patch) version component of the client library.
 *
 * @return The bugfix version number.
 */
LIBRARY_API int udaGetClientVersionBugfix();

/**
 * Gets the delta version component of the client library.
 *
 * @return The delta version number.
 */
LIBRARY_API int udaGetClientVersionDelta();

/**
 * Gets the hostname of the UDA server to be used.
 *
 * @return String containing the server hostname.
 */
LIBRARY_API const char* udaGetServerHost();

/**
 * Gets the port number of the UDA server to be used.
 *
 * @return The server port number.
 */
LIBRARY_API int udaGetServerPort();

/**
 * Gets the server version as a formatted string.
 *
 * @param version_string Pre-allocated char buffer to receive the version string.
 */
LIBRARY_API void udaGetServerVersionString(char* version_string);

/**
 * Gets the server version as a single integer.
 *
 * @return The server version encoded as a single integer.
 */
LIBRARY_API int udaGetServerVersion();

/**
 * Gets the major version component of the server.
 *
 * @return The major version number.
 */
LIBRARY_API int udaGetServerVersionMajor();

/**
 * Gets the minor version component of the server.
 *
 * @return The minor version number.
 */
LIBRARY_API int udaGetServerVersionMinor();

/**
 * Gets the bugfix (patch) version component of the server.
 *
 * @return The bugfix version number.
 */
LIBRARY_API int udaGetServerVersionBugfix();

/**
 * Gets the delta version component of the server.
 *
 * @return The delta version number.
 */
LIBRARY_API int udaGetServerVersionDelta();



LIBRARY_API void udaCloseAllConnections();

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

LIBRARY_API int udaNumErrors();
LIBRARY_API const char* udaGetErrorMessage(int err_num);
LIBRARY_API const char* udaGetError(int handle);
LIBRARY_API int udaGetErrorCode(int handle);


LIBRARY_API void udaPutServer(const char* host, int port);

LIBRARY_API void udaPutServerHost(const char* host);

LIBRARY_API void udaPutServerPort(int port);

LIBRARY_API int udaGetSourceStatus(int handle);

LIBRARY_API int udaGetSignalStatus(int handle);

LIBRARY_API int udaGetLastHandle();


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

LIBRARY_API int udaGetDataTypeId(const char* type);

LIBRARY_API int udaGetDataTypeSize(int type);

LIBRARY_API char* udaGetData(int handle);

LIBRARY_API const char* udaGetDataLabel(int handle);

LIBRARY_API const char* udaGetDataUnits(int handle);

LIBRARY_API const char* udaGetDataDesc(int handle);

LIBRARY_API int udaGetDimNum(int handle, int n_dim);

LIBRARY_API int udaGetDimType(int handle, int n_dim);

LIBRARY_API char* udaGetDimData(int handle, int n_dim);

LIBRARY_API const char* udaGetDimLabel(int handle, int n_dim);

LIBRARY_API const char* udaGetDimUnits(int handle, int n_dim);

LIBRARY_API int udaDataCheckSum(const char* data, int data_n, int type);

LIBRARY_API int udaGetDataCheckSum(int handle);

LIBRARY_API int udaGetDimDataCheckSum(int handle, int n_dim);

LIBRARY_API int udaSetDataTree(int handle);

LIBRARY_API NTREE* udaGetDataTree(int handle);

//////////////////////////////////////////
//////////////////////////////////////////
//////////////////////////////////////////
/// Marked for deletion but used in multiple places or some wrappers
LIBRARY_API int udaGetServerErrorStackSize(); // used in C++ client wrapper

LIBRARY_API int udaGetServerErrorStackRecordType(int record); // used in IDL wrapper

LIBRARY_API int udaGetServerErrorStackRecordCode(int record); // used in both C++ and IDL wrapper

LIBRARY_API const char* udaGetServerErrorStackRecordLocation(int record); // used in both C++ and IDL wrapper

LIBRARY_API const char* udaGetServerErrorStackRecordMsg(int record); // used in C++, IDL wrappers + plugin

LIBRARY_API void udaPutErrorModel(int handle, int model, int param_n, const float* params); // IDL

LIBRARY_API void udaPutDimErrorModel(int handle, int n_dim, int model, int param_n, const float* params); // IDL
// synthetic
LIBRARY_API char* udaGenerateSyntheticDimData(int handle, int n_dim);
LIBRARY_API char* udaGetSyntheticData(int handle);
LIBRARY_API char* udaGetSyntheticDimData(int handle, int n_dim);
LIBRARY_API void udaSetSyntheticData(int handle, char* data);
LIBRARY_API void udaSetSyntheticDimData(int handle, int n_dim, char* data);

LIBRARY_API LOGMALLOCLIST* udaGetLogMallocList(int handle);

LIBRARY_API USERDEFINEDTYPE* udaGetUserDefinedType(int handle);

LIBRARY_API USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);
//////////////////////////////////////////
//////////////////////////////////////////

LIBRARY_API void    udaGetDataErrorModel(int handle, int* model, int* param_n, float* params);
LIBRARY_API int     udaGetDataErrorType(int handle);
LIBRARY_API int     udaGetDataErrorAsymmetry(int handle);
LIBRARY_API int     udaGetDataErrorModelId(const char* model);

LIBRARY_API char*   udaGetDataErrLo(int handle);
LIBRARY_API char*   udaGetDataErrHi(int handle);
LIBRARY_API int     udaGetDataErrAsymmetry(int handle);
LIBRARY_API char*   udaGetDimErrLo(int handle, int n_dim);
LIBRARY_API char*   udaGetDimErrHi(int handle, int n_dim);
LIBRARY_API int     udaGetDimErrAsymmetry(int handle, int n_dim);

LIBRARY_API void    udaSetDataErrAsymmetry(int handle, int asymmetry);
LIBRARY_API void    udaSetDataErrType(int handle, int type);
LIBRARY_API void    udaSetDataErrLo(int handle, char* errlo);
LIBRARY_API void    udaSetDimErrAsymmetry(int handle, int n_dim, int asymmetry);
LIBRARY_API void    udaSetDimErrType(int handle, int n_dim, int type);
LIBRARY_API void    udaSetDimErrLo(int handle, int n_dim, char* errlo);
////////////////////////////

//changed
LIBRARY_API char*   udaGetDataError(int handle, bool above);
LIBRARY_API void    udaGetFloatError(int handle, bool above, float* data);

LIBRARY_API char*   udaGetDimError(int handle, int n_dim, bool above);
LIBRARY_API int     udaGetDimErrorType(int handle, int n_dim);
LIBRARY_API void    udaGetDimErrorModel(int handle, int n_dim, int* model, int* param_n, float* params);
LIBRARY_API void    udaGetFloatDimError(int handle, int n_dim, bool above, float* data);

// rename? bool is Error asymmetric
LIBRARY_API int     udaGetDimErrorAsymmetry(int handle, int n_dim);
//////////////////////////////////////////
//////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H
