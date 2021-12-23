#include "accAPI.h"

#include <cmath>
#include <vector>

#ifdef __GNUC__
#  include <pthread.h>
#  include <strings.h>
#  include <cmath>
#else
#  include <Windows.h>
#  include <string.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define strlwr _strlwr
#endif

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/stringUtils.h>
#include <clientserver/allocData.h>
#include <clientserver/protocol.h>
#include <clientserver/memstream.h>
#include <clientserver/xdrlib.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <version.h>

#include "generate_errors.hpp"
#include "thread_client.hpp"

#ifdef __APPLE__
#  include <cstdlib>
#elif !defined(A64)
#  include <malloc.h>
#endif

//--------------------------------------------------------------------------------------
// C Accessor Routines

void udaFreeDataBlocks()
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.clear();
}

DATA_BLOCK* udaGetCurrentDataBlock()
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.current_data_block();
}

int udaGetNewDataHandle()
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.new_handle();
}

//--------------------------------------------------------------
/* Notes:

Rank Ordering is as follows:

   Order is the Time Dimension from the Left, e.g.,

   Rank 2  A[t][x] has Order 0
      A[x][t] has Order 1

   Rank 3  A[t][x][y] has Order 0
      A[x][t][y] has Order 1
      A[x][y][t] has Order 2

 In IDL and FORTRAN the Time Dimension is counted from the Right, e.g.,

   Rank 3  A(y,x,t) has Order 0

*/

//--------------------------------------------------------------
// Private Flags (Server to Server communication via an UDA client server plugin)

//! Set a private_flags property
/** Set a/multiple specific bit/s in the private_flags property sent between UDA servers.
*
* @param flag The bit/s to be set to 1.
* @return Void.
*/
void udaSetPrivateFlag(unsigned int flag)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.set_flag(flag, true);
}

//! Reset a private_flags property
/** Reset a/multiple specific bit/s in the private_flags property sent between UDA servers.
*
* @param flag The bit/s to be set to 0.
* @return Void.
*/

void udaResetPrivateFlag(unsigned int flag)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.reset_flag(flag, true);
}

//--------------------------------------------------------------
// Client Flags

//! Set a client_flags->flags property
/** Set a/multiple specific bit/s in the client_flags->flags property sent to the UDA server.
*
* @param flag The bit/s to be set to 1.
* @return Void.
*/

void udaSetClientFlag(unsigned int flag)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.set_flag(flag);
}

//! Reset a client_flags->flags property
/** Reset a/multiple specific bit/s in the client_flags->flags property sent to the UDA server.
*
* @param flag The bit/s to be set to 0.
* @return Void.
*/

void udaResetClientFlag(unsigned int flag)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.reset_flag(flag);
}

//--------------------------------------------------------------
// Set Server Properties

//! Set a named server property
/** Set a variety of data server properties using their name. These affect the data type returned and any server side processing of data.
* Not all data access plugins respond to these properties.\n
*
* \eget_datadble  data are returned in double precision.\n
* \eget_dimdble   all coordinate (dimension) data are returned in double precision.\n
* \eget_timedble  the Time coordinate (dimension) data are returned in double precision.\n
* \eget_bytes\n
* \eget_bad\n
* \eget_meta   return all SQL database records used to locate and correct the requested data\n
* \eget_asis   do not apply server side correction to data\n
* \eget_uncal\n
* \eget_notoff do not apply any timing offset corrections\n
* \eget_synthetic\n
* \eget_scalar\n
* \eget_nodimdata do not return coordinate (dimension) data\n
* \etimeout=value name value pair to set the number of wait seconds before timing out the server connection\n
* \everbose \n
* \edebug      create debug output from the client\n
* \ealtData use efit++ with legacy efm data signal names \n
* \ealtRank select different efit++ output file as the data source \n
*
* @param property the name of the property to set true or a name value pair.
* @return Void.
*/
void udaSetProperty(const char* property, CLIENT_FLAGS* client_flags)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.set_property(property);
}

//! Return the value of a named server property
/**
* @param property the name of the property.
* @return Void.
*/
int udGetProperty(const char* property, int user_timeout, int alt_rank)
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.get_property(property);
}

//! Reset a specific named data server property to its default value
/**
* @param property the name of the property.
* @return Void.
*/

void udaResetProperty(const char* property, CLIENT_FLAGS* client_flags)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.reset_property(property);
}

//! Reset all data server properties to their default values
/**
* @return Void.
*/
void udaResetProperties()
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.reset_properties();
}

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
* @return CLIENT_BLOCK pointer to the data structure.
*/
const CLIENT_BLOCK* udaGetProperties(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.client_block(handle);
}

//--------------------------------------------------------------
//! Test for amount of Free heap memory and current usage
/** When the UDA client is a server plugin, set the Client's Debug File handle to that of the Server.
* @return void
*/
#if !defined(__APPLE__) && !defined(_WIN32)

int udaGetMemoryFree()
{
#ifdef A64
    return 0;
#else
    struct mallinfo stats = mallinfo();
    return (int) stats.fordblks;
#endif
}

int udaGetMemoryUsed()
{
#ifdef A64
    return 0;
#else
    struct mallinfo stats = mallinfo();
    return (int) stats.uordblks;
#endif
}

#endif

//--------------------------------------------------------------
// Standard C PUT Accessor Routines

void udaPutErrorModel(int handle, int model, int param_n, const float* params)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return;   // No valid Model

    data_block->error_model = model;               // Model ID
    data_block->error_param_n = param_n;             // Number of parameters

    if (param_n > MAXERRPARAMS) data_block->error_param_n = MAXERRPARAMS;

    for (int i = 0; i < data_block->error_param_n; i++) {
        data_block->errparams[i] = params[i];
    }
}

void udaPutDimErrorModel(int handle, int ndim, int model, int param_n, const float* params)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    if (ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;                     // No Dim
    }
    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return;  // No valid Model

    data_block->dims[ndim].error_model = model;                        // Model ID
    data_block->dims[ndim].error_param_n = param_n;                      // Number of parameters

    if (param_n > MAXERRPARAMS) data_block->dims[ndim].error_param_n = MAXERRPARAMS;
    for (int i = 0; i < data_block->dims[ndim].error_param_n; i++) {
        data_block->dims[ndim].errparams[i] = params[i];
    }
}

//! Set the UDA data server host name and port number
/** This takes precedence over the environment variables UDA_HOST and UDA_PORT.
* @param host The name of the server host computer.
* @param port The port number the server is connected to.
* @return void
*/
void udaPutServer(const char* host, int port, bool* env_host, bool* env_port)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    environment->server_port = port;                             // UDA server service port number
    strcpy(environment->server_host, host);                      // UDA server's host name or IP address
    environment->server_reconnect = 1;                           // Create a new Server instance
    *env_host = false;                                           // Skip initialisation at Startup if these are called first
    *env_port = false;
}

//! Set the UDA data server host name
/** This takes precedence over the environment variables UDA_HOST.
* @param host The name of the server host computer.
* @return void
*/
void udaPutServerHost(const char* host, bool* env_host)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    strcpy(environment->server_host, host);                      // UDA server's host name or IP address
    environment->server_reconnect = 1;                           // Create a new Server instance
    *env_host = false;
}

