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

/**
 * Frees memory associated with a specific data handle.
 *
 * @param handle The data handle to free.
 */
LIBRARY_API void udaFree(int handle);

/**
 * Frees memory associated with all data handles.
 */
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

/**
 * Closes all open server connections.
 */
LIBRARY_API void udaCloseAllConnections();

/**
 * Sets a private flag in the UDA client for server-to-server communication.
 *
 * @param flag The bit or bits to be set to 1.
 */
LIBRARY_API void udaSetPrivateFlag(unsigned int flag);

/**
 * Resets a private flag in the UDA client for server-to-server communication.
 *
 * @param flag The bit or bits to be set to 0.
 */
LIBRARY_API void udaResetPrivateFlag(unsigned int flag);

/**
 * Sets a client flag sent to the UDA server.
 *
 * @param flag The bit or bits to be set to 1.
 */
LIBRARY_API void udaSetClientFlag(unsigned int flag);

/**
 * Resets a client flag sent to the UDA server.
 *
 * @param flag The bit or bits to be set to 0.
 */
LIBRARY_API void udaResetClientFlag(unsigned int flag);

/**
 * Sets a named server property that affects the data type returned and server-side processing.
 *
 * @param property The name of the property to set or a name-value pair.
 */
LIBRARY_API void udaSetProperty(const char* property);

/**
 * Gets the value of a named server property.
 *
 * @param property The name of the property.
 * @return The property value.
 */
LIBRARY_API int udaGetProperty(const char* property);

/**
 * Resets a specific named server property to its default value.
 *
 * @param property The name of the property.
 */
LIBRARY_API void udaResetProperty(const char* property);

/**
 * Resets all server properties to their default values.
 */
LIBRARY_API void udaResetProperties();

#ifndef __APPLE__

/**
 * Gets the amount of free heap memory available.
 *
 * @return The amount of free memory in bytes.
 */
LIBRARY_API int udaGetMemoryFree();

/**
 * Gets the amount of heap memory currently used.
 *
 * @return The amount of memory used in bytes.
 */
LIBRARY_API int udaGetMemoryUsed();

#endif

/**
 * Gets the number of errors in the error stack.
 *
 * @return The number of errors.
 */
LIBRARY_API int udaNumErrors();

/**
 * Gets the error message for a specific error number.
 *
 * @param err_num The error number.
 * @return The error message.
 */
LIBRARY_API const char* udaGetErrorMessage(int err_num);

/**
 * Gets the error message for a specific data handle.
 *
 * @param handle The data handle.
 * @return The error message.
 */
LIBRARY_API const char* udaGetError(int handle);

/**
 * Gets the error code for a specific data handle.
 *
 * @param handle The data handle.
 * @return The error code.
 */
LIBRARY_API int udaGetErrorCode(int handle);

/**
 * Sets the UDA server host name and port number.
 *
 * @param host The hostname or IP address of the UDA server.
 * @param port The port number of the UDA server.
 */
LIBRARY_API void udaPutServer(const char* host, int port);

/**
 * Sets the UDA server host name.
 *
 * @param host The hostname or IP address of the UDA server.
 */
LIBRARY_API void udaPutServerHost(const char* host);

/**
 * Sets the UDA server port number.
 *
 * @param port The port number of the UDA server.
 */
LIBRARY_API void udaPutServerPort(int port);

/**
 * Gets a metadata value for a specific key.
 *
 * @param handle The data handle.
 * @param key The metadata key.
 * @return The metadata value.
 */
LIBRARY_API const char* udaGetMetadata(int handle, const char* key);

/**
 * Gets the number of metadata keys available.
 *
 * @param handle The data handle.
 * @return The number of metadata keys.
 */
LIBRARY_API int udaGetMetadataKeyCount(int handle);

/**
 * Gets a metadata key by index.
 *
 * @param handle The data handle.
 * @param index The index of the key to retrieve.
 * @return The metadata key.
 */
LIBRARY_API const char* udaGetMetadataKey(int handle, int index);

/**
 * Gets the last data handle issued.
 * 
 * @deprecated This function is marked for deprecation.
 * @return The last data handle.
 */
// __attribute ((deprecated))
LIBRARY_API int udaGetLastHandle();

/**
 * Gets the server's permission to locally cache data.
 *
 * @param handle The data handle.
 * @return The cache permission.
 */
