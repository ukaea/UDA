#include <uda/client.h>
#include <uda/structured.h>

#include <cmath>

#ifdef __GNUC__
#  include <strings.h>
#else
#  include <Windows.h>
#  include <string.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#  define strlwr _strlwr
#endif

#include "client/generateErrors.h"
#include "client/udaClient.h"
#include "clientserver/allocData.h"
#include "clientserver/initStructs.h"
#include "clientserver/memstream.h"
#include "protocol/protocol.h"
#include "common/string_utils.h"
#include "protocol/xdrlib.h"
#include "logging/logging.h"
#include "client/client_config.h"

#include <uda/version.h>

#ifdef __APPLE__
#  include <cstdlib>
#elif !defined(A64)
#  include <malloc.h>
#endif

using namespace uda::client_server;
using namespace uda::client;
using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;

using namespace std::string_literals;

USERDEFINEDTYPE* udaGetUserDefinedType(int handle);
USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle);
LOGMALLOCLIST* udaGetLogMallocList(int handle);

//---------------------------- Mutex locking for thread safety -------------------------
// State variable sets should be collected and managed for each individual thread!

static int idamThreadLastHandle = -1;

int udaGetThreadLastHandle()
{
    return idamThreadLastHandle;
}

void udaPutThreadLastHandle(int handle)
{
    idamThreadLastHandle = handle;
}

//--------------------------------------------------------------------------------------
// C Accessor Routines

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

//! Set a client_flags->flags property
/** Set a/multiple specific bit/s in the client_flags->flags property sent to the UDA server.
 *
 * @param flag The bit/s to be set to 1.
 * @return Void.
 */

void udaSetClientFlag(unsigned int flag)
{
    auto client_flags = udaClientFlags();
    client_flags->flags = client_flags->flags | flag;
}

//! Reset a client_flags->flags property
/** Reset a/multiple specific bit/s in the client_flags->flags property sent to the UDA server.
 *
 * @param flag The bit/s to be set to 0.
 * @return Void.
 */

void udaResetClientFlag(unsigned int flag)
{
    auto client_flags = udaClientFlags();
    client_flags->flags &= !flag;
}

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
    unsigned int* private_flags = udaPrivateFlags();
    *private_flags |= flag;
}

//! Reset a private_flags property
/** Reset a/multiple specific bit/s in the private_flags property sent between UDA servers.
 *
 * @param flag The bit/s to be set to 0.
 * @return Void.
 */

void udaResetPrivateFlag(unsigned int flag)
{
    unsigned int* private_flags = udaPrivateFlags();
    *private_flags &= !flag;
}

//--------------------------------------------------------------
// Set Server Properties

//! Set a named server property
/** Set a variety of data server properties using their name. These affect the data type returned and any server side
 * processing of data. Not all data access plugins respond to these properties.\n
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
    auto client_flags = udaClientFlags();

    // User settings for Client and Server behaviour

    char name[56];
    char* value;

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            client_flags->get_datadble = 1;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            client_flags->get_dimdble = 1;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            client_flags->get_timedble = 1;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            client_flags->get_bytes = 1;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            client_flags->get_bad = 1;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            client_flags->get_meta = 1;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            client_flags->get_asis = 1;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            client_flags->get_uncal = 1;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            client_flags->get_notoff = 1;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            client_flags->get_synthetic = 1;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            client_flags->get_scalar = 1;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            client_flags->get_nodimdata = 1;
        }
    } else {
        if (property[0] == 't') {
            copy_string(property, name, 56);
            trim_string(name);
            left_trim_string(name);
            mid_trim_string(name);
            strlwr(name);
            if ((value = strstr(name, "timeout=")) != nullptr) {
                value = name + 8;
                if (is_number(value)) {
                    client_flags->user_timeout = atoi(value);
                }
            }
        } else {
            if (STR_IEQUALS(property, "verbose")) {
                set_log_level(UDA_LOG_INFO);
            }
            if (STR_IEQUALS(property, "debug")) {
                set_log_level(UDA_LOG_DEBUG);
            }
            if (STR_IEQUALS(property, "altData")) {
                client_flags->flags = client_flags->flags | client_flags::AltData;
            }
            if (!strncasecmp(property, "altRank", 7)) {
                copy_string(property, name, 56);
                trim_string(name);
                left_trim_string(name);
                mid_trim_string(name);
                strlwr(name);
                if ((value = strcasestr(name, "altRank=")) != nullptr) {
                    value = name + 8;
                    if (is_number(value)) {
                        client_flags->alt_rank = atoi(value);
                    }
                }
            }
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            client_flags->flags = client_flags->flags | client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            client_flags->flags = client_flags->flags | client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            client_flags->flags = client_flags->flags | client_flags::FileCache;
        }
    }
}

//! Return the value of a named server property
/**
 * @param property the name of the property.
 * @return Void.
 */
int udaGetProperty(const char* property)
{
    auto client_flags = udaClientFlags();

    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            return client_flags->get_datadble;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            return client_flags->get_dimdble;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            return client_flags->get_timedble;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            return client_flags->get_bytes;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            return client_flags->get_bad;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            return client_flags->get_meta;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            return client_flags->get_asis;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            return client_flags->get_uncal;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            return client_flags->get_notoff;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            return client_flags->get_synthetic;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            return client_flags->get_scalar;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            return client_flags->get_nodimdata;
        }
    } else {
        if (STR_IEQUALS(property, "timeout")) {
            return client_flags->user_timeout;
        }
        if (STR_IEQUALS(property, "altRank")) {
            return client_flags->alt_rank;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            return (int)(client_flags->flags & client_flags::ReuseLastHandle);
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            return (int)(client_flags->flags & client_flags::FreeReuseLastHandle);
        }
        if (STR_IEQUALS(property, "verbose")) {
            return get_log_level() == UDA_LOG_INFO;
        }
        if (STR_IEQUALS(property, "debug")) {
            return get_log_level() == UDA_LOG_DEBUG;
        }
        if (STR_IEQUALS(property, "altData")) {
            return (int)(client_flags->flags & client_flags::AltData);
        }
        if (STR_IEQUALS(property, "fileCache")) {
            return (int)(client_flags->flags & client_flags::FileCache);
        }
    }
    return 0;
}