//! Set the UDA data server port number
/** This takes precedence over the environment variables UDA_PORT.
* @param port The port number the server is connected to.
* @return void
*/
void udaPutServerPort(int port, bool* env_port)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    environment->server_port = port;                             // UDA server service port number
    environment->server_reconnect = 1;                           // Create a new Server instance
    *env_port = false;
}

//! Specify a specific UDA server socket connection to use
/** The client can be connected to multiple servers, distinguished by their socket id.
Select the server connection required.
* @param socket The socket ID of the server connection required.
* @return void
*/
void udaPutServerSocket(int socket)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    if (environment->server_socket != socket) {      // Change to a different socket
        environment->server_socket = socket;         // UDA server service socket number (Must be Open)
        environment->server_change_socket = 1;       // Connect to an Existing Server
    }
}

//--------------------------------------------------------------
// Standard C GET Accessor Routines

//! Return the UDA data server host name, port number and socket connection id
/**
* @param host A preallocated string that will contain the name of the server host computer.
* @param port Returned port number.
* @param socket Returned socket id number.
* @return void
*/
void udaGetServer(const char** host, int* port, int* socket)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    *socket = environment->server_socket;                        // UDA server service socket number
    *port = environment->server_port;                          // UDA server service port number
    *host = environment->server_host;                          // UDA server's host name or IP address
}

//! the UDA server connection host name
/**
* @return the Name of the Host
*/
const char* udaGetServerHost()
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    return environment->server_host;                             // Active UDA server's host name or IP address
}

//! the UDA server connection port number
/**
* @return the Name of the Host
*/
int udaGetServerPort()
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    return environment->server_port;                             // Active UDA server service port number
}

const char* getUdaBuildVersion()
{
    return UDA_BUILD_VERSION;
}

const char* getUdaBuildDate()
{
    return UDA_BUILD_DATE;
}

//! the UDA server connection socket ID
/**
* @return the connection socket ID
*/
int udaGetServerSocket()
{
    auto& instance = uda::client::ThreadClient::instance();
    auto environment = instance.environment();
    return environment->server_socket;           // Active UDA server service socket number
}

//! the Error code of a specific server error record
/**
* @param record the error stack record number
* @return the error code
*/
int udaGetServerErrorStackRecordCode(int record)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto server_block = instance.server_block();

    if (record < 0 || (unsigned int)record >= server_block->idamerrorstack.nerrors) {
        return 0;
    }
    return server_block->idamerrorstack.idamerror[record].code;  // Server Error Stack Record Code
}

//!  returns the data access error code
/**
\param   handle   The data object handle.
\return   Return error code, if non-zero there was a problem: < 0 is client side, > 0 is server side.
*/
int udaGetErrorCode(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Error Code Returned from Server
    if (data_block == nullptr) {
        return udaGetServerErrorStackRecordCode(0);
    } else {
        return data_block->errcode;
    }
}

//!  returns the data access error message
/**
\param   handle   The data object handle.
\return   the error message.
*/
const char* udaGetErrorMsg(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);
    auto server_block = instance.server_block();

    // Error Message returned from server
    if (data_block == nullptr) {
        if (server_block->idamerrorstack.nerrors > 0) {
            return server_block->idamerrorstack.idamerror[0].msg;
        } else {
            return "Unknown server error";
        }
    } else {
        return data_block->error_msg;
    }
}

//!  returns the data source quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int udGetSourceStatus(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->source_status;
}

//!  returns the data object quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int udaGetSignalStatus(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Signal Status
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->signal_status;
}

int udaGetDataStatus(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Data Status based on Standard Rule
    if (data_block == nullptr) {
        return 0;
    }
    if (udaGetSignalStatus(handle) == DEFAULT_STATUS) {
        // Signal Status Not Changed from Default - use Data Source Value
        return data_block->source_status;
    } else {
        return data_block->signal_status;
    }
}

//!  returns the last data object handle issued
/**
\return   handle.
*/
int udaGetLastHandle()
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.current_data_block()->handle;
}

//!  returns the number of data items in the data object
/** the number of array elements
\param   handle   The data object handle
\return  the number of data items
*/
int udaGetDataNum(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Data Array Size
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->data_n;
}

//!  returns the rank of the data object
/** the number of array coordinate dimensions
\param   handle   The data object handle
\return  the rank
*/
int udaGetRank(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Array Rank
    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->rank;
}

//!  Returns the position of the time coordinate dimension in the data object
/** For example, a rank 3 array data[time][x][y] (in Fortran and IDL this is data(y,x,time)) has time order = 0 so order is
counted from left to right in c and from right to left in Fortran and IDL.
\param   handle   The data object handle
\return  the time coordinate dimension position
*/
int udaGetOrder(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Time Dimension Order in Array
    if (data_block == nullptr) {
        return -1;
    }
    return data_block->order;
}

/**
 * Returns the Server's Permission to locally Cache data
 * @param handle The data object handle
 * @return the permission
 */
unsigned int udaGetCachePermission(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    // Permission to cache?
    if (data_block == nullptr) {
        return UDA_PLUGIN_NOT_OK_TO_CACHE;
    }
    return data_block->cachePermission;
}

/**
 * Returns the total amount of data (bytes)
 *
 * @param handle The data object handle
 * @return byte count
 */
unsigned int udaGetTotalDataBlockSize(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->totalDataBlockSize;
}

//!  returns the atomic or structure type id of the data object
/**
\param   handle   The data object handle
\return  the type id
*/
int udaGetDataType(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->data_type;
}

int udaGetDataOpaqueType(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->opaque_type;
}

void* udaGetDataOpaqueBlock(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->opaque_block;
}

int udaGetDataOpaqueCount(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->opaque_count;
}

//!  returns the atomic or structure type id of the error data object
/**
\param   handle   The data object handle
\return  the type id
*/
int udaGetErrorType(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->error_type;
}

