#include <uda/client.h>
#include <uda/structured.h>

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

#include <uda/version.h>

#include "client2/generate_errors.hpp"
#include "client2/thread_client.hpp"
#include "clientserver/version.h"
#include "logging/logging.h"
#include "protocol/alloc_data.h"

#ifdef __APPLE__
#  include <cstdlib>
#elif !defined(A64)
#  include <malloc.h>
#endif

using namespace uda::client_server;
// using namespace uda::client;
using namespace uda::logging;
using namespace uda::structures;


//--------------------------------------------------------------------------------------
// C Accessor Routines

// void udaFreeDataBlocks()
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     instance.clear();
// }

// DATA_BLOCK* udaGetCurrentDataBlock()
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     return instance.current_data_block();
// }

// int udaGetNewDataHandle()
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     return instance.new_handle();
// }

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
void udaSetProperty(const char* property)
{
    auto& instance = uda::client::ThreadClient::instance();
    instance.set_property(property);
}

//! Return the value of a named server property
/**
* @param property the name of the property.
* @return Void.
*/
int udaGetProperty(const char* property)
{
    auto& instance = uda::client::ThreadClient::instance();
    return instance.get_property(property);
}

//! Reset a specific named data server property to its default value
/**
* @param property the name of the property.
* @return Void.
*/

void udaResetProperty(const char* property)
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
// const CLIENT_BLOCK* udaGetProperties(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     return instance.client_block(handle);
// }

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
    if (model <= static_cast<int>(ErrorModelType::Unknown)
        || model >= static_cast<int>(ErrorModelType::Undefined)) {
        return;   // No valid Model
    }

    data_block->error_model = model;               // Model ID
    data_block->error_param_n = param_n;             // Number of parameters

    if (param_n > MaxErrParams) {
        data_block->error_param_n = MaxErrParams;
    }

    for (int i = 0; i < data_block->error_param_n; i++) {
        data_block->errparams[i] = params[i];
    }
}

void udaPutDimErrorModel(int handle, int n_dim, int model, int param_n, const float* params)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    if (n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return;                     // No Dim
    }
    if (model <= static_cast<int>(ErrorModelType::Unknown)
        || model >= static_cast<int>(ErrorModelType::Undefined)) {
        return;  // No valid Model
    }

    data_block->dims[n_dim].error_model = model;                        // Model ID
    data_block->dims[n_dim].error_param_n = param_n;                      // Number of parameters

    if (param_n > MaxErrParams) data_block->dims[n_dim].error_param_n = MaxErrParams;
    for (int i = 0; i < data_block->dims[n_dim].error_param_n; i++) {
        data_block->dims[n_dim].errparams[i] = params[i];
    }
}

//! Set the UDA data server host name and port number
/** This takes precedence over the environment variables UDA_HOST and UDA_PORT.
* @param host The name of the server host computer.
* @param port The port number the server is connected to.
* @return void
*/
void udaPutServer(const char* host, int port)
{
    auto& instance = uda::client::ThreadClient::instance();
    // auto environment = instance.environment();
    // environment->server_port = port;                             // UDA server service port number
    // strcpy(environment->server_host, host);                      // UDA server's host name or IP address
    // environment->server_reconnect = 1;                           // Create a new Server instance
    // *env_host = false;                                           // Skip initialisation at Startup if these are called first
    // *env_port = false;

    instance.set_host(host);
    instance.set_port(port);
}

//! Set the UDA data server host name
/** This takes precedence over the environment variables UDA_HOST.
* @param host The name of the server host computer.
* @return void
*/
void udaPutServerHost(const char* host)
{
    auto& instance = uda::client::ThreadClient::instance();
    // auto environment = instance.environment();
    // strcpy(environment->server_host, host);                      // UDA server's host name or IP address
    // environment->server_reconnect = 1;                           // Create a new Server instance
    // *env_host = false;
    instance.set_host(host);
}

//! Set the UDA data server port number
/** This takes precedence over the environment variables UDA_PORT.
* @param port The port number the server is connected to.
* @return void
*/
void udaPutServerPort(int port)
{
    auto& instance = uda::client::ThreadClient::instance();
    // auto environment = instance.environment();
    // environment->server_port = port;                             // UDA server service port number
    // environment->server_reconnect = 1;                           // Create a new Server instance
    // *env_port = false;
    instance.set_port(port);
}

//! Specify a specific UDA server socket connection to use
/** The client can be connected to multiple servers, distinguished by their socket id.
Select the server connection required.
* @param socket The socket ID of the server connection required.
* @return void
*/
// void udaPutServerSocket(int socket)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto environment = instance.environment();
//     if (environment->server_socket != socket) {      // Change to a different socket
//         environment->server_socket = socket;         // UDA server service socket number (Must be Open)
//         environment->server_change_socket = 1;       // Connect to an Existing Server
//     }
// }

//--------------------------------------------------------------
// Standard C GET Accessor Routines

//! Return the UDA data server host name, port number and socket connection id
/**
* @param host A preallocated string that will contain the name of the server host computer.
* @param port Returned port number.
* @param socket Returned socket id number.
* @return void
*/
// void udaGetServer(const char** host, int* port, int* socket)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto environment = instance.environment();
//     *socket = environment->server_socket;                        // UDA server service socket number
//     *port = environment->server_port;                          // UDA server service port number
//     *host = environment->server_host;                          // UDA server's host name or IP address
// }

//! the UDA server connection host name
/**
* @return the Name of the Host
*/
const char* udaGetServerHost()
{
    const auto& instance = uda::client::ThreadClient::instance();
    // auto environment = instance.environment();
    // return environment->server_host;                             // Active UDA server's host name or IP address
    return instance.get_host().c_str();
}

//! the UDA server connection port number
/**
* @return the Name of the Host
*/
int udaGetServerPort()
{
    const auto& instance = uda::client::ThreadClient::instance();
    // auto environment = instance.environment();
    // return environment->server_port;                             // Active UDA server service port number
    return instance.get_port();
}

// const char* getUdaBuildVersion()
// {
//     return UDA_BUILD_VERSION;
// }

int udaGetClientVersion()
{
    const auto& instance = uda::client::ThreadClient::instance();
    return instance.version;
}

#define UDA_VERSION_STRING_LENGTH 256

void udaGetServerVersionString(char* version_string)
{
    const int major_version = udaGetServerVersionMajor();
    const int minor_version = udaGetServerVersionMinor();
    const int bugfix_version = udaGetServerVersionBugfix();
    const int delta_version = udaGetServerVersionDelta();
    snprintf(version_string, UDA_VERSION_STRING_LENGTH, "%d.%d.%d.%d", major_version, minor_version, bugfix_version, delta_version);
}