//! Reset a specific named data server property to its default value
/**
 * @param property the name of the property.
 * @return Void.
 */

void udaResetProperty(const char* property)
{
    auto client_flags = udaClientFlags();

    // User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) {
            client_flags->get_datadble = 0;
        }
        if (STR_IEQUALS(property, "get_dimdble")) {
            client_flags->get_dimdble = 0;
        }
        if (STR_IEQUALS(property, "get_timedble")) {
            client_flags->get_timedble = 0;
        }
        if (STR_IEQUALS(property, "get_bytes")) {
            client_flags->get_bytes = 0;
        }
        if (STR_IEQUALS(property, "get_bad")) {
            client_flags->get_bad = 0;
        }
        if (STR_IEQUALS(property, "get_meta")) {
            client_flags->get_meta = 0;
        }
        if (STR_IEQUALS(property, "get_asis")) {
            client_flags->get_asis = 0;
        }
        if (STR_IEQUALS(property, "get_uncal")) {
            client_flags->get_uncal = 0;
        }
        if (STR_IEQUALS(property, "get_notoff")) {
            client_flags->get_notoff = 0;
        }
        if (STR_IEQUALS(property, "get_synthetic")) {
            client_flags->get_synthetic = 0;
        }
        if (STR_IEQUALS(property, "get_scalar")) {
            client_flags->get_scalar = 0;
        }
        if (STR_IEQUALS(property, "get_nodimdata")) {
            client_flags->get_nodimdata = 0;
        }
    } else {
        if (STR_IEQUALS(property, "verbose")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "debug")) {
            set_log_level(UDA_LOG_NONE);
        }
        if (STR_IEQUALS(property, "altData")) {
            client_flags->flags &= !client_flags::AltData;
        }
        if (STR_IEQUALS(property, "altRank")) {
            client_flags->alt_rank = 0;
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) {
            client_flags->flags &= !client_flags::ReuseLastHandle;
        }
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            client_flags->flags &= !client_flags::FreeReuseLastHandle;
        }
        if (STR_IEQUALS(property, "fileCache")) {
            client_flags->flags &= !client_flags::FileCache;
        }
    }
}

//! Reset all data server properties to their default values
/**
 * @return Void.
 */
void udaResetProperties()
{
    auto client_flags = udaClientFlags();

    // Reset on Both Client and Server

    client_flags->get_datadble = 0;
    client_flags->get_dimdble = 0;
    client_flags->get_timedble = 0;
    client_flags->get_bad = 0;
    client_flags->get_meta = 0;
    client_flags->get_asis = 0;
    client_flags->get_uncal = 0;
    client_flags->get_notoff = 0;
    client_flags->get_synthetic = 0;
    client_flags->get_scalar = 0;
    client_flags->get_bytes = 0;
    client_flags->get_nodimdata = 0;
    set_log_level(UDA_LOG_NONE);
    client_flags->user_timeout = TimeOut;
    if (getenv("UDA_TIMEOUT")) {
        client_flags->user_timeout = atoi(getenv("UDA_TIMEOUT"));
    }
    client_flags->flags = 0;
    client_flags->alt_rank = 0;
}

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
 * @return ClientBlock pointer to the data structure.
 */
ClientBlock* udaGetProperties(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return &data_block->client_block;
}

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
 * @return ClientBlock pointer to the data structure.
 */
ClientBlock* udaGetDataProperties(int handle)
{
    return udaGetProperties(handle);
}

//--------------------------------------------------------------
//! Test for amount of Free heap memory and current usage
/** When the UDA client is a server plugin, set the Client's Debug File handle to that of the Server.
 * @return void
 */
#if !defined(__APPLE__) && !defined(_WIN32)

int udaGetMemoryFree()
{
#  ifdef A64
    return 0;
#  else
    struct mallinfo stats = mallinfo();
    return (int)stats.fordblks;
#  endif
}

int udaGetMemoryUsed()
{
#  ifdef A64
    return 0;
#  else
    struct mallinfo stats = mallinfo();
    return (int)stats.uordblks;
#  endif
}

#endif

//--------------------------------------------------------------
// Standard C PUT Accessor Routines

void udaPutErrorModel(int handle, int model, int param_n, const float* params)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    if (model <= (int)ErrorModelType::Unknown || model >= (int)ErrorModelType::Undefined) {
        return; // No valid Model
    }

    data_block->error_model = model;     // Model ID
    data_block->error_param_n = param_n; // Number of parameters

    if (param_n > MaxErrParams) {
        data_block->error_param_n = MaxErrParams;
    }

    for (int i = 0; i < data_block->error_param_n; i++) {
        data_block->errparams[i] = params[i];
    }
}

void udaPutDimErrorModel(int handle, int ndim, int model, int param_n, const float* params)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    if (ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return; // No Dim
    }
    if (model <= (int)ErrorModelType::Unknown || model >= (int)ErrorModelType::Undefined) {
        return; // No valid Model
    }

    data_block->dims[ndim].error_model = model;     // Model ID
    data_block->dims[ndim].error_param_n = param_n; // Number of parameters

    if (param_n > MaxErrParams) {
        data_block->dims[ndim].error_param_n = MaxErrParams;
    }
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
void udaPutServer(const char* host, int port)
{
    auto config = uda::client::client_config();

    auto old_port = config->get("connection.port").as_or_default(56565);
    auto old_host = config->get("connection.host").as_or_default("localhost"s);

    config->set("connection.port", port);
    config->set("connection.host", host);

    if (old_host != host || old_port != port) {
        config->set("connection.reconnect", true);
    }
}

//! Set the UDA data server host name
/** This takes precedence over the environment variables UDA_HOST.
 * @param host The name of the server host computer.
 * @return void
 */
void udaPutServerHost(const char* host)
{
    auto config = uda::client::client_config();

    auto old_host = config->get("connection.host").as_or_default("localhost"s);

    config->set("connection.host", host);

    if (old_host != host) {
        config->set("connection.reconnect", true);
    }
}

//! Set the UDA data server port number
/** This takes precedence over the environment variables UDA_PORT.
 * @param port The port number the server is connected to.
 * @return void
 */