//!  returns the atomic or structure type id of a named type
/**
\param   type   The name of the type
\return  the type id
*/
int udaGetDataTypeId(const char* type)
{
    // Return the Internal Code for Data Types
    if (STR_IEQUALS(type, "dcomplex")) return UDA_TYPE_DCOMPLEX;
    if (STR_IEQUALS(type, "complex")) return UDA_TYPE_COMPLEX;
    if (STR_IEQUALS(type, "double")) return UDA_TYPE_DOUBLE;
    if (STR_IEQUALS(type, "float")) return UDA_TYPE_FLOAT;
    if (STR_IEQUALS(type, "long64")) return UDA_TYPE_LONG64;
    if (STR_IEQUALS(type, "long long")) return UDA_TYPE_LONG64;
    if (STR_IEQUALS(type, "ulong64")) return UDA_TYPE_UNSIGNED_LONG64;
    if (STR_IEQUALS(type, "unsigned long64")) return UDA_TYPE_UNSIGNED_LONG64;
    if (STR_IEQUALS(type, "unsigned long long")) return UDA_TYPE_UNSIGNED_LONG64;
    if (STR_IEQUALS(type, "long")) return UDA_TYPE_LONG;
    if (STR_IEQUALS(type, "unsigned long")) return UDA_TYPE_UNSIGNED_LONG;
    if (STR_IEQUALS(type, "int")) return UDA_TYPE_INT;
    if (STR_IEQUALS(type, "integer")) return UDA_TYPE_INT;
    if (STR_IEQUALS(type, "unsigned")) return UDA_TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "unsigned int")) return UDA_TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "unsigned integer")) return UDA_TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "short")) return UDA_TYPE_SHORT;
    if (STR_IEQUALS(type, "unsigned short")) return UDA_TYPE_UNSIGNED_SHORT;
    if (STR_IEQUALS(type, "char")) return UDA_TYPE_CHAR;
    if (STR_IEQUALS(type, "unsigned char")) return UDA_TYPE_UNSIGNED_CHAR;
    if (STR_IEQUALS(type, "unknown")) return UDA_TYPE_UNKNOWN;
    if (STR_IEQUALS(type, "undefined")) return UDA_TYPE_UNDEFINED;

    if (STR_IEQUALS(type, "vlen")) return UDA_TYPE_VLEN;
    if (STR_IEQUALS(type, "compound")) return UDA_TYPE_COMPOUND;
    if (STR_IEQUALS(type, "opaque")) return UDA_TYPE_OPAQUE;
    if (STR_IEQUALS(type, "enum")) return UDA_TYPE_ENUM;
    if (STR_IEQUALS(type, "string")) return UDA_TYPE_STRING;
    if (STR_IEQUALS(type, "void")) return UDA_TYPE_VOID;

    if (STR_IEQUALS(type, "string *")) return UDA_TYPE_STRING;

    return UDA_TYPE_UNKNOWN;
}

int udaGetDataTypeSize(int type)
{
    // Return the size of the Data Type
    switch (type) {
        case UDA_TYPE_DCOMPLEX:
            return 0;
        case UDA_TYPE_COMPLEX:
            return 0;
        case UDA_TYPE_DOUBLE:
            return 8;
        case UDA_TYPE_FLOAT:
            return 4;
        case UDA_TYPE_LONG64:
            return 8;
        case UDA_TYPE_UNSIGNED_LONG64:
            return 8;
        case UDA_TYPE_LONG:
            return 8;
        case UDA_TYPE_UNSIGNED_LONG:
            return 8;
        case UDA_TYPE_INT:
            return 4;
        case UDA_TYPE_UNSIGNED_INT:
            return 4;
        case UDA_TYPE_SHORT:
            return 2;
        case UDA_TYPE_UNSIGNED_SHORT:
            return 2;
        case UDA_TYPE_CHAR:
            return 1;
        case UDA_TYPE_UNSIGNED_CHAR:
            return 1;
        case UDA_TYPE_UNKNOWN:
            return 0;
        case UDA_TYPE_UNDEFINED:
            return 0;
        case UDA_TYPE_VLEN:
            return 0;
        case UDA_TYPE_COMPOUND:
            return 0;
        case UDA_TYPE_OPAQUE:
            return 0;
        case UDA_TYPE_ENUM:
            return 4;
        case UDA_TYPE_STRING:
            return 0;
        case UDA_TYPE_VOID:
            return 8;
        default:
            return -1;
    }
}

void udaGetErrorModel(int handle, int* model, int* param_n, float* params)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        *model = ERROR_MODEL_UNKNOWN;
        *param_n = 0;
        return;
    }
    *model = data_block->error_model;     // Model ID
    *param_n = data_block->error_param_n;      // Number of parameters
    for (int i = 0; i < data_block->error_param_n; i++) {
        params[i] = data_block->errparams[i];
    }
}

int udaGetErrorAsymmetry(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->errasymmetry;
}

// Return the Internal Code for a named Error Model

int udaGetErrorModelId(const char* model)
{
    for (int i = 1; i < ERROR_MODEL_UNDEFINED; i++) {
        switch (i) {
            case 1:
                if (STR_IEQUALS(model, "default")) return ERROR_MODEL_DEFAULT;
                break;
            case 2:
                if (STR_IEQUALS(model, "default_asymmetric")) return ERROR_MODEL_DEFAULT_ASYMMETRIC;
                break;
#ifdef NO_GSL_LIB
            case 3:
                if (STR_IEQUALS(model, "gaussian")) return ERROR_MODEL_GAUSSIAN;
                break;
            case 4:
                if (STR_IEQUALS(model, "reseed")) return ERROR_MODEL_RESEED;
                break;
            case 5:
                if (STR_IEQUALS(model, "gaussian_shift")) return ERROR_MODEL_GAUSSIAN_SHIFT;
                break;
            case 6:
                if (STR_IEQUALS(model, "poisson")) return ERROR_MODEL_POISSON;
                break;
#endif
            default:
                return ERROR_MODEL_UNKNOWN;
        }
    }
    return 0;
}

char* udaGetSyntheticDimData(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].synthetic;
}

void udaSetSyntheticData(int handle, char* data)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->synthetic = data;
}

void udaSetSyntheticDimData(int handle, int ndim, char* data)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].synthetic = data;
}

char* udaGetSyntheticData(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);
    auto client_flags = instance.client_flags();

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return nullptr;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return nullptr;
    }
    if (!client_flags->get_synthetic || data_block->error_model == ERROR_MODEL_UNKNOWN) {
        return data_block->data;
    }
    uda::client::generate_synthetic_data(handle);
    return data_block->synthetic;
}

//!  Returns a pointer to the requested data
/** The data may be synthetically generated.
\param   handle   The data object handle
\return  a pointer to the data - if the status is poor, a nullptr pointer is returned unless the \e get_bad property is set.
*/
char* udaGetData(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);
    auto client_flags = instance.client_flags();

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return nullptr;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return nullptr;
    }
    if (!client_flags->get_synthetic) {
        return data_block->data;
    } else {
        return udaGetSyntheticData(handle);
    }
}

//! Copy the requested data block to a data buffer for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   char  A preallocated memory block to receive a copy of the data
\return  void
*/
void udaGetDataTdi(int handle, char* data)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    memcpy(data, (void*)data_block->data, (int)data_block->data_n);
}

char* udaGetDataErrLo(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errlo;
}

char* udaGetDataErrHi(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errhi;
}

int udaGetDataErrAsymmetry(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->errasymmetry;
}

void udaSetDataErrAsymmetry(int handle, int asymmetry)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errasymmetry = asymmetry;
};

void udaSetDataErrType(int handle, int type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->error_type = type;
};

void udaSetDataErrLo(int handle, char* errlo)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errlo = errlo;
};

char* udaGetDimErrLo(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].errlo;
}

char* udaGetDimErrHi(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].errhi;
}

int udaGetDimErrAsymmetry(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->dims[ndim].errasymmetry;
}

void udaSetDimErrAsymmetry(int handle, int ndim, int asymmetry)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].errasymmetry = asymmetry;
};

void udaSetDimErrType(int handle, int ndim, int type)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].error_type = type;
};

void udaSetDimErrLo(int handle, int ndim, char* errlo)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].errlo = errlo;
};