LIBRARY_API unsigned int udaGetCachePermission(int handle);

/**
 * Gets the total size of a data block in bytes.
 *
 * @param handle The data handle.
 * @return The total size in bytes.
 */
LIBRARY_API unsigned int udaGetTotalDataBlockSize(int handle);

/**
 * Gets a pointer to the data associated with a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the data.
 */
LIBRARY_API char* udaGetData(int handle);

/**
 * Gets the status of data associated with a handle.
 *
 * @param handle The data handle.
 * @return The data status.
 */
LIBRARY_API int udaGetDataStatus(int handle);

/**
 * Gets the number of data items in the data array.
 *
 * @param handle The data handle.
 * @return The number of data items.
 */
LIBRARY_API int udaGetDataCount(int handle);

/**
 * Gets the rank (dimensionality) of the data array.
 *
 * @param handle The data handle.
 * @return The rank of the data.
 */
LIBRARY_API int udaGetDataRank(int handle);

/**
 * Gets the time dimension order in the data array.
 *
 * @param handle The data handle.
 * @return The time dimension position.
 */
LIBRARY_API int udaGetDataOrder(int handle);

/**
 * Gets the type of data.
 *
 * @param handle The data handle.
 * @return The data type code.
 */
LIBRARY_API int udaGetDataType(int handle);

/**
 * Gets the type ID for a named type.
 *
 * @param type The name of the type.
 * @return The type ID.
 */
LIBRARY_API int udaGetDataTypeId(const char* type);

/**
 * Gets the size in bytes of a data type.
 *
 * @param type The type ID.
 * @return The size in bytes.
 */
LIBRARY_API int udaGetDataTypeSize(int type);

/**
 * Gets the label associated with the data.
 *
 * @param handle The data handle.
 * @return The data label.
 */
LIBRARY_API const char* udaGetDataLabel(int handle);

/**
 * Gets the units associated with the data.
 *
 * @param handle The data handle.
 * @return The data units.
 */
LIBRARY_API const char* udaGetDataUnits(int handle);

/**
 * Gets the description associated with the data.
 *
 * @param handle The data handle.
 * @return The data description.
 */
LIBRARY_API const char* udaGetDataDesc(int handle);


/**
 * Gets the type of opaque data.
 *
 * @param handle The data handle.
 * @return The opaque data type.
 */
LIBRARY_API int udaGetDataOpaqueType(int handle);

/**
 * Gets a pointer to the opaque data block.
 *
 * @param handle The data handle.
 * @return A pointer to the opaque data block.
 */
LIBRARY_API void* udaGetDataOpaqueBlock(int handle);

/**
 * Gets the count of opaque data items.
 *
 * @param handle The data handle.
 * @return The number of opaque data items.
 */
LIBRARY_API int udaGetDataOpaqueCount(int handle);

/**
 * Gets a pointer to the dimension data.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return A pointer to the dimension data.
 */
LIBRARY_API char* udaGetDimData(int handle, int n_dim);

/**
 * Gets the number of elements in a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The number of elements.
 */
LIBRARY_API int udaGetDimCount(int handle, int n_dim);

/**
 * Gets the data type of a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The dimension data type.
 */
LIBRARY_API int udaGetDimType(int handle, int n_dim);

/**
 * Gets the label associated with a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The dimension label.
 */
LIBRARY_API const char* udaGetDimLabel(int handle, int n_dim);

/**
 * Gets the units associated with a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The dimension units.
 */
LIBRARY_API const char* udaGetDimUnits(int handle, int n_dim);

/**
 * Calculates a checksum for a data array.
 *
 * @param data A pointer to the data.
 * @param data_n The number of data elements.
 * @param type The data type.
 * @return The calculated checksum.
 */
LIBRARY_API int udaDataCheckSum(const char* data, int data_n, int type);

/**
 * Gets the checksum for data associated with a handle.
 *
 * @param handle The data handle.
 * @return The checksum.
 */
LIBRARY_API int udaGetDataCheckSum(int handle);

/**
 * Gets the checksum for dimension data.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The checksum.
 */
LIBRARY_API int udaGetDimDataCheckSum(int handle, int n_dim);

/**
 * Sets the data tree associated with a handle.
 *
 * @param handle The data handle.
 * @return 1 on success, 0 on failure.
 */
LIBRARY_API int udaSetDataTree(int handle);