void udaPutServerPort(int port)
{
    auto config = uda::client::client_config();

    auto old_port = config->get("connection.port").as_or_default(56565);

    config->set("connection.port", port);

    if (old_port != port) {
        config->set("connection.reconnect", true);
    }
}

//! the UDA server connection host name
/**
 * @return the Name of the Host
 */
const char* udaGetServerHost()
{
    auto config = uda::client::client_config();
    return config->get("connection.host").as_or_default<const char*>("localhost");
}

//! the UDA server connection port number
/**
 * @return the Name of the Host
 */
int udaGetServerPort()
{
    auto config = uda::client::client_config();
    return config->get("connection.port").as_or_default(56565);
}

const char* udaGetBuildDate()
{
    return UDA_BUILD_DATE;
}

//!  returns the data access error code
/**
\param   handle   The data object handle.
\return   Return error code, if non-zero there was a problem: < 0 is client side, > 0 is server side.
*/
int udaGetErrorCode(int handle)
{
    // Error Code Returned from Server
    auto data_block = getDataBlock(handle);
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
    // Error Message returned from server
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return udaGetServerErrorStackRecordMsg(0);
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
{ // Source Status
    auto data_block = getDataBlock(handle);
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
    // Signal Status
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->signal_status;
}

int udaGetDataStatus(int handle)
{
    // Data Status based on Standard Rule
    auto data_block = getDataBlock(handle);
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

//!  returns the number of data items in the data object
/** the number of array elements
\param   handle   The data object handle
\return  the number of data items
*/
int udaGetDataNum(int handle)
{
    // Data Array Size
    auto data_block = getDataBlock(handle);
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
    // Array Rank
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->rank;
}

//!  Returns the position of the time coordinate dimension in the data object
/** For example, a rank 3 array data[time][x][y] (in Fortran and IDL this is data(y,x,time)) has time order = 0 so order
is counted from left to right in c and from right to left in Fortran and IDL. \param   handle   The data object handle
\return  the time coordinate dimension position
*/
int udaGetOrder(int handle)
{
    // Time Dimension Order in Array
    auto data_block = getDataBlock(handle);
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
    // Permission to cache?
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return (int)PluginCachePermission::NotOkToCache;
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->data_type;
}

int udaGetDataOpaqueType(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return UDA_TYPE_UNKNOWN;
    }
    return data_block->opaque_type;
}

void* udaGetDataOpaqueBlock(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->opaque_block;
}

int udaGetDataOpaqueCount(int handle)
{
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    if (STR_IEQUALS(type, "dcomplex")) {
        return UDA_TYPE_DCOMPLEX;
    }
    if (STR_IEQUALS(type, "complex")) {
        return UDA_TYPE_COMPLEX;
    }
    if (STR_IEQUALS(type, "double")) {
        return UDA_TYPE_DOUBLE;
    }
    if (STR_IEQUALS(type, "float")) {
        return UDA_TYPE_FLOAT;
    }
    if (STR_IEQUALS(type, "long64")) {
        return UDA_TYPE_LONG64;
    }
    if (STR_IEQUALS(type, "long long")) {
        return UDA_TYPE_LONG64;
    }
    if (STR_IEQUALS(type, "ulong64")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (STR_IEQUALS(type, "unsigned long64")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (STR_IEQUALS(type, "unsigned long long")) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (STR_IEQUALS(type, "long")) {
        return UDA_TYPE_LONG;
    }
    if (STR_IEQUALS(type, "unsigned long")) {
        return UDA_TYPE_UNSIGNED_LONG;
    }
    if (STR_IEQUALS(type, "int")) {
        return UDA_TYPE_INT;
    }
    if (STR_IEQUALS(type, "integer")) {
        return UDA_TYPE_INT;
    }
    if (STR_IEQUALS(type, "unsigned")) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (STR_IEQUALS(type, "unsigned int")) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (STR_IEQUALS(type, "unsigned integer")) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (STR_IEQUALS(type, "short")) {
        return UDA_TYPE_SHORT;
    }
    if (STR_IEQUALS(type, "unsigned short")) {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (STR_IEQUALS(type, "char")) {
        return UDA_TYPE_CHAR;
    }
    if (STR_IEQUALS(type, "unsigned char")) {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (STR_IEQUALS(type, "unknown")) {
        return UDA_TYPE_UNKNOWN;
    }
    if (STR_IEQUALS(type, "undefined")) {
        return UDA_TYPE_UNDEFINED;
    }

    if (STR_IEQUALS(type, "vlen")) {
        return UDA_TYPE_VLEN;
    }
    if (STR_IEQUALS(type, "compound")) {
        return UDA_TYPE_COMPOUND;
    }
    if (STR_IEQUALS(type, "opaque")) {
        return UDA_TYPE_OPAQUE;
    }
    if (STR_IEQUALS(type, "enum")) {
        return UDA_TYPE_ENUM;
    }
    if (STR_IEQUALS(type, "string")) {
        return UDA_TYPE_STRING;
    }
    if (STR_IEQUALS(type, "void")) {
        return UDA_TYPE_VOID;
    }

    if (STR_IEQUALS(type, "string *")) {
        return UDA_TYPE_STRING;
    }

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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        *model = (int)ErrorModelType::Unknown;
        *param_n = 0;
        return;
    }
    *model = data_block->error_model;     // Model ID
    *param_n = data_block->error_param_n; // Number of parameters
    for (int i = 0; i < data_block->error_param_n; i++) {
        params[i] = data_block->errparams[i];
    }
}

int udaGetErrorAsymmetry(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    return (int)data_block->errasymmetry;
}

// Return the Internal Code for a named Error Model

int udaGetErrorModelId(const char* model)
{
    for (int i = 1; i < (int)ErrorModelType::Undefined; i++) {
        switch (i) {
            case 1:
                if (STR_IEQUALS(model, "default")) {
                    return (int)ErrorModelType::Default;
                }
                break;
            case 2:
                if (STR_IEQUALS(model, "default_asymmetric")) {
                    return (int)ErrorModelType::DefaultAsymmetric;
                }
                break;
#ifdef NO_GSL_LIB
            case 3:
                if (STR_IEQUALS(model, "gaussian")) {
                    return ErrorModelType::Gaussian;
                }
                break;
            case 4:
                if (STR_IEQUALS(model, "reseed")) {
                    return ErrorModelType::Reseed;
                }
                break;
            case 5:
                if (STR_IEQUALS(model, "gaussian_shift")) {
                    return ErrorModelType::GaussianShift;
                }
                break;
            case 6:
                if (STR_IEQUALS(model, "poisson")) {
                    return ErrorModelType::Poisson;
                }
                break;
#endif
            default:
                return (int)ErrorModelType::Unknown;
        }
    }
    return 0;
}

char* udaGetSyntheticDimData(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].synthetic;
}

void udaSetSyntheticData(int handle, char* data)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->synthetic = data;
}

void udaSetSyntheticDimData(int handle, int ndim, char* data)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].synthetic = data;
}