char* udaGetAsymmetricError(int handle, bool above)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    if (data_block->error_type != UDA_TYPE_UNKNOWN) {
        if (above) {
            return data_block->errhi;      // return the default error array
        } else {
            if (!data_block->errasymmetry) {
                return data_block->errhi;     // return the default error array if symmetric errors
            } else {
                return data_block->errlo;
            }     // otherwise the data array must have been returned by the server or generated
        }
    } else {
        if (data_block->error_model != ERROR_MODEL_UNKNOWN) {
            uda::client::generate_data_error(handle);
            if (above) {
                return data_block->errhi;
            } else if (!data_block->errasymmetry) {
                return data_block->errhi;
            } else {
                return data_block->errlo;
            }
        } else {

            char* errhi = nullptr;    // Regular Error Component
            char* errlo = nullptr;    // Asymmetric Error Component
            int ndata;

            ndata = data_block->data_n;
            data_block->error_type = data_block->data_type;  // Error Type is Unknown so Assume Data's Data Type

            if (allocArray(data_block->error_type, ndata, &errhi) != 0) {
                // Allocate Heap for Regular Error Data
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Data Errors\n");
                data_block->errhi = nullptr;
            } else {
                data_block->errhi = errhi;
            }

            if (data_block->errasymmetry) {           // Allocate Heap for the Asymmetric Error Data
                if (allocArray(data_block->error_type, ndata, &errlo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Asymmetric Errors\n");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    data_block->errlo = nullptr;
                    data_block->errasymmetry = 0;
                } else {
                    data_block->errlo = errlo;
                }
            }

            // Generate and return Zeros if this data is requested unless Error is Modelled

            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    float* fh, * fl = nullptr;
                    fh = (float*)data_block->errhi;
                    if (data_block->errasymmetry) fl = (float*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(fh + i) = (float)0.0;
                        if (data_block->errasymmetry) *(fl + i) = (float)0.0;
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    double* dh, * dl = nullptr;
                    dh = (double*)data_block->errhi;
                    if (data_block->errasymmetry) dl = (double*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(dh + i) = (double)0.0;
                        if (data_block->errasymmetry) *(dl + i) = (double)0.0;
                    }
                    break;
                }
                case UDA_TYPE_SHORT: {
                    short* sh, * sl = nullptr;
                    sh = (short*)data_block->errhi;
                    if (data_block->errasymmetry) sl = (short*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(sh + i) = (short)0;
                        if (data_block->errasymmetry) *(sl + i) = (short)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_SHORT: {
                    unsigned short* sh, * sl = nullptr;
                    sh = (unsigned short*)data_block->errhi;
                    if (data_block->errasymmetry) sl = (unsigned short*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (data_block->errasymmetry) sl[i] = (unsigned short)0;
                    }
                    break;
                }
                case UDA_TYPE_INT: {
                    int* ih, * il = nullptr;
                    ih = (int*)data_block->errhi;
                    if (data_block->errasymmetry) il = (int*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(ih + i) = (int)0;
                        if (data_block->errasymmetry) *(il + i) = (int)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_INT: {
                    unsigned int* uh, * ul = nullptr;
                    uh = (unsigned int*)data_block->errhi;
                    if (data_block->errasymmetry) ul = (unsigned int*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(uh + i) = (unsigned int)0;
                        if (data_block->errasymmetry) *(ul + i) = (unsigned int)0;
                    }
                    break;
                }
                case UDA_TYPE_LONG: {
                    long* lh, * ll = nullptr;
                    lh = (long*)data_block->errhi;
                    if (data_block->errasymmetry) ll = (long*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(lh + i) = (long)0;
                        if (data_block->errasymmetry) *(ll + i) = (long)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG: {
                    unsigned long* lh, * ll = nullptr;
                    lh = (unsigned long*)data_block->errhi;
                    if (data_block->errasymmetry) ll = (unsigned long*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (data_block->errasymmetry) ll[i] = (unsigned long)0;
                    }
                    break;
                }
                case UDA_TYPE_LONG64: {
                    long long int* lh, * ll = nullptr;
                    lh = (long long int*)data_block->errhi;
                    if (data_block->errasymmetry) ll = (long long int*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(lh + i) = (long long int)0;
                        if (data_block->errasymmetry) *(ll + i) = (long long int)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG64: {
                    unsigned long long int* lh, * ll = nullptr;
                    lh = (unsigned long long int*) data_block->errhi;
                    if (data_block->errasymmetry) ll = (unsigned long long int*) data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int) 0;
                        if (data_block->errasymmetry) ll[i] = (unsigned long long int) 0;
                    }
                    break;
                }
                case UDA_TYPE_CHAR: {
                    char* ch, * cl = nullptr;
                    ch = data_block->errhi;
                    if (data_block->errasymmetry) cl = data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (char)0;
                        if (data_block->errasymmetry) cl[i] = (char)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_CHAR: {
                    unsigned char* ch, * cl = nullptr;
                    ch = (unsigned char*)data_block->errhi;
                    if (data_block->errasymmetry) cl = (unsigned char*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (data_block->errasymmetry) cl[i] = (unsigned char)0;
                    }
                    break;
                }
                case UDA_TYPE_DCOMPLEX: {
                    DCOMPLEX* ch, * cl = nullptr;
                    ch = (DCOMPLEX*)data_block->errhi;
                    if (data_block->errasymmetry) cl = (DCOMPLEX*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i].real = (double)0.0;
                        ch[i].imaginary = (double)0.0;
                        if (data_block->errasymmetry) {
                            cl[i].real = (double)0.0;
                            cl[i].imaginary = (double)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_COMPLEX: {
                    COMPLEX* ch, * cl = nullptr;
                    ch = (COMPLEX*)data_block->errhi;
                    if (data_block->errasymmetry) cl = (COMPLEX*)data_block->errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i].real = (float)0.0;
                        ch[i].imaginary = (float)0.0;
                        if (data_block->errasymmetry) {
                            cl[i].real = (float)0.0;
                            cl[i].imaginary = (float)0.0;
                        }
                    }
                    break;
                }
            }

            if (above) {
                return data_block->errhi;
            } else if (!data_block->errasymmetry) {
                return data_block->errhi;
            } else {
                return data_block->errlo;
            }
        }
    }
}

//!  Returns a pointer to the memory block containing the requested error data
/** The error data may be synthetically generated.
\param   handle   The data object handle
\return  a pointer to the data
*/
char* udaGetError(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    int above = 1;
    if (data_block == nullptr) {
        return nullptr;
    }
    return udaGetAsymmetricError(handle, above);
}

//!  Returns data cast to double precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the status of the data is poor, no copy to the buffer occurs unless
the property \b get_bad is set.
\param   handle   The data object handle
\param   fp A \b double pointer to a preallocated data buffer
\return  void
*/
void udaGetDoubleData(int handle, double* fp)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);
    auto client_flags = instance.client_flags();

    // Copy Data cast as double to User Provided Array

    // **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) return;
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) return;
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) return;

    if (data_block->data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(double));
        else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->synthetic,
                       (size_t)data_block->data_n * sizeof(double));
            else
                memcpy((void*)fp, (void*)data_block->data,
                       (size_t)data_block->data_n * sizeof(double));
            return;
        }
    } else {

        char* array;
        int ndata;

        ndata = udaGetDataNum(handle);

        if (!client_flags->get_synthetic) {
            array = data_block->data;
        } else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                array = data_block->synthetic;
            } else {
                array = data_block->data;
            }
        }

        switch (data_block->data_type) {
            case UDA_TYPE_FLOAT: {
                auto dp = (float*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)dp[i];
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)ip[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)up[i];
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*) array;
                for (int i = 0; i < ndata; i++) fp[i] = (double) lp[i];
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < ndata; i++) fp[i] = (double)0.0;  // No Data !
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto dp = (DCOMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto dp = (COMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            default:
                for (int i = 0; i < ndata; i++) fp[i] = (double)0.0;
                break;

        }
        return;
    }
}