const char* udaGetBuildDate()
{
    return UDA_BUILD_DATE;
}

int udaGetClientVersionMajor() { return UDA_VERSION_MAJOR; }
int udaGetClientVersionMinor() { return UDA_VERSION_MINOR; }
int udaGetClientVersionBugfix() { return UDA_VERSION_BUGFIX; }
int udaGetClientVersionDelta() { return UDA_VERSION_DELTA; }

int udaGetServerVersionMajor() 
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto& server_block = instance.server_block();
    return UDA_GET_MAJOR_VERSION(server_block->version); 
}

int udaGetServerVersionMinor() 
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto& server_block = instance.server_block();
    return UDA_GET_MINOR_VERSION(server_block->version); 
}

int udaGetServerVersionBugfix() 
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto& server_block = instance.server_block();
    return UDA_GET_BUGFIX_VERSION(server_block->version); 
}

int udaGetServerVersionDelta() 
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto& server_block = instance.server_block();
    return UDA_GET_DELTA_VERSION(server_block->version); 
}

void udaFree(int handle) {
    auto& instance = uda::client::ThreadClient::instance();
    instance.free_handle(handle);
}

void udaFreeAll() {
    auto& instance = uda::client::ThreadClient::instance();
    instance.free_all();
}

//! the UDA server connection socket ID
/**
* @return the connection socket ID
*/
// int udaGetServerSocket()
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto environment = instance.environment();
//     return environment->server_socket;           // Active UDA server service socket number
// }

int udaGetServerErrorStackSize() {
    const auto& instance = uda::client::ThreadClient::instance();
    const auto server_block = instance.server_block();
    return server_block->idamerrorstack.nerrors;
}

//! the Error code of a specific server error record
/**
* @param record the error stack record number
* @return the error code
*/
int udaGetServerErrorStackRecordCode(int record)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto server_block = instance.server_block();

    if (record < 0 || (unsigned int)record >= server_block->idamerrorstack.nerrors) {
        return 0;
    }
    return server_block->idamerrorstack.idamerror[record].code;  // Server Error Stack Record Code
}

const char* udaGetServerErrorStackRecordLocation(int record)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto server_block = instance.server_block();

    if (record < 0 || (unsigned int)record >= server_block->idamerrorstack.nerrors) {
        return 0;
    }
    return server_block->idamerrorstack.idamerror[record].location;  // Server Error Stack Record Code
}

const char* udaGetServerErrorStackRecordMsg(int record)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto server_block = instance.server_block();

    if (record < 0 || (unsigned int)record >= server_block->idamerrorstack.nerrors) {
        return 0;
    }
    return server_block->idamerrorstack.idamerror[record].msg;  // Server Error Stack Record Code
}

//!  returns the data access error code
/**
\param   handle   The data object handle.
\return   Return error code, if non-zero there was a problem: < 0 is client side, > 0 is server side.
*/
int udaGetErrorCode(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);
    const auto server_block = instance.server_block();

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
int udaGetSourceStatus(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    // Signal Status
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->signal_status;
}

int udaGetDataStatus(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    // Data Status based on Standard Rule
    if (data_block == nullptr) {
        return 0;
    }
    if (udaGetSignalStatus(handle) == DefaultStatus) {
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
    const auto& instance = uda::client::ThreadClient::instance();
    return instance.current_data_block()->handle;
}

//!  returns the number of data items in the data object
/** the number of array elements
\param   handle   The data object handle
\return  the number of data items
*/
int udaGetDataNum(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    // Array Rank
    if (data_block == nullptr) {
        return 0;
    }
    return static_cast<int>(data_block->rank);
}

//!  Returns the position of the time coordinate dimension in the data object
/** For example, a rank 3 array data[time][x][y] (in Fortran and IDL this is data(y,x,time)) has time order = 0 so order is
counted from left to right in c and from right to left in Fortran and IDL.
\param   handle   The data object handle
\return  the time coordinate dimension position
*/
int udaGetOrder(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    // Permission to cache?
    if (data_block == nullptr) {
        return static_cast<int>(PluginCachePermission::NotOkToCache);
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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->data_type;
}

int udaGetDataOpaqueType(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->opaque_type;
}

void* udaGetDataOpaqueBlock(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->opaque_block;
}

int udaGetDataOpaqueCount(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        *model = static_cast<int>(ErrorModelType::Unknown);
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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->errasymmetry;
}

// Return the Internal Code for a named Error Model

int udaGetErrorModelId(const char* model)
{
    if (STR_IEQUALS(model, "default")) {
        return static_cast<int>(ErrorModelType::Default);
    }
    if (STR_IEQUALS(model, "default_asymmetric")) {
        return static_cast<int>(ErrorModelType::DefaultAsymmetric);
    }
    if (STR_IEQUALS(model, "gaussian")) {
        return static_cast<int>(ErrorModelType::Gaussian);
    }
    if (STR_IEQUALS(model, "reseed")) {
        return static_cast<int>(ErrorModelType::Reseed);
    }
    if (STR_IEQUALS(model, "gaussian_shift")) {
        return static_cast<int>(ErrorModelType::GaussianShift);
    }
    if (STR_IEQUALS(model, "poisson")) {
        return static_cast<int>(ErrorModelType::Poisson);
    }
    return static_cast<int>(ErrorModelType::Unknown);
}

char* udaGetSyntheticDimData(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].synthetic;
}

void udaSetSyntheticData(int handle, char* data)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->synthetic = data;
}

void udaSetSyntheticDimData(int handle, int n_dim, char* data)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].synthetic = data;
}

char* udaGetSyntheticData(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);
    const auto client_flags = instance.client_flags();

    const int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return nullptr;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return nullptr;
    }
    if (!client_flags->get_synthetic || data_block->error_model == static_cast<int>(ErrorModelType::Unknown)) {
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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);
    const auto client_flags = instance.client_flags();

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    memcpy(data, (void*)data_block->data, (int)data_block->data_n);
}

char* udaGetDataErrLo(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errlo;
}

char* udaGetDataErrHi(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errhi;
}

int udaGetDataErrAsymmetry(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->errasymmetry;
}

void udaSetDataErrAsymmetry(int handle, int asymmetry)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errasymmetry = asymmetry;
};

void udaSetDataErrType(int handle, int type)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->error_type = type;
};

void udaSetDataErrLo(int handle, char* errlo)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->errlo = errlo;
};

char* udaGetDimErrLo(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].errlo;
}