char* udaGetSyntheticData(int handle)
{
    auto client_flags = udaClientFlags();

    auto data_block = getDataBlock(handle);
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
    if (!client_flags->get_synthetic || data_block->error_model == (int)ErrorModelType::Unknown) {
        return data_block->data;
    }
    generate_synthetic_data(handle);
    return data_block->synthetic;
}

//!  Returns a pointer to the requested data
/** The data may be synthetically generated.
\param   handle   The data object handle
\return  a pointer to the data - if the status is poor, a nullptr pointer is returned unless the \e get_bad property is
set.
*/
char* udaGetData(int handle)
{
    auto client_flags = udaClientFlags();

    auto data_block = getDataBlock(handle);
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

char* udaGetDataErrLo(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errlo;
}

char* udaGetDataErrHi(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->errhi;
}

int udaGetDataErrAsymmetry(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->errasymmetry;
}

void udaSetDataErrAsymmetry(int handle, int asymmetry)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->errasymmetry = asymmetry;
};

void udaSetDataErrType(int handle, int type)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->error_type = type;
};

void udaSetDataErrLo(int handle, char* errlo)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->errlo = errlo;
};

char* udaGetDimErrLo(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].errlo;
}

char* udaGetDimErrHi(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return data_block->dims[ndim].errhi;
}

int udaGetDimErrAsymmetry(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    return data_block->dims[ndim].errasymmetry;
}

void udaSetDimErrAsymmetry(int handle, int ndim, int asymmetry)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].errasymmetry = asymmetry;
};

void udaSetDimErrType(int handle, int ndim, int type)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].error_type = type;
};

void udaSetDimErrLo(int handle, int ndim, char* errlo)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }
    data_block->dims[ndim].errlo = errlo;
};