//!  Returns data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the status of the data is poor, no copy to the buffer occurs unless
the property \b get_bad is set.
\param   handle   The data object handle
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatData(int handle, float* fp)
{
    // Copy Data cast as float to User Provided Array

    // **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);
    auto client_flags = instance.client_flags();

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) return;
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) return;

    if (data_block->data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(float));
        else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->synthetic,
                       (size_t)data_block->data_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)data_block->data,
                       (size_t)data_block->data_n * sizeof(float));
            return;
        }
    } else {

        char* array;
        int ndata;

        ndata = udaGetDataNum(handle);

        if (!client_flags->get_synthetic) {
            array = data_block->data;
        } else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                array = data_block->synthetic;
            } else {
                array = data_block->data;
            }
        }

        switch (data_block->data_type) {
            case UDA_TYPE_DOUBLE: {
                double* dp = (double*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)dp[i];
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)ip[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)up[i];
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*) array;
                for (int i = 0; i < ndata; i++) fp[i] = (float) lp[i];
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < ndata; i++) fp[i] = (float)0.0;   // No Data !
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto dp = (DCOMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto dp = (COMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            default:
                for (int i = 0; i < ndata; i++) fp[i] = (float)0.0;
                break;

        }
        return;
    }
}

//!  Returns data as void type
/** The copy buffer must be preallocated and sized for the correct data type.
\param   handle   The data object handle
\param   data  A \b void pointer to a preallocated data buffer
\return  void
*/
void udaGetGenericData(int handle, void* data)
{
    switch (udaGetDataType(handle)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(int));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(unsigned int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(long));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(unsigned long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(long long int));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(short));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(unsigned short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(COMPLEX));
            break;
    }
}