char* udaGetDimErrHi(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[n_dim].errhi;
}

int udaGetDimErrAsymmetry(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    return data_block->dims[n_dim].errasymmetry;
}

void udaSetDimErrAsymmetry(int handle, int n_dim, int asymmetry)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].errasymmetry = asymmetry;
};

void udaSetDimErrType(int handle, int n_dim, int type)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].error_type = type;
};

void udaSetDimErrLo(int handle, int n_dim, char* errlo)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    data_block->dims[n_dim].errlo = errlo;
};

template <typename T>
void generate_asymmetic_errror(DataBlock* data_block, int n_data) {
    T* const low = data_block->errasymmetry ? reinterpret_cast<T*>(data_block->errlo) : nullptr;
    T* const high = reinterpret_cast<T*>(data_block->errhi);
    for (int i = 0; i < n_data; i++) {
        high[i] = T{};
        if (data_block->errasymmetry) {
            low[i] = T{};
        }
    }
}

char* udaGetAsymmetricError(int handle, bool above)
{
    auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
        if (data_block->error_model != static_cast<int>(ErrorModelType::Unknown)) {
            uda::client::generate_data_error(handle);
            if (above) {
                return data_block->errhi;
            } else if (!data_block->errasymmetry) {
                return data_block->errhi;
            } else {
                return data_block->errlo;
            }
        } else {

            char* err_hi = nullptr;    // Regular Error Component
            char* err_lo = nullptr;    // Asymmetric Error Component
            int n_data = data_block->data_n;
            data_block->error_type = data_block->data_type;  // Error Type is Unknown so Assume Data's Data Type

            if (alloc_array(data_block->error_type, n_data, &err_hi) != 0) {
                // Allocate Heap for Regular Error Data
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Data Errors\n");
                data_block->errhi = nullptr;
            } else {
                data_block->errhi = err_hi;
            }

            if (data_block->errasymmetry) {           // Allocate Heap for the Asymmetric Error Data
                if (alloc_array(data_block->error_type, n_data, &err_lo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Asymmetric Errors\n");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    data_block->errlo = nullptr;
                    data_block->errasymmetry = 0;
                } else {
                    data_block->errlo = err_lo;
                }
            }

            // Generate and return Zeros if this data is requested unless Error is Modelled

            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT:
                    generate_asymmetic_errror<float>(data_block, n_data);
                    break;
                case UDA_TYPE_DOUBLE:
                    generate_asymmetic_errror<double>(data_block, n_data);
                    break;
                case UDA_TYPE_SHORT:
                    generate_asymmetic_errror<short>(data_block, n_data);
                    break;
                case UDA_TYPE_UNSIGNED_SHORT:
                    generate_asymmetic_errror<unsigned short>(data_block, n_data);
                    break;
                case UDA_TYPE_INT:
                    generate_asymmetic_errror<int>(data_block, n_data);
                    break;
                case UDA_TYPE_UNSIGNED_INT:
                    generate_asymmetic_errror<unsigned int>(data_block, n_data);
                    break;
                case UDA_TYPE_LONG:
                    generate_asymmetic_errror<long>(data_block, n_data);
                    break;
                case UDA_TYPE_UNSIGNED_LONG:
                    generate_asymmetic_errror<unsigned long>(data_block, n_data);
                    break;
                case UDA_TYPE_LONG64:
                    generate_asymmetic_errror<long long>(data_block, n_data);
                    break;
                case UDA_TYPE_UNSIGNED_LONG64:
                    generate_asymmetic_errror<unsigned long long>(data_block, n_data);
                    break;
                case UDA_TYPE_CHAR:
                    generate_asymmetic_errror<char>(data_block, n_data);
                    break;
                case UDA_TYPE_UNSIGNED_CHAR:
                    generate_asymmetic_errror<unsigned char>(data_block, n_data);
                    break;
                case UDA_TYPE_DCOMPLEX:
                    generate_asymmetic_errror<DComplex>(data_block, n_data);
                    break;
                case UDA_TYPE_COMPLEX:
                    generate_asymmetic_errror<Complex>(data_block, n_data);
                    break;
                default:
                    UDA_LOG(UDA_LOG_ERROR, "Invalid Data Type\n");
                    return nullptr;
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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    constexpr bool above = 1;
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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);
    const auto client_flags = instance.client_flags();

    // Copy Data cast as double to User Provided Array

    // **** The double array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    const int status = udaGetDataStatus(handle);
    if (data_block == nullptr) return;
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) return;
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) return;

    if (data_block->data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->data, static_cast<size_t>(data_block->data_n) * sizeof(double));
        else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->synthetic,
                       static_cast<size_t>(data_block->data_n) * sizeof(double));
            else
                memcpy((void*)fp, (void*)data_block->data,
                       static_cast<size_t>(data_block->data_n) * sizeof(double));
            return;
        }
    } else {

        char* array;

        int n_data = udaGetDataNum(handle);

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
                const auto dp = reinterpret_cast<float*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(dp[i]);
                break;
            }
            case UDA_TYPE_SHORT: {
                const auto sp = reinterpret_cast<short*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(sp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                const auto sp = reinterpret_cast<unsigned short*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(sp[i]);
                break;
            }
            case UDA_TYPE_INT: {
                const auto ip = reinterpret_cast<int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(ip[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                const auto up = reinterpret_cast<unsigned int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(up[i]);
                break;
            }
            case UDA_TYPE_LONG: {
                const auto lp = reinterpret_cast<long*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(lp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                const auto lp = reinterpret_cast<unsigned long*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(lp[i]);
                break;
            }
            case UDA_TYPE_LONG64: {
                const auto lp = reinterpret_cast<long long int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(lp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                const auto lp = reinterpret_cast<unsigned long long int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(lp[i]);
                break;
            }
            case UDA_TYPE_CHAR: {
                const auto cp = array;
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(cp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                const auto cp = reinterpret_cast<unsigned char*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<double>(cp[i]);
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < n_data; i++) fp[i] = 0.0;  // No Data !
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                const auto dp = reinterpret_cast<DComplex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = dp[i].real;
                    fp[j++] = dp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                const auto dp = reinterpret_cast<Complex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = static_cast<double>(dp[i].real);
                    fp[j++] = static_cast<double>(dp[i].imaginary);
                }
                break;
            }
            default:
                for (int i = 0; i < n_data; i++) fp[i] = 0.0;
                break;

        }
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

    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);
    const auto client_flags = instance.client_flags();

    const int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) return;
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) return;

    if (data_block->data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic)
            memcpy((void*)fp, (void*)data_block->data, static_cast<size_t>(data_block->data_n) * sizeof(float));
        else {
            uda::client::generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr)
                memcpy((void*)fp, (void*)data_block->synthetic,
                       static_cast<size_t>(data_block->data_n) * sizeof(float));
            else
                memcpy((void*)fp, (void*)data_block->data,
                       static_cast<size_t>(data_block->data_n) * sizeof(float));
            return;
        }
    } else {

        char* array;

        const int n_data = udaGetDataNum(handle);

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
                const double* dp = reinterpret_cast<double*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(dp[i]);
                break;
            }
            case UDA_TYPE_SHORT: {
                const auto sp = reinterpret_cast<short*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(sp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                const auto sp = reinterpret_cast<unsigned short*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(sp[i]);
                break;
            }
            case UDA_TYPE_INT: {
                const auto ip = reinterpret_cast<int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(ip[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                const auto up = reinterpret_cast<unsigned int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(up[i]);
                break;
            }
            case UDA_TYPE_LONG: {
                const auto lp = reinterpret_cast<long*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(lp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                const auto lp = reinterpret_cast<unsigned long*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(lp[i]);
                break;
            }
            case UDA_TYPE_LONG64: {
                const auto lp = reinterpret_cast<long long int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(lp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                const auto lp = reinterpret_cast<unsigned long long int*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(lp[i]);
                break;
            }
            case UDA_TYPE_CHAR: {
                const auto cp = array;
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(cp[i]);
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                const auto cp = reinterpret_cast<unsigned char*>(array);
                for (int i = 0; i < n_data; i++) fp[i] = static_cast<float>(cp[i]);
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < n_data; i++) fp[i] = 0.0f;   // No Data !
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                const auto dp = reinterpret_cast<DComplex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = static_cast<float>(dp[i].real);
                    fp[j++] = static_cast<float>(dp[i].imaginary);
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                const auto dp = reinterpret_cast<Complex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = dp[i].real;
                    fp[j++] = dp[i].imaginary;
                }
                break;
            }
            default:
                for (int i = 0; i < n_data; i++) fp[i] = 0.0f;
                break;

        }
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
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(float));
            break;
        case UDA_TYPE_DOUBLE:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(double));
            break;
        case UDA_TYPE_INT:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(int));
            break;
        case UDA_TYPE_UNSIGNED_INT:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(unsigned int));
            break;
        case UDA_TYPE_LONG:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(long));
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(unsigned long));
            break;
        case UDA_TYPE_LONG64:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(long long int));
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(unsigned long long int));
            break;
        case UDA_TYPE_SHORT:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(short));
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(unsigned short));
            break;
        case UDA_TYPE_CHAR:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(char));
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(unsigned char));
            break;
        case UDA_TYPE_DCOMPLEX:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(DComplex));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, udaGetData(handle), static_cast<size_t>(udaGetDataNum(handle)) * sizeof(Complex));
            break;
    }
}