/**
 * Gets the data tree associated with a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the data tree.
 */
LIBRARY_API NTREE* udaGetDataTree(int handle);

////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

/// Marked for deletion but used in multiple places or some wrappers

/**
 * Gets the size of the server error stack.
 *
 * @deprecated Used in C++ client wrapper.
 * @return The size of the server error stack.
 */
LIBRARY_API int udaGetServerErrorStackSize();

/**
 * Gets the type of a server error stack record.
 *
 * @deprecated Used in IDL wrapper.
 * @param record The error record index.
 * @return The error record type.
 */
LIBRARY_API int udaGetServerErrorStackRecordType(int record);

/**
 * Gets the code of a server error stack record.
 *
 * @deprecated Used in C++ and IDL wrappers.
 * @param record The error record index.
 * @return The error record code.
 */
LIBRARY_API int udaGetServerErrorStackRecordCode(int record);

/**
 * Gets the location of a server error stack record.
 *
 * @deprecated Used in C++ and IDL wrappers.
 * @param record The error record index.
 * @return The error record location.
 */
LIBRARY_API const char* udaGetServerErrorStackRecordLocation(int record);

/**
 * Gets the message of a server error stack record.
 *
 * @deprecated Used in C++, IDL wrappers and plugin.
 * @param record The error record index.
 * @return The error record message.
 */
LIBRARY_API const char* udaGetServerErrorStackRecordMsg(int record);

/**
 * Sets the error model for a data handle.
 *
 * @deprecated Used in IDL wrapper.
 * @param handle The data handle.
 * @param model The error model ID.
 * @param param_n The number of model parameters.
 * @param params The model parameters.
 */
LIBRARY_API void udaPutErrorModel(int handle, int model, int param_n, const float* params);

/**
 * Sets the error model for a dimension.
 *
 * @deprecated Used in IDL wrapper.
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param model The error model ID.
 * @param param_n The number of model parameters.
 * @param params The model parameters.
 */
LIBRARY_API void udaPutDimErrorModel(int handle, int n_dim, int model, int param_n, const float* params);
// Synthetic data functions

/**
 * Generates synthetic dimension data.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return A pointer to the synthetic dimension data.
 */
LIBRARY_API char* udaGenerateSyntheticDimData(int handle, int n_dim);

/**
 * Gets synthetic data for a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the synthetic data.
 */
LIBRARY_API char* udaGetSyntheticData(int handle);

/**
 * Gets synthetic dimension data.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return A pointer to the synthetic dimension data.
 */
LIBRARY_API char* udaGetSyntheticDimData(int handle, int n_dim);

/**
 * Sets synthetic data for a handle.
 *
 * @param handle The data handle.
 * @param data A pointer to the synthetic data.
 */
LIBRARY_API void udaSetSyntheticData(int handle, char* data);

/**
 * Sets synthetic dimension data.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param data A pointer to the synthetic dimension data.
 */
LIBRARY_API void udaSetSyntheticDimData(int handle, int n_dim, char* data);

/**
 * Gets the log malloc list for a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the log malloc list.
 */
LIBRARY_API LOGMALLOCLIST* udaGetLogMallocList(int handle);

/**
 * Gets the user-defined type for a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the user-defined type.
 */
LIBRARY_API USERDEFINEDTYPE* udaGetUserDefinedType(int handle);

/**
 * Gets the user-defined type list for a handle.
 *
 * @param handle The data handle.
 * @return A pointer to the user-defined type list.
 */
LIBRARY_API USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);
//////////////////////////////////////////
//////////////////////////////////////////

/**
 * Gets the error model for data.
 *
 * @param handle The data handle.
 * @param model Pointer to receive the error model ID.
 * @param param_n Pointer to receive the number of parameters.
 * @param params Array to receive the model parameters.
 */
LIBRARY_API void    udaGetDataErrorModel(int handle, int* model, int* param_n, float* params);

/**
 * Gets the error type for data.
 *
 * @param handle The data handle.
 * @return The error type.
 */
LIBRARY_API int     udaGetDataErrorType(int handle);

/**
 * Gets the error asymmetry flag for data.
 *
 * @param handle The data handle.
 * @return The error asymmetry flag.
 */
LIBRARY_API int     udaGetDataErrorAsymmetry(int handle);

/**
 * Gets the ID for a named error model.
 *
 * @param model The name of the error model.
 * @return The error model ID.
 */