char* udaGetAsymmetricError(int handle, int above)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    if (data_block->error_type != UDA_TYPE_UNKNOWN) {
        if (above) {
            return data_block->errhi; // return the default error array
        } else {
            if (!data_block->errasymmetry) {
                return data_block->errhi; // return the default error array if symmetric errors
            } else {
                return data_block->errlo;
            } // otherwise the data array must have been returned by the server or generated
        }
    } else {
        if (data_block->error_model != (int)ErrorModelType::Unknown) {
            generate_data_error(handle); // Create the errors from a model if the model exits
            if (above) {
                return data_block->errhi;
            } else if (!data_block->errasymmetry) {
                return data_block->errhi;
            } else {
                return data_block->errlo;
            }
        } else {

            char* errhi = nullptr; // Regular Error Component
            char* errlo = nullptr; // Asymmetric Error Component
            int ndata;

            ndata = data_block->data_n;
            data_block->error_type = data_block->data_type; // Error Type is Unknown so Assume Data's Data Type

            if (alloc_array(data_block->error_type, ndata, &errhi) != 0) {
                // Allocate Heap for Regular Error Data
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Data Errors");
                data_block->errhi = nullptr;
            } else {
                data_block->errhi = errhi;
            }

            if (data_block->errasymmetry) { // Allocate Heap for the Asymmetric Error Data
                if (alloc_array(data_block->error_type, ndata, &errlo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Asymmetric Errors");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!");
                    data_block->errlo = nullptr;
                    data_block->errasymmetry = 0;
                } else {
                    data_block->errlo = errlo;
                }
            }

            // Generate and return Zeros if this data is requested unless Error is Modelled

            switch (data_block->data_type) {
                case UDA_TYPE_FLOAT: {
                    float *fh, *fl = nullptr;
                    fh = (float*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        fl = (float*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(fh + i) = (float)0.0;
                        if (data_block->errasymmetry) {
                            *(fl + i) = (float)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    double *dh, *dl = nullptr;
                    dh = (double*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        dl = (double*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(dh + i) = (double)0.0;
                        if (data_block->errasymmetry) {
                            *(dl + i) = (double)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_SHORT: {
                    short *sh, *sl = nullptr;
                    sh = (short*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        sl = (short*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(sh + i) = (short)0;
                        if (data_block->errasymmetry) {
                            *(sl + i) = (short)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_SHORT: {
                    unsigned short *sh, *sl = nullptr;
                    sh = (unsigned short*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        sl = (unsigned short*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (data_block->errasymmetry) {
                            sl[i] = (unsigned short)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_INT: {
                    int *ih, *il = nullptr;
                    ih = (int*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        il = (int*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(ih + i) = (int)0;
                        if (data_block->errasymmetry) {
                            *(il + i) = (int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_INT: {
                    unsigned int *uh, *ul = nullptr;
                    uh = (unsigned int*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        ul = (unsigned int*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(uh + i) = (unsigned int)0;
                        if (data_block->errasymmetry) {
                            *(ul + i) = (unsigned int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_LONG: {
                    long *lh, *ll = nullptr;
                    lh = (long*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        ll = (long*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(lh + i) = (long)0;
                        if (data_block->errasymmetry) {
                            *(ll + i) = (long)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG: {
                    unsigned long *lh, *ll = nullptr;
                    lh = (unsigned long*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        ll = (unsigned long*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (data_block->errasymmetry) {
                            ll[i] = (unsigned long)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_LONG64: {
                    long long int *lh, *ll = nullptr;
                    lh = (long long int*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        ll = (long long int*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(lh + i) = (long long int)0;
                        if (data_block->errasymmetry) {
                            *(ll + i) = (long long int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG64: {
                    unsigned long long int *lh, *ll = nullptr;
                    lh = (unsigned long long int*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        ll = (unsigned long long int*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int)0;
                        if (data_block->errasymmetry) {
                            ll[i] = (unsigned long long int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_CHAR: {
                    char *ch, *cl = nullptr;
                    ch = data_block->errhi;
                    if (data_block->errasymmetry) {
                        cl = data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (char)0;
                        if (data_block->errasymmetry) {
                            cl[i] = (char)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_CHAR: {
                    unsigned char *ch, *cl = nullptr;
                    ch = (unsigned char*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        cl = (unsigned char*)data_block->errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (data_block->errasymmetry) {
                            cl[i] = (unsigned char)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_DCOMPLEX: {
                    DComplex *ch, *cl = nullptr;
                    ch = (DComplex*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        cl = (DComplex*)data_block->errlo;
                    }
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
                    Complex *ch, *cl = nullptr;
                    ch = (Complex*)data_block->errhi;
                    if (data_block->errasymmetry) {
                        cl = (Complex*)data_block->errlo;
                    }
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
    int above = 1;
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return nullptr;
    }
    return udaGetAsymmetricError(handle, above);
}

//!  Returns data cast to double precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the
status of the data is poor, no copy to the buffer occurs unless the property \b get_bad is set. \param   handle   The
data object handle \param   fp A \b double pointer to a preallocated data buffer \return  void
*/
void udaGetDoubleData(int handle, double* fp)
{
    // Copy Data cast as double to User Provided Array

    // **** The double array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    auto data_block = getDataBlock(handle);
    auto client_flags = udaClientFlags();

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return;
    }

    if (data_block->data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic) {
            memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(double));
        } else {
            generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                memcpy((void*)fp, (void*)data_block->synthetic, (size_t)data_block->data_n * sizeof(double));
            } else {
                memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(double));
            }
            return;
        }
    } else {

        char* array;
        int ndata;

        ndata = udaGetDataNum(handle);

        if (!client_flags->get_synthetic) {
            array = data_block->data;
        } else {
            generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                array = data_block->synthetic;
            } else {
                array = data_block->data;
            }
        }

        switch (data_block->data_type) {
            case UDA_TYPE_FLOAT: {
                auto dp = (float*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)dp[i];
                }
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)sp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)sp[i];
                }
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)ip[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)up[i];
                }
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)0.0; // No Data !
                }
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto dp = (DComplex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto dp = (Complex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            default:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)0.0;
                }
                break;
        }
        return;
    }
}

//!  Returns data cast to single precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the
status of the data is poor, no copy to the buffer occurs unless the property \b get_bad is set. \param   handle   The
data object handle \param   fp A \b float pointer to a preallocated data buffer \return  void
*/
void udaGetFloatData(int handle, float* fp)
{
    // Copy Data cast as float to User Provided Array

    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    auto data_block = getDataBlock(handle);
    auto client_flags = udaClientFlags();

    int status = udaGetDataStatus(handle);
    if (data_block == nullptr) {
        return;
    }
    if (status == MIN_STATUS && !data_block->client_block.get_bad && !client_flags->get_bad) {
        return;
    }
    if (status != MIN_STATUS && (data_block->client_block.get_bad || client_flags->get_bad)) {
        return;
    }

    if (data_block->data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic) {
            memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(float));
        } else {
            generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                memcpy((void*)fp, (void*)data_block->synthetic, (size_t)data_block->data_n * sizeof(float));
            } else {
                memcpy((void*)fp, (void*)data_block->data, (size_t)data_block->data_n * sizeof(float));
            }
            return;
        }
    } else {

        char* array;
        int ndata;

        ndata = udaGetDataNum(handle);

        if (!client_flags->get_synthetic) {
            array = data_block->data;
        } else {
            generate_synthetic_data(handle);
            if (data_block->synthetic != nullptr) {
                array = data_block->synthetic;
            } else {
                array = data_block->data;
            }
        }

        switch (data_block->data_type) {
            case UDA_TYPE_DOUBLE: {
                double* dp = (double*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)dp[i];
                }
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)sp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)sp[i];
                }
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)ip[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)up[i];
                }
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNKNOWN: {
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)0.0; // No Data !
                }
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto dp = (DComplex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto dp = (Complex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            default:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)0.0;
                }
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
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(DComplex));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, (void*)udaGetData(handle), (size_t)udaGetDataNum(handle) * sizeof(Complex));
            break;
    }
}

void udaGetFloatAsymmetricError(int handle, int above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return;
    }

    int ndata = data_block->data_n;

    if (data_block->error_type == UDA_TYPE_UNKNOWN) {
        udaGetAsymmetricError(handle, above);
    } // Create the Error Data prior to Casting

    switch (data_block->error_type) {
        case UDA_TYPE_UNKNOWN:
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)0.0; // No Error Data
            }
            break;
        case UDA_TYPE_FLOAT:
            if (above) {
                memcpy((void*)fp, (void*)data_block->errhi, (size_t)data_block->data_n * sizeof(float));
            } else if (!data_block->errasymmetry) {
                memcpy((void*)fp, (void*)data_block->errhi, (size_t)data_block->data_n * sizeof(float));
            } else {
                memcpy((void*)fp, (void*)data_block->errlo, (size_t)data_block->data_n * sizeof(float));
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)dp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)sp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)sp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)ip[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)up[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long int* lp;
            if (above) {
                lp = (unsigned long long int*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                lp = (unsigned long long int*)data_block->errhi;
            } else {
                lp = (unsigned long long int*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)cp[i];
            }
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
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DComplex* cp;
            if (above) {
                cp = (DComplex*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = (DComplex*)data_block->errhi;
            } else {
                cp = (DComplex*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            Complex* cp;
            if (above) {
                cp = (Complex*)data_block->errhi;
            } else if (!data_block->errasymmetry) {
                cp = (Complex*)data_block->errhi;
            } else {
                cp = (Complex*)data_block->errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        default:
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)0.0;
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
    int above = 1;
    udaGetFloatAsymmetricError(handle, above, fp);
}

//!  Returns the data label of a data object
/**
\param   handle   The data object handle
\return  pointer to the data label
*/
const char* udaGetDataLabel(int handle)
{
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
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
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return 0;
    }
    return data_block->dims[ndim].errasymmetry;
}

void udaGetDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {

        *model = (int)ErrorModelType::Unknown;
        *param_n = 0;
        return;
    }
    *model = data_block->dims[ndim].error_model;     // Model ID
    *param_n = data_block->dims[ndim].error_param_n; // Number of parameters
    for (int i = 0; i < data_block->dims[ndim].error_param_n; i++) {
        params[i] = data_block->dims[ndim].errparams[i];
    }
    // *params  = data_block->dims[ndim].errparams;        // Array of Model Parameters
}

///!  Returns a pointer to the requested coordinate data
/** The data may be synthetically generated.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data
*/
char* udaGetDimData(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    auto client_flags = udaClientFlags();
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    auto data_block = getDataBlock(handle);
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
    // **** The double array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }
    auto client_flags = udaClientFlags();
    if (data_block->dims[ndim].data_type == UDA_TYPE_DOUBLE) {
        if (!client_flags->get_synthetic) {
            memcpy((void*)fp, (void*)data_block->dims[ndim].dim, (size_t)data_block->dims[ndim].dim_n * sizeof(double));
        } else {
            generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                memcpy((void*)fp, (void*)data_block->dims[ndim].synthetic,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(double));
            } else {
                memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(double));
            }
            return;
        }
    } else {
        char* array;

        int ndata = data_block->dims[ndim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[ndim].dim;
        } else {
            generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                array = data_block->dims[ndim].synthetic;
            } else {
                array = data_block->dims[ndim].dim;
            }
        }

        switch (data_block->dims[ndim].data_type) {
            case UDA_TYPE_FLOAT: {
                auto dp = (float*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)dp[i];
                }
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)sp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)sp[i];
                }
                break;
            }
            case UDA_TYPE_INT: {
                int* ip = (int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)ip[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)up[i];
                }
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)lp[i];
                }
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)cp[i];
                }
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto cp = (DComplex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto cp = (Complex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)0.0;
                }
                break;
            default:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (double)0.0;
                }
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
    // **** The float array must be TWICE the size if the type is Complex otherwise a seg fault will occur!

    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }
    auto client_flags = udaClientFlags();
    if (data_block->dims[ndim].data_type == UDA_TYPE_FLOAT) {
        if (!client_flags->get_synthetic) {
            memcpy((void*)fp, (void*)data_block->dims[ndim].dim, (size_t)data_block->dims[ndim].dim_n * sizeof(float));
        } else {
            generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                memcpy((void*)fp, (void*)data_block->dims[ndim].synthetic,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            } else {
                memcpy((void*)fp, (void*)data_block->dims[ndim].dim,
                       (size_t)data_block->dims[ndim].dim_n * sizeof(float));
            }
            return;
        }
    } else {
        char* array;

        int ndata = data_block->dims[ndim].dim_n;
        if (!client_flags->get_synthetic) {
            array = data_block->dims[ndim].dim;
        } else {
            generate_synthetic_dim_data(handle, ndim);
            if (data_block->dims[ndim].synthetic != nullptr) {
                array = data_block->dims[ndim].synthetic;
            } else {
                array = data_block->dims[ndim].dim;
            }
        }

        switch (data_block->dims[ndim].data_type) {
            case UDA_TYPE_DOUBLE: {
                auto dp = (double*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)dp[i];
                }
                break;
            }
            case UDA_TYPE_SHORT: {
                auto sp = (short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)sp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_SHORT: {
                auto sp = (unsigned short*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)sp[i];
                }
                break;
            }
            case UDA_TYPE_INT: {
                auto ip = (int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)ip[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_INT: {
                auto up = (unsigned int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)up[i];
                }
                break;
            }
            case UDA_TYPE_LONG: {
                auto lp = (long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG: {
                auto lp = (unsigned long*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_LONG64: {
                auto lp = (long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_LONG64: {
                auto lp = (unsigned long long int*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)lp[i];
                }
                break;
            }
            case UDA_TYPE_CHAR: {
                auto cp = (char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)cp[i];
                }
                break;
            }
            case UDA_TYPE_UNSIGNED_CHAR: {
                auto cp = (unsigned char*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)cp[i];
                }
                break;
            }
            case UDA_TYPE_DCOMPLEX: {
                int j = 0;
                auto cp = (DComplex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_COMPLEX: {
                int j = 0;
                auto cp = (Complex*)array;
                for (int i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case UDA_TYPE_UNKNOWN:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)0.0;
                }
                break;
            default:
                for (int i = 0; i < ndata; i++) {
                    fp[i] = (float)0.0;
                }
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
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(unsigned int));
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
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(DComplex));
            break;
        case UDA_TYPE_COMPLEX:
            memcpy(data, (void*)udaGetDimData(handle, ndim), (size_t)udaGetDimNum(handle, ndim) * sizeof(Complex));
            break;
    }
}

//!  Returns the coordinate dimension's Dims data structure - the coordinate data and associated meta data.
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  Dims pointer
*/
Dims* udaGetDimBlock(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    return data_block->dims + ndim;
}

char* udaGetDimAsymmetricError(int handle, int ndim, int above)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    if (data_block->dims[ndim].error_type != UDA_TYPE_UNKNOWN) {
        if (above) {
            return data_block->dims[ndim].errhi; // return the default error array
        } else {
            if (!data_block->dims[ndim].errasymmetry) {
                return data_block->dims[ndim].errhi; // return the default error array if symmetric errors
            } else {
                return data_block->dims[ndim].errlo;
            } // otherwise the data array must have been returned by the server
        }     // or generated in a previous call
    } else {
        if (data_block->dims[ndim].error_model != (int)ErrorModelType::Unknown) {
            generate_dim_data_error(handle, ndim);
            if (above || !data_block->dims[ndim].errasymmetry) {
                return data_block->dims[ndim].errhi;
            } else {
                return data_block->dims[ndim].errlo;
            }
        } else {
            char* errhi = nullptr;
            char* errlo = nullptr;

            int ndata = data_block->dims[ndim].dim_n;
            data_block->dims[ndim].error_type =
                data_block->dims[ndim].data_type; // Error Type is Unknown so Assume Data's Data Type

            if (alloc_array(data_block->dims[ndim].error_type, ndata, &errhi) != 0) {
                UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Data Errors");
                data_block->dims[ndim].errhi = nullptr;
            } else {
                data_block->dims[ndim].errhi = errhi;
            }

            if (data_block->dims[ndim].errasymmetry) { // Allocate Heap for the Asymmetric Error Data
                if (alloc_array(data_block->dims[ndim].error_type, ndata, &errlo) != 0) {
                    UDA_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Asymmetric Errors");
                    UDA_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!");
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
                    if (data_block->dims[ndim].errasymmetry) {
                        fl = (float*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        fh[i] = (float)0.0;
                        if (data_block->dims[ndim].errasymmetry) {
                            fl[i] = (float)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_DOUBLE: {
                    double* dl = nullptr;
                    auto dh = (double*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        dl = (double*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        dh[i] = (double)0.0;
                        if (data_block->dims[ndim].errasymmetry) {
                            dl[i] = (double)0.0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_SHORT: {
                    short* sl = nullptr;
                    auto sh = (short*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        sl = (short*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (short)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            sl[i] = (short)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_SHORT: {
                    unsigned short* sl = nullptr;
                    auto sh = (unsigned short*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        sl = (unsigned short*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            sl[i] = (unsigned short)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_INT: {
                    int* il = nullptr;
                    auto ih = (int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        il = (int*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        ih[i] = (int)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            il[i] = (int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_INT: {
                    unsigned int* ul = nullptr;
                    auto uh = (unsigned int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        ul = (unsigned int*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        uh[i] = (unsigned int)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            ul[i] = (unsigned int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_LONG: {
                    long* ll = nullptr;
                    auto lh = (long*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        ll = (long*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (long)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            ll[i] = (long)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG: {
                    unsigned long* ll = nullptr;
                    auto lh = (unsigned long*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        ll = (unsigned long*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            ll[i] = (unsigned long)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_LONG64: {
                    long long int* ll = nullptr;
                    auto lh = (long long int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        ll = (long long int*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (long long int)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            ll[i] = (long long int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_LONG64: {
                    unsigned long long int* ll = nullptr;
                    auto lh = (unsigned long long int*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        ll = (unsigned long long int*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            ll[i] = (unsigned long long int)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_CHAR: {
                    char* cl = nullptr;
                    auto ch = data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        cl = data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        *(ch + i) = ' ';
                        if (data_block->dims[ndim].errasymmetry) {
                            *(cl + i) = ' ';
                        }
                    }
                    break;
                }
                case UDA_TYPE_UNSIGNED_CHAR: {
                    unsigned char* cl = nullptr;
                    auto ch = (unsigned char*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        cl = (unsigned char*)data_block->dims[ndim].errlo;
                    }
                    for (int i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (data_block->dims[ndim].errasymmetry) {
                            cl[i] = (unsigned char)0;
                        }
                    }
                    break;
                }
                case UDA_TYPE_DCOMPLEX: {
                    DComplex* cl = nullptr;
                    auto ch = (DComplex*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        cl = (DComplex*)data_block->dims[ndim].errlo;
                    }
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
                    Complex* cl = nullptr;
                    auto ch = (Complex*)data_block->dims[ndim].errhi;
                    if (data_block->dims[ndim].errasymmetry) {
                        cl = (Complex*)data_block->dims[ndim].errlo;
                    }
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
            return data_block->dims[ndim].errhi; // Errors are Symmetric at this point
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
    int above = 1;
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return nullptr;
    }
    return udaGetDimAsymmetricError(handle, ndim, above);
}

void udaGetFloatDimAsymmetricError(int handle, int ndim, int above, float* fp)
{
    // Copy Error Data cast as float to User Provided Array

    auto data_block = getDataBlock(handle);
    if (data_block == nullptr || ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return;
    }

    int ndata = data_block->dims[ndim].dim_n;

    if (data_block->dims[ndim].error_type == UDA_TYPE_UNKNOWN) {
        udaGetDimAsymmetricError(handle, ndim, above);
    } // Create the Error Data prior to Casting

    switch (data_block->dims[ndim].error_type) {
        case UDA_TYPE_UNKNOWN:
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)0.0; // No Error Data
            }
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
            double* dp; // Return Zeros if this data is requested unless Error is Modelled
            if (above || !data_block->dims[ndim].errasymmetry) {
                dp = (double*)data_block->dims[ndim].errhi;
            } else {
                dp = (double*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)dp[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            short* sp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                sp = (short*)data_block->dims[ndim].errhi;
            } else {
                sp = (short*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                sp = (unsigned short*)data_block->dims[ndim].errhi;
            } else {
                sp = (unsigned short*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)sp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            int* ip;
            if (above || !data_block->dims[ndim].errasymmetry) {
                ip = (int*)data_block->dims[ndim].errhi;
            } else {
                ip = (int*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)ip[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            unsigned int* up;
            if (above || !data_block->dims[ndim].errasymmetry) {
                up = (unsigned int*)data_block->dims[ndim].errhi;
            } else {
                up = (unsigned int*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)up[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            long* lp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                lp = (long*)data_block->dims[ndim].errhi;
            } else {
                lp = (long*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                lp = (unsigned long*)data_block->dims[ndim].errhi;
            } else {
                lp = (unsigned long*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)lp[i];
            }
            break;
        }
        case UDA_TYPE_CHAR: {
            char* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = data_block->dims[ndim].errhi;
            } else {
                cp = data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (unsigned char*)data_block->dims[ndim].errhi;
            } else {
                cp = (unsigned char*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[i] = (float)cp[i];
            }
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            int j = 0;
            DComplex* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (DComplex*)data_block->dims[ndim].errhi;
            } else {
                cp = (DComplex*)data_block->dims[ndim].errlo;
            }
            for (int i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            int j = 0;
            Complex* cp;
            if (above || !data_block->dims[ndim].errasymmetry) {
                cp = (Complex*)data_block->dims[ndim].errhi;
            } else {
                cp = (Complex*)data_block->dims[ndim].errlo;
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
    int above = 1;
    udaGetFloatDimAsymmetricError(handle, ndim, above, fp);
}

//-----------------------------------------------------------------------------------------------------------
// Various Utilities

int idamDataCheckSum(void* data, int data_n, int type)
{
    int sum = 0;
    switch (type) {
        case UDA_TYPE_FLOAT: {
            float fsum = 0.0;
            auto dp = (float*)data;
            for (int i = 0; i < data_n; i++) {
                if (std::isfinite(dp[i])) {
                    fsum = fsum + dp[i];
                }
            }
            sum = (int)fsum;
            if (sum == 0) {
                sum = (int)(1000000.0 * fsum); // Rescale
            }
            break;
        }
        case UDA_TYPE_DOUBLE: {
            double fsum = 0.0;
            auto dp = (double*)data;
            for (int i = 0; i < data_n; i++) {
                if (std::isfinite(dp[i])) {
                    fsum = fsum + dp[i];
                }
            }
            sum = (int)fsum;
            if (sum == 0) {
                sum = (int)(1000000.0 * fsum); // Rescale
            }
            break;
        }
        case UDA_TYPE_COMPLEX: {
            float fsum = 0.0;
            auto dp = (Complex*)data;
            for (int i = 0; i < data_n; i++) {
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            }
            sum = (int)fsum;
            if (sum == 0) {
                sum = (int)(1000000.0 * fsum); // Rescale
            }
            break;
        }
        case UDA_TYPE_DCOMPLEX: {
            double fsum = 0.0;
            auto dp = (DComplex*)data;
            for (int i = 0; i < data_n; i++) {
                if (std::isfinite(dp[i].real) && std::isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            }
            sum = (int)fsum;
            if (sum == 0) {
                sum = (int)(1000000.0 * fsum); // Rescale
            }
            break;
        }

        case UDA_TYPE_CHAR: {
            char* dp = (char*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_SHORT: {
            auto dp = (short int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_INT: {
            int* dp = (int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_LONG: {
            auto dp = (long*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_LONG64: {
            auto dp = (long long int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_CHAR: {
            auto dp = (unsigned char*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_SHORT: {
            auto dp = (unsigned short int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_INT: {
            auto dp = (unsigned int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG: {
            auto dp = (unsigned long*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        case UDA_TYPE_UNSIGNED_LONG64: {
            auto dp = (unsigned long long int*)data;
            for (int i = 0; i < data_n; i++) {
                sum = sum + (int)dp[i];
            }
            break;
        }
        default:
            sum = 0;
    }
    return sum;
}

int udaGetDataCheckSum(int handle)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) {
        return 0;
    }

    return (idamDataCheckSum((void*)data_block->data, data_block->data_n, data_block->data_type));
}

int udaGetDimDataCheckSum(int handle, int ndim)
{
    auto data_block = getDataBlock(handle);
    if (data_block == nullptr) {
        return 0;
    }
    if (data_block->errcode != 0) {
        return 0;
    }
    if (ndim < 0 || (unsigned int)ndim >= data_block->rank) {
        return 0;
    }

    return (idamDataCheckSum((void*)data_block->dims[ndim].dim, data_block->dims[ndim].dim_n,
                             data_block->dims[ndim].data_type));
}

//===========================================================================================================
// Access to (De)Serialiser

void udaGetClientSerialisedDataBlock(int handle, void** object, size_t* objectSize, char** key, size_t* keySize,
                                     int protocolVersion, LogStructList* log_struct_list, int private_flags,
                                     int malloc_source)
{
    // Extract the serialised Data Block from Cache or serialise it if not cached (hash key in Data Block, empty if not
    // cached) Use Case: extract data in object form for storage in external data object store, e.g. CEPH, HDF5
    /*
     * TODO
     *
     * 1> Add cache key to DataBlock
     * 2> Investigate creation of cache key when RequestBlock is out of scope!
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

    ProtocolId token;

    auto userdefinedtypelist = static_cast<UserDefinedTypeList*>(udaGetUserDefinedTypeList(handle));
    auto logmalloclist = static_cast<LogMallocList*>(udaGetLogMallocList(handle));
    DataBlockList data_block_list;
    data_block_list.count = 1;
    data_block_list.data = getDataBlock(handle);
    protocol2(&xdrs, ProtocolId::DataBlockList, XDRStreamDirection::Send, &token, logmalloclist, userdefinedtypelist,
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

    xdr_destroy(&xdrs); // Destroy before the  file otherwise a segmentation error occurs
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
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return 0; // Return FALSE
    }
    if (udaGetData(handle) == nullptr) {
        return 0;
    }

    udaSetFullNTree((NTREE*)udaGetData(handle));
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    setUserDefinedTypeList(((GeneralBlock*)opaque_block)->userdefinedtypelist);
    setLogMallocList(((GeneralBlock*)opaque_block)->logmalloclist);
    udaSetLastMallocIndexValue(&(((GeneralBlock*)opaque_block)->lastMallocIndex));
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

PUTDATA_BLOCK* udaNewPutDataBlock(UDA_TYPE data_type, int count, int rank, int* shape, const char* data)
{
    PutDataBlock* put_data = (PutDataBlock*)malloc(sizeof(PutDataBlock));
    put_data->data_type = data_type;
    put_data->count = count;
    put_data->rank = rank;
    put_data->shape = (int*)malloc(sizeof(int) * rank);
    for (int i = 0; i < rank; ++i) {
        put_data->shape[i] = shape[i];
    }
    put_data->data = data;
    return put_data;
}

void udaFreePutDataBlock(PUTDATA_BLOCK* putdata_block)
{
    PutDataBlock* put_data_block = (PutDataBlock*)putdata_block;
    free(put_data_block->shape);
    free(put_data_block);
}

// Return a user defined data structure definition

USERDEFINEDTYPE* udaGetUserDefinedType(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return nullptr;
    }
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GeneralBlock*)opaque_block)->userdefinedtype;
}

USERDEFINEDTYPELIST* udaGetUserDefinedTypeList(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return nullptr;
    }
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GeneralBlock*)opaque_block)->userdefinedtypelist;
}

LOGMALLOCLIST* udaGetLogMallocList(int handle)
{
    if (udaGetDataOpaqueType(handle) != UDA_OPAQUE_TYPE_STRUCTURES) {
        return nullptr;
    }
    void* opaque_block = udaGetDataOpaqueBlock(handle);
    return ((GeneralBlock*)opaque_block)->logmalloclist;
}

NTREE* udaFindIdamNTreeStructureDefinition(NTREE* node, const char* target)
{
    return udaFindNTreeStructureDefinition(node, target);
}