template <typename T>
void get_float_asymmetric_error(const bool above, const DataBlock* data_block, const int n_data, float* out)
{
    T* err;
    if (above) {
        err = reinterpret_cast<T*>(data_block->errhi);
    } else if (!data_block->errasymmetry) {
        err = reinterpret_cast<T*>(data_block->errhi);
    } else {
        err = reinterpret_cast<T*>(data_block->errlo);
    }
    for (int i = 0; i < n_data; i++) {
        out[i] = static_cast<float>(err[i]);
    }
}

void udaGetFloatAsymmetricError(int handle, bool above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }

    const int n_data = data_block->data_n;

    if (data_block->error_type == UDA_TYPE_UNKNOWN) {
        udaGetAsymmetricError(handle, above);
    } // Create the Error Data prior to Casting

    switch (data_block->error_type) {
        case UDA_TYPE_UNKNOWN:
            for (int i = 0; i < n_data; i++) {
                fp[i] = 0.0f; // No Error Data
            }
            break;
        case UDA_TYPE_FLOAT:
            if (above) {
                memcpy(fp, data_block->errhi, static_cast<size_t>(data_block->data_n) * sizeof(float));
            } else if (!data_block->errasymmetry) {
                memcpy(fp, data_block->errhi, static_cast<size_t>(data_block->data_n) * sizeof(float));
            } else {
                memcpy(fp, data_block->errlo, static_cast<size_t>(data_block->data_n) * sizeof(float));
            }
            break;
        case UDA_TYPE_DOUBLE:
            get_float_asymmetric_error<double>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_SHORT:
            get_float_asymmetric_error<short>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            get_float_asymmetric_error<unsigned short>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_INT:
            get_float_asymmetric_error<int>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            get_float_asymmetric_error<unsigned int>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_LONG:
            get_float_asymmetric_error<long>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_UNSIGNED_LONG:
            get_float_asymmetric_error<unsigned long>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_LONG64:
            get_float_asymmetric_error<long long>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            get_float_asymmetric_error<unsigned long long>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_CHAR:
            get_float_asymmetric_error<char>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            get_float_asymmetric_error<unsigned char>(above, data_block, n_data, fp);
            break;
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DComplex* cp;
            if (above) {
                cp = reinterpret_cast<DComplex*>(data_block->errhi);
            } else if (!data_block->errasymmetry) {
                cp = reinterpret_cast<DComplex*>(data_block->errhi);
            } else {
                cp = reinterpret_cast<DComplex*>(data_block->errlo);
            }
            for (int i = 0; i < n_data; i++) {
                fp[j++] = static_cast<float>(cp[i].real);
                fp[j++] = static_cast<float>(cp[i].imaginary);
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            Complex* cp;
            if (above) {
                cp = reinterpret_cast<Complex*>(data_block->errhi);
            } else if (!data_block->errasymmetry) {
                cp = reinterpret_cast<Complex*>(data_block->errhi);
            } else {
                cp = reinterpret_cast<Complex*>(data_block->errlo);
            }
            for (int i = 0; i < n_data; i++) {
                fp[j++] = cp[i].real;
                fp[j++] = cp[i].imaginary;
            }
            break;
        }
        default:
            for (int i = 0; i < n_data; i++) {
                fp[i] = 0.0f;
            }
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
    constexpr bool above = 1;
    udaGetFloatAsymmetricError(handle, above, fp);
}

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**
\param   handle   The data object handle
\param   db Returned \b DATA_BLOCK pointer
\return  void
*/
// void udaGetDBlock(int handle, DATA_BLOCK* db)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return;
//     }
//     *db = *data_block;
// }