void udaGetFloatAsymmetricError(int handle, int above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    // **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }

    int ndata = data_block->data_n;

    if (data_block->error_type == UDA_TYPE_UNKNOWN) {
        udaGetAsymmetricError(handle, above);
    } // Create the Error Data prior to Casting

    switch (data_block->error_type) {
        case UDA_TYPE_UNKNOWN:
            for (int i = 0; i < ndata; i++) fp[i] = (float)0.0; // No Error Data
            break;
        case UDA_TYPE_FLOAT:
            if (above)
                memcpy((void*)fp, (void*)data_block->errhi,
                       (size_t)data_block->data_n * sizeof(float));
            else if (!data_block->errasymmetry)
                memcpy((void*)fp, (void*)data_block->errhi,
                       (size_t)data_block->data_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)data_block->errlo,
                       (size_t)data_block->data_n * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE: {
            double* dp;
            if (above) {
                dp = (double*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                dp = (double*)data_block->errhi;
            } else {
                dp = (double*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            short* sp;
            if (above) {
                sp = (short*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                sp = (short*)data_block->errhi;
            } else {
                sp = (short*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            if (above) {
                sp = (unsigned short*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                sp = (unsigned short*)data_block->errhi;
            } else {
                sp = (unsigned short*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_INT: {
            int* ip;
            if (above) {
                ip = (int*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                ip = (int*)data_block->errhi;
            } else {
                ip = (int*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            unsigned int* up;
            if (above) {
                up = (unsigned int*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                up = (unsigned int*)data_block->errhi;
            } else {
                up = (unsigned int*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_LONG: {
            long* lp;
            if (above) {
                lp = (long*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                lp = (long*)data_block->errhi;
            } else {
                lp = (long*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            if (above) {
                lp = (unsigned long*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                lp = (unsigned long*)data_block->errhi;
            } else {
                lp = (unsigned long*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            long long int* lp;
            if (above) {
                lp = (long long int*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                lp = (long long int*)data_block->errhi;
            } else {
                lp = (long long int*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int* lp;
            if (above)
                lp = (unsigned long long int*) data_block->errhi;
            else if (!data_block->errasymmetry)
                lp = (unsigned long long int*) data_block->errhi;
            else
                lp = (unsigned long long int*) data_block->errlo;
            for (int i = 0; i < ndata; i++) fp[i] = (float) lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp;
            if (above) {
                cp = data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = data_block->errhi;
            } else {
                cp = data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            if (above) {
                cp = (unsigned char*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = (unsigned char*)data_block->errhi;
            } else {
                cp = (unsigned char*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DCOMPLEX* cp;
            if (above) {
                cp = (DCOMPLEX*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = (DCOMPLEX*)data_block->errhi;
            } else {
                cp = (DCOMPLEX*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            COMPLEX* cp;
            if (above) {
                cp = (COMPLEX*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = (COMPLEX*)data_block->errhi;
            } else {
                cp = (COMPLEX*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        default:
            for (int i = 0; i < ndata; i++) fp[i] = (float)0.0;
            break;

    }
}

//!  Returns error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatError(int handle, float* fp)
{
    int above = 1;
    udaGetFloatAsymmetricError(handle, above, fp);
}

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**
\param   handle   The data object handle
\param   db Returned \b DATA_BLOCK pointer
\return  void
*/
void udaGetDBlock(int handle, DATA_BLOCK* db)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    *db = *data_block;
}

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**
\param   handle   The data object handle
\return  DATA_BLOCK pointer
*/
DATA_BLOCK* udaGetDataBlock(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block;
}

//!  Returns the data label of a data object
/**
\param   handle   The data object handle
\return  pointer to the data label
*/
const char* udaGetDataLabel(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->data_label;
}

//!  Returns the data label of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   label   preallocated string buffer to receive the copy of the data label
\return  void
*/
void udaGetDataLabelTdi(int handle, char* label)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    strcpy(label, data_block->data_label);
}

//!  Returns the data units of a data object
/**
\param   handle   The data object handle
\return  pointer to the data units
*/
const char* udaGetDataUnits(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->data_units;
}

//!  Returns the data units of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   units   preallocated string buffer to receive the copy of the data units
\return  void
*/
void udaGetDataUnitsTdi(int handle, char* units)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    strcpy(units, data_block->data_units);
}

//!  Returns the description of a data object
/**
\param   handle   The data object handle
\return  pointer to the data description
*/
const char* udaGetDataDesc(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->data_desc;
}

//!  Returns the description of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   units   preallocated string buffer to receive the copy of the data description
\return  void
*/
void udaGetDataDescTdi(int handle, char* desc)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    strcpy(desc, data_block->data_desc);
}

// Dimension Coordinates

//! Returns the coordinate dimension size
/** the number of elements in the coordinate array
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the dimension size
*/
int udaGetDimNum(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 ||
        (unsigned int)ndim >= data_block->rank) {
        return 0;
    }
    return data_block->dims[ndim].dim_n;
}

//! Returns the coordinate dimension data type
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int udaGetDimType(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 ||
        (unsigned int)ndim >= data_block->rank) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->dims[ndim].data_type;
}

//! Returns the coordinate dimension error data type
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int udaGetDimErrorType(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 ||
        (unsigned int)ndim >= data_block->rank) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->dims[ndim].error_type;
}

//! Returns whether or not coordinate error data are asymmetric.
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  boolean true or false i.e. 1 or 0
*/
int udaGetDimErrorAsymmetry(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 ||
        (unsigned int)ndim >= data_block->rank) {
        return 0;
    }
    return data_block->dims[ndim].errasymmetry;
}

void udaGetDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        *model = ERROR_MODEL_UNKNOWN;
        *param_n = 0;
        return;
    }

    *model = data_block->dims[ndim].error_model;      // Model ID
    *param_n = data_block->dims[ndim].error_param_n;    // Number of parameters
    for (int i = 0; i < data_block->dims[ndim].error_param_n; i++) {
        params[i] = data_block->dims[ndim].errparams[i];
    }
    // *params  = data_block->dims[ndim].errparams;        // Array of Model Parameters
}

char* udaGenerateSyntheticDimData(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }

    auto client_flags = instance.client_flags();
    if (!client_flags->get_synthetic || data_block->dims[ndim].error_model == ERROR_MODEL_UNKNOWN) {
        return data_block->dims[ndim].dim;
    }
    uda::client::generate_synthetic_dim_data(handle, ndim);
    return data_block->dims[ndim].synthetic;
}

///!  Returns a pointer to the requested coordinate data
/** The data may be synthetically generated.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data
*/
char* udaGetDimData(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }

    auto client_flags = instance.client_flags();
    if (!client_flags->get_synthetic) {
        return data_block->dims[ndim].dim;
    }
    return udaGetSyntheticDimData(handle, ndim);
}

//! Returns the data label of a coordinate dimension
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data label
*/
const char* udaGetDimLabel(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    return data_block->dims[ndim].dim_label;
}
//! Returns the data units of a coordinate dimension
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data units
*/
const char* udaGetDimUnits(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    return data_block->dims[ndim].dim_units;
}

//!  Returns the data label of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   label   preallocated string buffer to receive the copy of the data label
\return  void
*/
void udaGetDimLabelTdi(int handle, int ndim, char* label)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }
    strcpy(label, data_block->dims[ndim].dim_label);
}

//!  Returns the data units of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   units   preallocated string buffer to receive the copy of the data units
\return  void
*/
void udaGetDimUnitsTdi(int handle, int ndim, char* units)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }
    strcpy(units, data_block->dims[ndim].dim_units);
}

//!  Returns coordinate data cast to double precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b double pointer to a preallocated data buffer
\return  void
*/
void udaGetDoubleDimData(int handle, int ndim, double* fp)
{
    // **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }

    auto client_flags = instance.client_flags();
    if (data_block->dims[ndim].data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                   (size_t)data_block->dims[ndim].dim_n * sizeof(double));
        else {
            uda::client::generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->dims[ndim].synthetic,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(double));
            else
                memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(double));
            return;
        }
    } else {
        char* array;

        int ndata = data_block->dims[ndim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[ndim].dim;
        } else {
            uda::client::generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                array = data_block->dims[ndim].synthetic;
            } else {
                array = data_block->dims[ndim].dim;
            }
        }

        switch (data_block->dims[ndim].data_type) {
            case UDA_TYPE_FLOAT: {
                auto dp = (float*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)dp[i];
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)ip[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)up[i];
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*) array;
                for (int i = 0; i < ndata; i++) fp[i] = (double) lp[i];
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto cp = (DCOMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto cp = (COMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
                for (int i = 0; i < ndata; i++) fp[i] = (double)0.0;
                break;
            default:
                for (int i = 0; i < ndata; i++) fp[i] = (double)0.0;
                break;

        }
        return;
    }
}

//!  Returns coordinate data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatDimData(int handle, int ndim, float* fp)
{
    // **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
            return;
    }

    auto client_flags = instance.client_flags();
    if (data_block->dims[ndim].data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                   (size_t)data_block->dims[ndim].dim_n * sizeof(float));
        else {
            uda::client::generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->dims[ndim].synthetic,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            return;
        }
    } else {
        char* array;

        int ndata = data_block->dims[ndim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[ndim].dim;
        } else {
            uda::client::generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                array = data_block->dims[ndim].synthetic;
            } else {
                array = data_block->dims[ndim].dim;
            }
        }

        switch (data_block->dims[ndim].data_type) {
            case UDA_TYPE_DOUBLE: {
                auto dp = (double*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)dp[i];
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case UDA_TYPE_INT: {
                auto ip = (int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)ip[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)up[i];
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*) array;
                for (int i = 0; i < ndata; i++) fp[i] = (float) lp[i];
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto cp = (DCOMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto cp = (COMPLEX*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
                for (int i = 0; i < ndata; i++) fp[i] = (float)0.0;
                break;
            default:
                for (int i = 0; i < ndata; i++) fp[i] = (float)0.0;
                break;
        }
        return;
    }
}

//!  Returns coordinate data as void type
/** The copy buffer must be preallocated and sized for the correct data type.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   data  A \b void pointer to a preallocated data buffer
\return  void
*/
void udaGetGenericDimData(int handle, int ndim, void* data)
{
    switch (udaGetDimType(handle, ndim)) {
        case UDA_TYPE_FLOAT:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned int));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned long));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned short));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, (void*)udaGetDimData(handle, ndim),
                   (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(DCOMPLEX));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(COMPLEX));
            break;
    }
}

//!  Returns the coordinate dimension's DIMS data structure - the coordinate data and associated meta data.
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  DIMS pointer
*/
DIMS* udaGetDimBlock(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
            return nullptr;
    }
    return data_block->dims + ndim;
}


char* udaGetDimAsymmetricError(int handle, int ndim, int above)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
            return nullptr;
    }
    if (data_block->dims[ndim].error_type != UDA_TYPE_UNKNOWN) {
        if (above) {
            return data_block->dims[ndim].errhi;    // return the default error array
        } else {
            if (!data_block->dims[ndim].errasymmetry) {
                return data_block->dims[ndim].errhi;   // return the default error array if symmetric errors
            } else {
                return data_block->dims[ndim].errlo;
            }   // otherwise the data array must have been returned by the server
        }                           // or generated in a previous call
    } else {
        if (data_block->dims[ndim].error_model != ERROR_MODEL_UNKNOWN) {
            uda::client::generate_dim_data_error(handle, ndim);
            if (above || !data_block->dims[ndim].errasymmetry) {
                return data_block->dims[ndim].errhi;
            } else {
                return data_block->dims[ndim].errlo;
            }
        } else {
            char* errhi = nullptr;
            char* errlo = nullptr;

            int ndata = data_block->dims[ndim].dim_n;
            data_block->dims[ndim].error_type = data_block->dims[ndim].data_type; // Error Type is Unknown so Assume Data's Data Type

            if (allocArray(data_block->dims[ndim].error_type, ndata, &errhi) != 0) {
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Data Errors\n");
                data_block->dims[ndim].errhi = nullptr;
            } else {
                data_block->dims[ndim].errhi = errhi;
            }

            if (data_block->dims[ndim].errasymmetry) {               // Allocate Heap for the Asymmetric Error Data
                if (allocArray(data_block->dims[ndim].error_type, ndata, &errlo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Asymmetric Errors\n");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    data_block->dims[ndim].errlo = errlo;
                    data_block->dims[ndim].errasymmetry = 0;
                } else {
                    data_block->dims[ndim].errlo = errlo;
                }
            }

            switch (data_block->dims[ndim].data_type) {
                case UDA_TYPE_FLOAT: {
                    float* fl = nullptr;
                    auto fh = (float*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) fl = (float*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        fh[i] = (float)0.0;
                        if (data_block->dims[ndim].errasymmetry) fl[i] = (float)0.0;
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    double* dl = nullptr;
                    auto dh = (double*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) dl = (double*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        dh[i] = (double)0.0;
                        if (data_block->dims[ndim].errasymmetry) dl[i] = (double)0.0;
                    }
                    break;
                }
                case UDA_TYPE_SHORT: {
                    short* sl = nullptr;
                    auto sh = (short*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) sl = (short*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (short)0;
                        if (data_block->dims[ndim].errasymmetry) sl[i] = (short)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_SHORT: {
                    unsigned short* sl = nullptr;
                    auto sh = (unsigned short*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) sl = (unsigned short*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (data_block->dims[ndim].errasymmetry) sl[i] = (unsigned short)0;
                    }
                    break;
                }
                case UDA_TYPE_INT: {
                    int* il = nullptr;
                    auto ih = (int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) il = (int*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        ih[i] = (int)0;
                        if (data_block->dims[ndim].errasymmetry) il[i] = (int)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_INT: {
                    unsigned int* ul = nullptr;
                    auto uh = (unsigned int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) ul = (unsigned int*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        uh[i] = (unsigned int)0;
                        if (data_block->dims[ndim].errasymmetry) ul[i] = (unsigned int)0;
                    }
                    break;
                }
                case UDA_TYPE_LONG: {
                    long* ll = nullptr;
                    auto lh = (long*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) ll = (long*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (long)0;
                        if (data_block->dims[ndim].errasymmetry) ll[i] = (long)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG: {
                    unsigned long* ll = nullptr;
                    auto lh = (unsigned long*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) ll = (unsigned long*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (data_block->dims[ndim].errasymmetry) ll[i] = (unsigned long)0;
                    }
                    break;
                }
                case UDA_TYPE_LONG64: {
                    long long int* ll = nullptr;
                    auto lh = (long long int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) ll = (long long int*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (long long int)0;
                        if (data_block->dims[ndim].errasymmetry) ll[i] = (long long int)0;
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG64: {
                    unsigned long long int* ll = nullptr;
                    auto lh = (unsigned long long int*) data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) ll = (unsigned long long int*) data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int) 0;
                        if (data_block->dims[ndim].errasymmetry) ll[i] = (unsigned long long int) 0;
                    }
                    break;
                }
                case UDA_TYPE_CHAR: {
                    char* cl = nullptr;
                    auto ch = data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) cl = data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        *(ch + i) = ' ';
                        if (data_block->dims[ndim].errasymmetry) *(cl + i) = ' ';
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_CHAR: {
                    unsigned char* cl = nullptr;
                    auto ch = (unsigned char*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) cl = (unsigned char*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (data_block->dims[ndim].errasymmetry) cl[i] = (unsigned char)0;
                    }
                    break;
                }
                case UDA_TYPE_DCOMPLEX: {
                    DCOMPLEX* cl = nullptr;
                    auto ch = (DCOMPLEX*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) cl = (DCOMPLEX*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i].real = (double)0.0;
                        ch[i].imaginary = (double)0.0;
                        if (data_block->dims[ndim].errasymmetry) {
                            cl[i].real = (double)0.0;
                            cl[i].imaginary = (double)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_COMPLEX: {
                    COMPLEX* cl = nullptr;
                    auto ch = (COMPLEX*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) cl = (COMPLEX*)data_block->dims[ndim].errlo;
                    for (int i = 0; i < ndata; i++) {
                        ch[i].real = (float)0.0;
                        ch[i].imaginary = (float)0.0;
                        if (data_block->dims[ndim].errasymmetry) {
                            cl[i].real = (float)0.0;
                            cl[i].imaginary = (float)0.0;
                        }
                    }
                    break;
                }
            }
            return data_block->dims[ndim].errhi;    // Errors are Symmetric at this point
        }
    }
}

//!  Returns a pointer to the requested coordinate error data
/**
\param   handle   The data object handle
\param   ndim  the position of the dimension in the data array - numbering is as data[0][1][2]
\return  a pointer to the data
*/
char* udaGetDimError(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
            return nullptr;
    }
    return udaGetDimAsymmetricError(handle, ndim, true);
}

void udaGetFloatDimAsymmetricError(int handle, int ndim, int above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
            return;
    }

    int ndata = data_block->dims[ndim].dim_n;

    if (data_block->dims[ndim].error_type == UDA_TYPE_UNKNOWN) {
        udaGetDimAsymmetricError(handle, ndim, above);
    }     // Create the Error Data prior to Casting

    switch (data_block->dims[ndim].error_type) {
        case UDA_TYPE_UNKNOWN:
            for (int i = 0; i < ndata; i++) fp[i] = (float)0.0; // No Error Data
            break;
        case UDA_TYPE_FLOAT:
            if (above || !data_block->dims[ndim].errasymmetry) {
                memcpy((void*)fp, (void*)data_block->dims[ndim].errhi,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            } else {
                memcpy((void*)fp, (void*)data_block->dims[ndim].errlo,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            }
            break;
        case UDA_TYPE_DOUBLE: {
            double* dp;                          // Return Zeros if this data is requested unless Error is Modelled
            if (above || !data_block->dims[ndim].errasymmetry) {
                dp = (double*)data_block->dims[ndim].errhi;
            } else {
                dp = (double*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)dp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            short* sp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                sp = (short*)data_block->dims[ndim].errhi;
            } else {
                sp = (short*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                sp = (unsigned short*)data_block->dims[ndim].errhi;
            } else {
                sp = (unsigned short*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case UDA_TYPE_INT: {
            int* ip;
            if (above || !data_block->dims[ndim].errasymmetry) {
                ip = (int*)data_block->dims[ndim].errhi;
            } else {
                ip = (int*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)ip[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            unsigned int* up;
            if (above || !data_block->dims[ndim].errasymmetry) {
                up = (unsigned int*)data_block->dims[ndim].errhi;
            } else {
                up = (unsigned int*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)up[i];
            break;
        }
        case UDA_TYPE_LONG: {
            long* lp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                lp = (long*)data_block->dims[ndim].errhi;
            } else {
                lp = (long*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                lp = (unsigned long*)data_block->dims[ndim].errhi;
            } else {
                lp = (unsigned long*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = data_block->dims[ndim].errhi;
            } else {
                cp = data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (unsigned char*)data_block->dims[ndim].errhi;
            } else {
                cp = (unsigned char*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DCOMPLEX* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (DCOMPLEX*)data_block->dims[ndim].errhi;
            } else {
                cp = (DCOMPLEX*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            COMPLEX* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (COMPLEX*)data_block->dims[ndim].errhi;
            } else {
                cp = (COMPLEX*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
    }
}

//!  Returns coordinate error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   ndim  the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatDimError(int handle, int ndim, float* fp)
{
    udaGetFloatDimAsymmetricError(handle, ndim, true, fp);
}

//!  Returns a pointer to the DATA_SYSTEM Meta Data structure
/** A copy of the \b Data_System database table record
\param   handle   The data object handle
\return  DATA_SYSTEM pointer
*/
DATA_SYSTEM* udaGetDataSystem(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->data_system;
}

//!  Returns a pointer to the SYSTEM_CONFIG Meta Data structure
/** A copy of the \b system_config database table record
\param   handle   The data object handle
\return  SYSTEM_CONFIG pointer
*/
SYSTEM_CONFIG* udaGetSystemConfig(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->system_config;
}

//!  Returns a pointer to the DATA_SOURCE Meta Data structure
/** A copy of the \b data_source database table record - the location of data
\param   handle   The data object handle
\return  DATA_SOURCE pointer
*/
DATA_SOURCE* udaGetDataSource(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->data_source;
}

//!  Returns a pointer to the SIGNAL Meta Data structure
/** A copy of the \b signal database table record
\param   handle   The data object handle
\return  SIGNAL pointer
*/
SIGNAL* udaGetSignal(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->signal_rec;
}

//!  Returns a pointer to the SIGNAL_DESC Meta Data structure
/** A copy of the \b signal_desc database table record - a description of the data signal/object
\param   handle   The data object handle
\return  SIGNAL_DESC pointer
*/
SIGNAL_DESC* udaGetSignalDesc(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->signal_desc;
}

//!  Returns a pointer to the File Format string returned in the DATA_SOURCE metadata record
/** Dependent on the server property \b get_meta
\param   handle   The data object handle
\return  pointer to the data file format
*/
const char* udaGetFileFormat(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    DATA_SOURCE* data_source = udaGetDataSource(handle);
    if (data_source == nullptr) return nullptr;
    return data_source->format;
}

//-----------------------------------------------------------------------------------------------------------
// Various Utilities

void udaInitDataBlock(DATA_BLOCK* str)
{
    initDataBlock(str);
}

void udaInitRequestBlock(REQUEST_BLOCK* str)
{
    initRequestBlock(str);
}

int udaDataCheckSum(void* data, int data_n, int type)
{
    int sum = 0;
    switch (type) {
        case UDA_TYPE_FLOAT: {
            float fsum = 0.0;
            auto dp = (float*)data;
            for (int i = 0; i < data_n; i++) if (std::isfinite(dp[i])) fsum = fsum + dp[i];
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case UDA_TYPE_DOUBLE: {
            double fsum = 0.0;
            auto dp = (double*)data;
            for (int i = 0; i < data_n; i++) if (std::isfinite(dp[i])) fsum = fsum + dp[i];
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case UDA_TYPE_COMPLEX: {
            float fsum = 0.0;
            auto dp = (COMPLEX*)data;
            for (int i = 0; i < data_n; i++)
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            double fsum = 0.0;
            auto dp = (DCOMPLEX*)data;
            for (int i = 0; i < data_n; i++)
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }

        case UDA_TYPE_CHAR: {
            char* dp = (char*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_SHORT: {
            auto dp = (short int*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_INT: {
            int* dp = (int*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_LONG: {
            auto dp = (long*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_LONG64: {
            auto dp = (long long int*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto dp = (unsigned char*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto dp = (unsigned short int*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto dp = (unsigned int*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto dp = (unsigned long*)data;
            for (int i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto dp = (unsigned long long int*) data;
            for (int i = 0; i < data_n; i++) sum = sum + (int) dp[i];
            break;
        }
        default:
            sum = 0;
    }
    return sum;
}

int udaGetDataCheckSum(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) return 0;

    return (udaDataCheckSum((void*)data_block->data, data_block->data_n,
                            data_block->data_type));
}

int udaGetDimDataCheckSum(int handle, int ndim)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) return 0;
    if (ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return 0;
    }

    return (udaDataCheckSum((void*)data_block->dims[ndim].dim, data_block->dims[ndim].dim_n,
                            data_block->dims[ndim].data_type));
}

//===========================================================================================================
// Access to (De)Serialiser

void udaGetClientSerialisedDataBlock(int handle, void** object, size_t* objectSize, char** key, size_t* keySize,
                                      int protocolVersion, NTREE* full_ntree, LOGSTRUCTLIST* log_struct_list,
                                      int private_flags, int malloc_source)
{
    // Extract the serialised Data Block from Cache or serialise it if not cached (hash key in Data Block, empty if not cached)
    // Use Case: extract data in object form for storage in external data object store, e.g. CEPH, HDF5
    /*
     * TODO
     *
     * 1> Add cache key to DATA_BLOCK
     * 2> Investigate creation of cache key when REQUEST_BLOCK is out of scope!
     * 3> is keySize useful if the key is always a string!
     */

#ifndef _WIN32
    char* buffer;
    size_t bufsize = 0;
    FILE* memfile = open_memstream(&buffer, &bufsize);
#else
    FILE* memfile = tmpfile();
#endif

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_ENCODE);

    int token;

    USERDEFINEDTYPELIST* userdefinedtypelist = udaGetUserDefinedTypeList(handle);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    DATA_BLOCK_LIST data_block_list;
    data_block_list.count = 1;
    data_block_list.data = udaGetDataBlock(handle);
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK_LIST, XDR_SEND, &token, logmalloclist, userdefinedtypelist,
              &data_block_list, protocolVersion, log_struct_list, private_flags, malloc_source);

#ifdef _WIN32
    fflush(memfile);
    fseek(memfile, 0, SEEK_END);
    long fsize = ftell(memfile);
    rewind(memfile);

    size_t bufsize = (size_t)fsize;
    char* buffer = (char*)malloc(bufsize);
    fread(buffer, bufsize, 1, memfile);
#endif

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
    fclose(memfile);

    // return the serialised data object and key

    *object = buffer;
    *objectSize = bufsize;
    *key = nullptr;
    *keySize = 0;
}

//---------------------------------------------------------------
// Accessor Functions to General/Arbitrary Data Structures
//----------------------------------------------------------------

int udaSetDataTree(int handle)
{
    auto& instance = uda::client::ThreadClient::instance();

    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return 0;    // Return FALSE
    }
    if (udaGetData(handle) == nullptr) {
        return 0;
    }

    instance.set_full_ntree((NTREE*)udaGetData(handle));
    void* opaque_block = udaGetDataOpaqueBlock(handle);

    instance.set_user_defined_type_list(((GENERAL_BLOCK*)opaque_block)->userdefinedtypelist);
    instance.set_log_malloc_list(((GENERAL_BLOCK*)opaque_block)->logmalloclist);
    setLastMallocIndexValue(&(((GENERAL_BLOCK*)opaque_block)->lastMallocIndex));
    return 1; // Return TRUE
}

// Return a specific data tree

NTREE* udaGetDataTree(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return nullptr;
    }
    return (NTREE*)udaGetData(handle);
}

// Return a user defined data structure definition

USERDEFINEDTYPE* udaGetUserDefinedType(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->userdefinedtype;
}

USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->userdefinedtypelist;
}

LOGMALLOCLIST* udaGetLogMallocList(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->logmalloclist;
}

NTREE* udaFindNTreeStructureDefinition(NTREE* node, const char* target, NTREE* full_ntree)
{
    return findNTreeStructureDefinition(node, target);
}