LIBRARY_API int     udaGetDataErrorModelId(const char* model);

/**
 * Gets the lower error bound for data.
 *
 * @param handle The data handle.
 * @return A pointer to the lower error data.
 */
LIBRARY_API char*   udaGetDataErrLo(int handle);

/**
 * Gets the upper error bound for data.
 *
 * @param handle The data handle.
 * @return A pointer to the upper error data.
 */
LIBRARY_API char*   udaGetDataErrHi(int handle);

/**
 * Gets the error asymmetry flag for data.
 *
 * @param handle The data handle.
 * @return The error asymmetry flag.
 */
LIBRARY_API int     udaGetDataErrAsymmetry(int handle);

/**
 * Gets the lower error bound for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return A pointer to the lower error data.
 */
LIBRARY_API char*   udaGetDimErrLo(int handle, int n_dim);

/**
 * Gets the upper error bound for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return A pointer to the upper error data.
 */
LIBRARY_API char*   udaGetDimErrHi(int handle, int n_dim);

/**
 * Gets the error asymmetry flag for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The error asymmetry flag.
 */
LIBRARY_API int     udaGetDimErrAsymmetry(int handle, int n_dim);

/**
 * Sets the error asymmetry flag for data.
 *
 * @param handle The data handle.
 * @param asymmetry The asymmetry flag (0 for symmetric, non-zero for asymmetric).
 */
LIBRARY_API void    udaSetDataErrAsymmetry(int handle, int asymmetry);

/**
 * Sets the error type for data.
 *
 * @param handle The data handle.
 * @param type The error type.
 */
LIBRARY_API void    udaSetDataErrType(int handle, int type);

/**
 * Sets the lower error bound for data.
 *
 * @param handle The data handle.
 * @param errlo A pointer to the lower error data.
 */
LIBRARY_API void    udaSetDataErrLo(int handle, char* errlo);

/**
 * Sets the error asymmetry flag for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param asymmetry The asymmetry flag (0 for symmetric, non-zero for asymmetric).
 */
LIBRARY_API void    udaSetDimErrAsymmetry(int handle, int n_dim, int asymmetry);

/**
 * Sets the error type for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param type The error type.
 */
LIBRARY_API void    udaSetDimErrType(int handle, int n_dim, int type);

/**
 * Sets the lower error bound for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param errlo A pointer to the lower error data.
 */
LIBRARY_API void    udaSetDimErrLo(int handle, int n_dim, char* errlo);
////////////////////////////

/**
 * Gets the error data for a handle.
 *
 * @param handle The data handle.
 * @param above If true, gets upper error bound; if false, gets lower bound.
 * @return A pointer to the error data.
 */
LIBRARY_API char*   udaGetDataError(int handle, bool above);

/**
 * Gets error data cast as float.
 *
 * @param handle The data handle.
 * @param above If true, gets upper error bound; if false, gets lower bound.
 * @param data Pre-allocated float array to receive the error data.
 */
LIBRARY_API void    udaGetFloatError(int handle, bool above, float* data);

/**
 * Gets the error data for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param above If true, gets upper error bound; if false, gets lower bound.
 * @return A pointer to the error data.
 */
LIBRARY_API char*   udaGetDimError(int handle, int n_dim, bool above);

/**
 * Gets the error type for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The error type.
 */
LIBRARY_API int     udaGetDimErrorType(int handle, int n_dim);

/**
 * Gets the error model for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param model Pointer to receive the error model ID.
 * @param param_n Pointer to receive the number of parameters.
 * @param params Array to receive the model parameters.
 */
LIBRARY_API void    udaGetDimErrorModel(int handle, int n_dim, int* model, int* param_n, float* params);

/**
 * Gets dimension error data cast as float.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @param above If true, gets upper error bound; if false, gets lower bound.
 * @param data Pre-allocated float array to receive the error data.
 */
LIBRARY_API void    udaGetFloatDimError(int handle, int n_dim, bool above, float* data);

/**
 * Gets the error asymmetry flag for a dimension.
 *
 * @param handle The data handle.
 * @param n_dim The dimension index.
 * @return The error asymmetry flag (0 for symmetric, non-zero for asymmetric).
 */
LIBRARY_API int     udaGetDimErrorAsymmetry(int handle, int n_dim);
//////////////////////////////////////////
//////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H