//!  Returns the data label of a data object
/**
\param   handle   The data object handle
\return  pointer to the data label
*/
const char* udaGetDataLabel(int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

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
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return;
    }
    strcpy(desc, data_block->data_desc);
}

// Dimension Coordinates

//! Returns the coordinate dimension size
/** the number of elements in the coordinate array
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the dimension size
*/
int udaGetDimNum(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return 0;
    }
    return data_block->dims[n_dim].dim_n;
}

//! Returns the coordinate dimension data type
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int udaGetDimType(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->dims[n_dim].data_type;
}

//! Returns the coordinate dimension error data type
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int udaGetDimErrorType(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->dims[n_dim].error_type;
}

//! Returns whether or not coordinate error data are asymmetric.
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  boolean true or false i.e. 1 or 0
*/
int udaGetDimErrorAsymmetry(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return 0;
    }
    return data_block->dims[n_dim].errasymmetry;
}

void udaGetDimErrorModel(int handle, int n_dim, int* model, int* param_n, float* params)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        *model = static_cast<int>(ErrorModelType::Unknown);
        *param_n = 0;
        return;
    }

    *model = data_block->dims[n_dim].error_model;      // Model ID
    *param_n = data_block->dims[n_dim].error_param_n;    // Number of parameters
    for (int i = 0; i < data_block->dims[n_dim].error_param_n; i++) {
        params[i] = data_block->dims[n_dim].errparams[i];
    }
    // *params  = data_block->dims[n_dim].errparams;        // Array of Model Parameters
}

char* udaGenerateSyntheticDimData(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return nullptr;
    }

    const auto client_flags = instance.client_flags();
    if (!client_flags->get_synthetic || data_block->dims[n_dim].error_model == static_cast<int>(ErrorModelType::Unknown)) {
        return data_block->dims[n_dim].dim;
    }
    uda::client::generate_synthetic_dim_data(handle, n_dim);
    return data_block->dims[n_dim].synthetic;
}

///!  Returns a pointer to the requested coordinate data
/** The data may be synthetically generated.
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data
*/
char* udaGetDimData(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
        return nullptr;
    }

    const auto client_flags = instance.client_flags();
    if (!client_flags->get_synthetic) {
        return data_block->dims[n_dim].dim;
    }
    return udaGetSyntheticDimData(handle, n_dim);
}

//! Returns the data label of a coordinate dimension
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data label
*/
const char* udaGetDimLabel(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return nullptr;
    }
    return data_block->dims[n_dim].dim_label;
}
//! Returns the data units of a coordinate dimension
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data units
*/
const char* udaGetDimUnits(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return nullptr;
    }
    return data_block->dims[n_dim].dim_units;
}

//!  Returns the data label of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   label   preallocated string buffer to receive the copy of the data label
\return  void
*/
void udaGetDimLabelTdi(int handle, int n_dim, char* label)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return;
    }
    strcpy(label, data_block->dims[n_dim].dim_label);
}

//!  Returns the data units of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   units   preallocated string buffer to receive the copy of the data units
\return  void
*/
void udaGetDimUnitsTdi(int handle, int n_dim, char* units)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return;
    }
    strcpy(units, data_block->dims[n_dim].dim_units);
}

template <typename T>
void get_double_dim_data(const char* array, const int n_data, double* out) {
    const auto ptr = reinterpret_cast<const T*>(array);
    for (int i = 0; i < n_data; i++) {
        out[i] = static_cast<double>(ptr[i]);
    }
}

//!  Returns coordinate data cast to double precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b double pointer to a preallocated data buffer
\return  void
*/
void udaGetDoubleDimData(int handle, int n_dim, double* fp)
{
    // **** The double array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return;
    }

    const auto client_flags = instance.client_flags();
    if (data_block->dims[n_dim].data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic) {
            memcpy(fp, data_block->dims[n_dim].dim, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(double));
        } else {
            uda::client::generate_synthetic_dim_data(handle, n_dim);
            if (data_block->dims[n_dim].synthetic != nullptr) {
                memcpy(fp, data_block->dims[n_dim].synthetic, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(double));
            } else {
                memcpy(fp, data_block->dims[n_dim].dim, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(double));
            }
        }
    } else {
        char* array;

        const int n_data = data_block->dims[n_dim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[n_dim].dim;
        } else {
            uda::client::generate_synthetic_dim_data(handle, n_dim);
            if (data_block->dims[n_dim].synthetic != nullptr) {
                array = data_block->dims[n_dim].synthetic;
            } else {
                array = data_block->dims[n_dim].dim;
            }
        }

        switch (data_block->dims[n_dim].data_type) {
            case UDA_TYPE_FLOAT: get_double_dim_data<float>(array, n_data, fp); break;
            case UDA_TYPE_SHORT: get_double_dim_data<short>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_SHORT: get_double_dim_data<unsigned short>(array, n_data, fp); break;
            case UDA_TYPE_INT: get_double_dim_data<int>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_INT: get_double_dim_data<unsigned int>(array, n_data, fp); break;
            case UDA_TYPE_LONG: get_double_dim_data<long>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_LONG: get_double_dim_data<unsigned long>(array, n_data, fp); break;
            case UDA_TYPE_LONG64: get_double_dim_data<long long>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_LONG64: get_double_dim_data<unsigned long long>(array, n_data, fp); break;
            case UDA_TYPE_CHAR: get_double_dim_data<char>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_CHAR: get_double_dim_data<unsigned char>(array, n_data, fp); break;
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                const auto cp = reinterpret_cast<DComplex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = cp[i].real;
                    fp[j++] = cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                const auto cp = reinterpret_cast<Complex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = static_cast<double>(cp[i].real);
                    fp[j++] = static_cast<double>(cp[i].imaginary);
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
            default:
                for (int i = 0; i < n_data; i++) fp[i] = 0.0;
                break;

        }
    }
}

template <typename T>
void get_float_dim_data(const char* array, const int n_data, float* out) {
    const auto ptr = reinterpret_cast<const T*>(array);
    for (int i = 0; i < n_data; i++) {
        out[i] = static_cast<float>(ptr[i]);
    }
}

//!  Returns coordinate data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatDimData(int handle, int n_dim, float* fp)
{
    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
            return;
    }

    const auto client_flags = instance.client_flags();
    if (data_block->dims[n_dim].data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic)
            memcpy(fp, data_block->dims[n_dim].dim, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(float));
        else {
            uda::client::generate_synthetic_dim_data(handle, n_dim);
            if (data_block->dims[n_dim].synthetic != nullptr) {
                memcpy(fp, data_block->dims[n_dim].synthetic, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(float));
            } else {
                memcpy(fp, data_block->dims[n_dim].dim, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(float));
            }
        }
    } else {
        char* array;

        const int n_data = data_block->dims[n_dim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[n_dim].dim;
        } else {
            uda::client::generate_synthetic_dim_data(handle, n_dim);
            if (data_block->dims[n_dim].synthetic != nullptr) {
                array = data_block->dims[n_dim].synthetic;
            } else {
                array = data_block->dims[n_dim].dim;
            }
        }

        switch (data_block->dims[n_dim].data_type) {
            case UDA_TYPE_DOUBLE: get_float_dim_data<double>(array, n_data, fp); break;
            case UDA_TYPE_SHORT: get_float_dim_data<short>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_SHORT: get_float_dim_data<unsigned short>(array, n_data, fp); break;
            case UDA_TYPE_INT: get_float_dim_data<int>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_INT: get_float_dim_data<unsigned int>(array, n_data, fp); break;
            case UDA_TYPE_LONG: get_float_dim_data<long>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_LONG: get_float_dim_data<unsigned long>(array, n_data, fp); break;
            case UDA_TYPE_LONG64: get_float_dim_data<long long>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_LONG64: get_float_dim_data<unsigned long long>(array, n_data, fp); break;
            case UDA_TYPE_CHAR: get_float_dim_data<char>(array, n_data, fp); break;
            case UDA_TYPE_UNSIGNED_CHAR: get_float_dim_data<unsigned char>(array, n_data, fp); break;
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                const auto cp = reinterpret_cast<DComplex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = static_cast<float>(cp[i].real);
                    fp[j++] = static_cast<float>(cp[i].imaginary);
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                const auto cp = reinterpret_cast<Complex*>(array);
                for (int i = 0; i < n_data; i++) {
                    fp[j++] = cp[i].real;
                    fp[j++] = cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
            default:
                for (int i = 0; i < n_data; i++) fp[i] = 0.0f;
                break;
        }
    }
}

//!  Returns coordinate data as void type
/** The copy buffer must be preallocated and sized for the correct data type.
\param   handle   The data object handle
\param   n_dim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   data  A \b void pointer to a preallocated data buffer
\return  void
*/
void udaGetGenericDimData(int handle, int n_dim, void* data)
{
    const auto type = static_cast<UdaType>(udaGetDimType(handle, n_dim));
    const auto dim_data = udaGetDimData(handle, n_dim);
    const auto count = static_cast<size_t>(udaGetDimNum(handle, n_dim));
    memcpy(data, dim_data, count * getSizeOf(type));
}

template <typename T>
void generate_dim_asymmetric_error(DataBlock* data_block, int n_dim, int n_data) {
    const int err_asymmetry = data_block->dims[n_dim].errasymmetry;
    T* low = err_asymmetry ? reinterpret_cast<T*>(data_block->dims[n_dim].errlo) : nullptr;
    T* high = reinterpret_cast<T*>(data_block->dims[n_dim].errhi);
    for (int i = 0; i < n_data; i++) {
        high[i] = 0.0f;
        if (err_asymmetry) {
            low[i] = 0.0f;
        }
    }
}

char* udaGetDimAsymmetricError(int handle, int n_dim, bool above)
{
    auto& instance = uda::client::ThreadClient::instance();
    auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
            return nullptr;
    }
    if (data_block->dims[n_dim].error_type != UDA_TYPE_UNKNOWN) {
        if (above) {
            return data_block->dims[n_dim].errhi;    // return the default error array
        } else {
            if (!data_block->dims[n_dim].errasymmetry) {
                return data_block->dims[n_dim].errhi;   // return the default error array if symmetric errors
            } else {
                return data_block->dims[n_dim].errlo;
            }   // otherwise the data array must have been returned by the server
        }                           // or generated in a previous call
    } else {
        if (data_block->dims[n_dim].error_model != static_cast<int>(ErrorModelType::Unknown)) {
            uda::client::generate_dim_data_error(handle, n_dim);
            if (above || !data_block->dims[n_dim].errasymmetry) {
                return data_block->dims[n_dim].errhi;
            } else {
                return data_block->dims[n_dim].errlo;
            }
        } else {
            char* err_hi = nullptr;
            char* err_lo = nullptr;

            const int n_data = data_block->dims[n_dim].dim_n;
            data_block->dims[n_dim].error_type = data_block->dims[n_dim].data_type; // Error Type is Unknown so Assume Data's Data Type

            if (alloc_array(data_block->dims[n_dim].error_type, n_data, &err_hi) != 0) {
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Data Errors\n");
                data_block->dims[n_dim].errhi = nullptr;
            } else {
                data_block->dims[n_dim].errhi = err_hi;
            }

            if (data_block->dims[n_dim].errasymmetry) {               // Allocate Heap for the Asymmetric Error Data
                if (alloc_array(data_block->dims[n_dim].error_type, n_data, &err_lo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Asymmetric Errors\n");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    data_block->dims[n_dim].errlo = err_lo;
                    data_block->dims[n_dim].errasymmetry = 0;
                } else {
                    data_block->dims[n_dim].errlo = err_lo;
                }
            }

            switch (data_block->dims[n_dim].data_type) {
                case UDA_TYPE_FLOAT: generate_dim_asymmetric_error<float>(data_block, n_dim, n_data); break;
                case UDA_TYPE_DOUBLE: generate_dim_asymmetric_error<double>(data_block, n_dim, n_data); break;
                case UDA_TYPE_SHORT: generate_dim_asymmetric_error<short>(data_block, n_dim, n_data); break;
                case UDA_TYPE_UNSIGNED_SHORT: generate_dim_asymmetric_error<unsigned short>(data_block, n_dim, n_data); break;
                case UDA_TYPE_INT: generate_dim_asymmetric_error<int>(data_block, n_dim, n_data); break;
                case UDA_TYPE_UNSIGNED_INT: generate_dim_asymmetric_error<unsigned int>(data_block, n_dim, n_data); break;
                case UDA_TYPE_LONG: generate_dim_asymmetric_error<long>(data_block, n_dim, n_data); break;
                case UDA_TYPE_UNSIGNED_LONG: generate_dim_asymmetric_error<unsigned long>(data_block, n_dim, n_data); break;
                case UDA_TYPE_LONG64: generate_dim_asymmetric_error<long long>(data_block, n_dim, n_data); break;
                case UDA_TYPE_UNSIGNED_LONG64: generate_dim_asymmetric_error<unsigned long long>(data_block, n_dim, n_data); break;
                case UDA_TYPE_CHAR: generate_dim_asymmetric_error<char>(data_block, n_dim, n_data); break;
                case UDA_TYPE_UNSIGNED_CHAR: generate_dim_asymmetric_error<unsigned char>(data_block, n_dim, n_data); break;
                case UDA_TYPE_DCOMPLEX: {
                    DComplex* cl = nullptr;
                    const auto ch = reinterpret_cast<DComplex*>(data_block->dims[n_dim].errhi);
                    if (data_block->dims[n_dim].errasymmetry) cl = reinterpret_cast<DComplex*>(data_block->dims[n_dim].errlo);
                    for (int i = 0; i < n_data; i++) {
                        ch[i].real = 0.0;
                        ch[i].imaginary = 0.0;
                        if (data_block->dims[n_dim].errasymmetry) {
                            cl[i].real = 0.0;
                            cl[i].imaginary = 0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_COMPLEX: {
                    Complex* cl = nullptr;
                    const auto ch = reinterpret_cast<Complex*>(data_block->dims[n_dim].errhi);
                    if (data_block->dims[n_dim].errasymmetry) cl = reinterpret_cast<Complex*>(data_block->dims[n_dim].errlo);
                    for (int i = 0; i < n_data; i++) {
                        ch[i].real = 0.0f;
                        ch[i].imaginary = 0.0f;
                        if (data_block->dims[n_dim].errasymmetry) {
                            cl[i].real = 0.0f;
                            cl[i].imaginary = 0.0f;
                        }
                    }
                    break;
                }
            }
            return data_block->dims[n_dim].errhi;    // Errors are Symmetric at this point
        }
    }
}

//!  Returns a pointer to the requested coordinate error data
/**
\param   handle   The data object handle
\param   n_dim  the position of the dimension in the data array - numbering is as data[0][1][2]
\return  a pointer to the data
*/
char* udaGetDimError(int handle, int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
            return nullptr;
    }
    return udaGetDimAsymmetricError(handle, n_dim, true);
}

template <typename T>
void get_float_dim_asymmetric_error(const bool above, const DataBlock* data_block, const int n_dim, const int n_data, float* out) {
    T* dp;
    if (above || !data_block->dims[n_dim].errasymmetry) {
        dp = reinterpret_cast<T*>(data_block->dims[n_dim].errhi);
    } else {
        dp = reinterpret_cast<T*>(data_block->dims[n_dim].errlo);
    }
    for (int i = 0; i < n_data; i++) {
        out[i] = static_cast<float>(dp[i]);
    }
}

void udaGetFloatDimAsymmetricError(int handle, int n_dim, bool above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr || n_dim < 0 || (unsigned int)n_dim >= data_block->rank) {
            return;
    }

    const int n_data = data_block->dims[n_dim].dim_n;

    if (data_block->dims[n_dim].error_type == UDA_TYPE_UNKNOWN) {
        udaGetDimAsymmetricError(handle, n_dim, above);
    }     // Create the Error Data prior to Casting

    switch (data_block->dims[n_dim].error_type) {
        case UDA_TYPE_FLOAT:
            if (above || !data_block->dims[n_dim].errasymmetry) {
                memcpy(fp, data_block->dims[n_dim].errhi, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(float));
            } else {
                memcpy(fp, data_block->dims[n_dim].errlo, static_cast<size_t>(data_block->dims[n_dim].dim_n) * sizeof(float));
            }
            break;
        case UDA_TYPE_DOUBLE: get_float_dim_asymmetric_error<double>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_SHORT: get_float_dim_asymmetric_error<short>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_UNSIGNED_SHORT: get_float_dim_asymmetric_error<unsigned short>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_INT: get_float_dim_asymmetric_error<int>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_UNSIGNED_INT: get_float_dim_asymmetric_error<unsigned int>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_LONG: get_float_dim_asymmetric_error<long>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_UNSIGNED_LONG: get_float_dim_asymmetric_error<unsigned long>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_CHAR: get_float_dim_asymmetric_error<char>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_UNSIGNED_CHAR: get_float_dim_asymmetric_error<unsigned char>(above, data_block, n_dim, n_data, fp); break;
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DComplex* cp;
            if (above || !data_block->dims[n_dim].errasymmetry) {
                cp = reinterpret_cast<DComplex*>(data_block->dims[n_dim].errhi);
            } else {
                cp = reinterpret_cast<DComplex*>(data_block->dims[n_dim].errlo);
            }
            for (int i = 0; i < n_data; i++) {
                fp[j++] = static_cast<float>(cp[i].real);
                fp[j++] = static_cast<float>(cp[i].imaginary);
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            Complex* cp;
            if (above || !data_block->dims[n_dim].errasymmetry) {
                cp = reinterpret_cast<Complex*>(data_block->dims[n_dim].errhi);
            } else {
                cp = reinterpret_cast<Complex*>(data_block->dims[n_dim].errlo);
            }
            for (int i = 0; i < n_data; i++) {
                fp[j++] = cp[i].real;
                fp[j++] = cp[i].imaginary;
            }
            break;
        }
        case UDA_TYPE_UNKNOWN:
        default:
            for (int i = 0; i < n_data; i++) {
                fp[i] = 0.0f; // No Error Data
            }
            break;
    }
}

//!  Returns coordinate error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   n_dim  the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void udaGetFloatDimError(int handle, int n_dim, float* fp)
{
    udaGetFloatDimAsymmetricError(handle, n_dim, true, fp);
}

// //!  Returns a pointer to the DATA_SYSTEM Meta Data structure
// /** A copy of the \b Data_System database table record
// \param   handle   The data object handle
// \return  DATA_SYSTEM pointer
// */
// DATA_SYSTEM* udaGetDataSystem(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     return data_block->data_system;
// }

// //!  Returns a pointer to the SYSTEM_CONFIG Meta Data structure
// /** A copy of the \b system_config database table record
// \param   handle   The data object handle
// \return  SYSTEM_CONFIG pointer
// */
// SYSTEM_CONFIG* udaGetSystemConfig(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     return data_block->system_config;
// }

// //!  Returns a pointer to the DATA_SOURCE Meta Data structure
// /** A copy of the \b data_source database table record - the location of data
// \param   handle   The data object handle
// \return  DATA_SOURCE pointer
// */
// DATA_SOURCE* udaGetDataSource(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     return data_block->data_source;
// }

// //!  Returns a pointer to the SIGNAL Meta Data structure
// /** A copy of the \b signal database table record
// \param   handle   The data object handle
// \return  SIGNAL pointer
// */
// SIGNAL* udaGetSignal(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     return data_block->signal_rec;
// }

// //!  Returns a pointer to the SIGNAL_DESC Meta Data structure
// /** A copy of the \b signal_desc database table record - a description of the data signal/object
// \param   handle   The data object handle
// \return  SIGNAL_DESC pointer
// */
// SIGNAL_DESC* udaGetSignalDesc(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     return data_block->signal_desc;
// }

// //!  Returns a pointer to the File Format string returned in the DATA_SOURCE metadata record
// /** Dependent on the server property \b get_meta
// \param   handle   The data object handle
// \return  pointer to the data file format
// */
// const char* udaGetFileFormat(int handle)
// {
//     auto& instance = uda::client::ThreadClient::instance();
//     auto data_block = instance.data_block(handle);
//
//     if (data_block == nullptr) {
//         return nullptr;
//     }
//     DataSource* data_source = udaGetDataSource(handle);
//     if (data_source == nullptr) return nullptr;
//     return data_source->format;
// }

//-----------------------------------------------------------------------------------------------------------
// Various Utilities
//
// void udaInitDataBlock(DATA_BLOCK* str)
// {
//     initDataBlock(str);
// }

// void udaInitRequestBlock(REQUEST_BLOCK* str)
// {
//     initRequestBlock(str);
// }

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, int>
data_check_sum(const char* data, const int data_n) {
    T t_sum = 0.0;
    auto dp = reinterpret_cast<const T*>(data);
    for (int i = 0; i < data_n; i++) {
        if (std::isfinite(dp[i])) {
            t_sum = t_sum + dp[i];
        }
    }
    int sum = static_cast<int>(t_sum);
    if (sum == 0) {
        sum = static_cast<int>(1000000.0 * t_sum);      // Rescale
    }
    return sum;
}

template <typename T>
std::enable_if_t<!std::is_floating_point_v<T>, int>
data_check_sum(const char* data, const int data_n) {
    auto dp = reinterpret_cast<const T*>(data);
    int sum = 0;
    for (int i = 0; i < data_n; i++) {
        sum = sum + static_cast<int>(dp[i]);
    }
    return sum;
}

int udaDataCheckSum(const char* data, const int data_n, const int type)
{
    int sum = 0;
    switch (type) {
        case UDA_TYPE_FLOAT: data_check_sum<float>(data, data_n); break;
        case UDA_TYPE_DOUBLE: data_check_sum<double>(data, data_n); break;
        case UDA_TYPE_CHAR: data_check_sum<char>(data, data_n); break;
        case UDA_TYPE_SHORT: data_check_sum<short>(data, data_n); break;
        case UDA_TYPE_INT: data_check_sum<int>(data, data_n); break;
        case UDA_TYPE_LONG: data_check_sum<long>(data, data_n); break;
        case UDA_TYPE_LONG64: data_check_sum<long long>(data, data_n); break;
        case UDA_TYPE_UNSIGNED_CHAR: data_check_sum<unsigned char>(data, data_n); break;
        case UDA_TYPE_UNSIGNED_SHORT: data_check_sum<unsigned short>(data, data_n); break;
        case UDA_TYPE_UNSIGNED_INT: data_check_sum<unsigned int>(data, data_n); break;
        case UDA_TYPE_UNSIGNED_LONG: data_check_sum<unsigned long>(data, data_n); break;
        case UDA_TYPE_UNSIGNED_LONG64: data_check_sum<unsigned long long>(data, data_n); break;
        case UDA_TYPE_COMPLEX: {
            float f_sum = 0.0;
            const auto dp = reinterpret_cast<const Complex*>(data);
            for (int i = 0; i < data_n; i++)
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    f_sum = f_sum + dp[i].real + dp[i].imaginary;
                }
            sum = static_cast<int>(f_sum);
            if (sum == 0) {
                sum = static_cast<int>(1000000.0 * f_sum);      // Rescale
            }
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            double f_sum = 0.0;
            const auto dp = reinterpret_cast<const DComplex*>(data);
            for (int i = 0; i < data_n; i++)
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    f_sum = f_sum + dp[i].real + dp[i].imaginary;
                }
            sum = static_cast<int>(f_sum);
            if (sum == 0) {
                sum = static_cast<int>(1000000.0 * f_sum);      // Rescale
            }
            break;
        }
        default:
            sum = 0;
    }
    return sum;
}

int udaGetDataCheckSum(const int handle)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) return 0;

    return udaDataCheckSum(data_block->data, data_block->data_n, data_block->data_type);
}

int udaGetDimDataCheckSum(const int handle, const int n_dim)
{
    const auto& instance = uda::client::ThreadClient::instance();
    const auto data_block = instance.data_block(handle);

    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) return 0;
    if (n_dim < 0 || static_cast<unsigned int>(n_dim) >= data_block->rank) {
        return 0;
    }

    return udaDataCheckSum(data_block->dims[n_dim].dim, data_block->dims[n_dim].dim_n,
                            data_block->dims[n_dim].data_type);
}

//---------------------------------------------------------------
// Accessor Functions to General/Arbitrary Data Structures
//----------------------------------------------------------------

int udaSetDataTree(const int handle)
{
    auto& instance = uda::client::ThreadClient::instance();

    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return 0;    // Return FALSE
    }
    if (udaGetData(handle) == nullptr) {
        return 0;
    }

    instance.set_full_ntree(reinterpret_cast<NTREE*>(udaGetData(handle)));
    void* opaque_block = udaGetDataOpaqueBlock(handle);

    auto general_block = static_cast<GeneralBlock*>(opaque_block);
    instance.set_user_defined_type_list(general_block->userdefinedtypelist);
    instance.set_log_malloc_list(general_block->logmalloclist);
    udaSetLastMallocIndexValue(&general_block->lastMallocIndex);
    return 1; // Return TRUE
}

// Return a specific data tree

NTREE* udaGetDataTree(const int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return nullptr;
    }
    return reinterpret_cast<NTREE*>(udaGetData(handle));
}

// Return a user defined data structure definition

USERDEFINEDTYPE* udaGetUserDefinedType(const int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return static_cast<GeneralBlock*>(opaque_block)->userdefinedtype;
}

USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(const int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return static_cast<GeneralBlock*>(opaque_block)->userdefinedtypelist;
}

LOGMALLOCLIST* udaGetLogMallocList(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) return nullptr;
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return static_cast<GeneralBlock*>(opaque_block)->logmalloclist;
}

// NTREE* udaFindNTreeStructureDefinition(const char* target, NTREE* full_ntree)
// {
//     return findNTreeStructureDefinition(full_ntree, target);
// }
