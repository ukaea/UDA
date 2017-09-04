/*---------------------------------------------------------------
* Accessor Functions
*
*/

#include "accAPI.h"

#include <math.h>
#include <pthread.h>
#include <strings.h>

#include <logging/logging.h>
#include <clientserver/initStructs.h>
#include <clientserver/errorLog.h>
#include <clientserver/stringUtils.h>
#include <clientserver/allocData.h>
#include <clientserver/protocol.h>
#include <clientserver/memstream.h>
#include <clientserver/xdrlib.h>
#include <clientserver/socketStructs.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <cache/cache.h>

#include "generateErrors.h"
#include "getEnvironment.h"
#include "udaClient.h"

#ifdef __APPLE__
#  include <stdlib.h>
#elif !defined(A64)
#  include <malloc.h>
#endif

/*
#ifndef FATCLIENT
#  ifdef SECURITYENABLED
#    include "idamsecurity.h"
#  endif
#endif
*/

static int Data_Block_Count = 0;        // Count of Blocks recorded
static DATA_BLOCK* Data_Block = NULL;   // All Data are recorded here!

//---------------------------- Mutex locking for thread safety -------------------------
// State variable sets should be collected and managed for each individual thread!

static int idamThreadLastHandle = -1;

#ifndef NOPTHREADS

typedef struct {
    int id;                         // Thread identifier assigned by the application
    int socket;                     // Either a shared or private server socket connection
    int lastHandle;
    ENVIRONMENT environment;        // State
    CLIENT_BLOCK client_block;
    SERVER_BLOCK server_block;
} IDAMSTATE;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// STATE management

static IDAMSTATE idamState[NUMIDAMCLIENTTHREADS];    // Threads are managed by the application, not IDAM
static pthread_t threadList[NUMIDAMCLIENTTHREADS];
static int threadCount = 0;

int getIdamMaxThreadCount()
{
    return NUMIDAMCLIENTTHREADS;
}

int getThreadId(pthread_t id)
{            // Search the set of registered threads for the State ID
    int i;
    for (i = 0; i < threadCount; i++) if (pthread_equal(id, threadList[i])) return i;
    return -1;
}

// Lock the thread and set the previous STATE  
void lockIdamThread()
{
    pthread_mutex_lock(&lock);            // Apply the lock first

    static unsigned int mutex_initialised = 0;

// Identify the Current Thread   

    pthread_t threadId = pthread_self();

// Initialise the thread's state

    if (!mutex_initialised) {
        mutex_initialised = 1;
        int i;
        for (i = 0; i < NUMIDAMCLIENTTHREADS; i++) {        // Initialise the STATE array
            idamState[i].id = i;
            idamState[i].socket = -1;
            idamState[i].lastHandle = -1;
            //initEnvironment(&(idamState[i].environment));
            initClientBlock(&(idamState[i].client_block), 0, "");
            initServerBlock(&(idamState[i].server_block), 0);
            threadList[i] = 0;            // and the thread identifiers
        }
    }

// Retain unique thread IDs 

    int id = getThreadId(threadId);

    if (threadCount < NUMIDAMCLIENTTHREADS && id == -1) {
        threadList[++threadCount - 1] = threadId;
    }        // Preserve the thread ID if not registered

// Assign State for the current thread if previously registered 

    if (id >= 0) {
        putIdamServerSocket(idamState[id].socket);
        putIdamClientEnvironment(&idamState[id].environment);
        putIdamThreadClientBlock(&idamState[id].client_block);
        putIdamThreadServerBlock(&idamState[id].server_block);
        clientFlags = idamState[id].client_block.clientFlags;
        putIdamThreadLastHandle(idamState[id].lastHandle);
    } else {
        putIdamThreadLastHandle(-1);
    }
}

// Unlock the thread and save the current STATE
void unlockIdamThread()
{
    pthread_t threadId = pthread_self();
    int id = getThreadId(threadId);        // Must be registered
    if (id >= 0) {
        idamState[id].socket = getIdamServerSocket();
        idamState[id].environment = *getIdamClientEnvironment();
        idamState[id].client_block = getIdamThreadClientBlock();
        idamState[id].server_block = getIdamThreadServerBlock();
        idamState[id].client_block.clientFlags = clientFlags;
        idamState[id].lastHandle = getIdamThreadLastHandle();
    }
    pthread_mutex_unlock(&lock);
}

// Free thread resources
void freeIdamThread()
{
    lockIdamThread();
    pthread_t threadId = pthread_self();
    int i, id = getThreadId(threadId);
    threadCount--;
    if (id >= 0) {
        for (i = id; i < threadCount; i++) {
            threadList[i] = threadList[i + 1];        // Shuffle state
            idamState[i] = idamState[i + 1];
            idamState[i].id = i;
        }
        idamState[threadCount].id = threadCount;
        idamState[threadCount].socket = -1;
        idamState[threadCount].lastHandle = -1;
        //initEnvironment(&(idamState[threadCount].environment));
        initClientBlock(&(idamState[threadCount].client_block), 0, "");
        initServerBlock(&(idamState[threadCount].server_block), 0);
        threadList[threadCount] = 0;
    }
    unlockIdamThread();
}

#else
void lockIdamThread() {}
void unlockIdamThread() {}
void freeIdamThread() {}
#endif // NOPTHREADS

int getIdamThreadLastHandle()
{
    return idamThreadLastHandle;
}

void putIdamThreadLastHandle(int handle)
{
    idamThreadLastHandle = handle;
}

//--------------------------------------------------------------------------------------
// C Accessor Routines

void acc_freeDataBlocks()
{
    free((void*)Data_Block);
    Data_Block = NULL;
    Data_Block_Count = 0;
    putIdamThreadLastHandle(-1);
}

DATA_BLOCK* acc_getCurrentDataBlock()
{
    if ((clientFlags & CLIENTFLAG_REUSELASTHANDLE || clientFlags & CLIENTFLAG_FREEREUSELASTHANDLE) &&
        getIdamThreadLastHandle() >= 0) {
        return &Data_Block[getIdamThreadLastHandle()];
    }
    return &Data_Block[Data_Block_Count - 1];
}

int acc_getCurrentDataBlockIndex()
{
    if ((clientFlags & CLIENTFLAG_REUSELASTHANDLE || clientFlags & CLIENTFLAG_FREEREUSELASTHANDLE) &&
        getIdamThreadLastHandle() >= 0) {
        return getIdamThreadLastHandle();
    }
    return Data_Block_Count - 1;
}

int acc_growIdamDataBlocks()
{

    if ((clientFlags & CLIENTFLAG_REUSELASTHANDLE || clientFlags & CLIENTFLAG_FREEREUSELASTHANDLE) &&
        getIdamThreadLastHandle() >= 0) {
        return 0;
    }

    ++Data_Block_Count;
    Data_Block = (DATA_BLOCK*)realloc(Data_Block, Data_Block_Count * sizeof(DATA_BLOCK));

    if (!Data_Block) {
        int err = ERROR_ALLOCATING_DATA_BOCK_HEAP;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "idamClient", err, "Error Allocating Heap for Data Block");
        return err;
    }

    initDataBlock(&Data_Block[Data_Block_Count - 1]);
    Data_Block[Data_Block_Count - 1].handle = Data_Block_Count - 1;

    putIdamThreadLastHandle(Data_Block_Count - 1);

    return 0;
}

#define GROWHANDLELIST 10000        // 50MB allocation (Use an Environment Variable to modify)

static int findNewHandleIndex()
{
    int i;
    for (i = 0; i < Data_Block_Count; i++) {
        if (Data_Block[i].handle == -1) {
            return i;
        }
    }
    return -1;
}

int acc_getIdamNewDataHandle()
{
    int newHandleIndex = -1;
    static unsigned int handleListSize = 0;
    static unsigned int growHandleList = 0;

    if ((clientFlags & CLIENTFLAG_REUSELASTHANDLE || clientFlags & CLIENTFLAG_FREEREUSELASTHANDLE) &&
        (newHandleIndex = getIdamThreadLastHandle()) >= 0) {
        if (clientFlags & CLIENTFLAG_FREEREUSELASTHANDLE) {
            idamFree(newHandleIndex);
        } else {
            initDataBlock(
                    &Data_Block[newHandleIndex]);
        }        // Application has responsibility for freeing heap in the Data Block
        Data_Block[newHandleIndex].handle = newHandleIndex;
        return newHandleIndex;
    }

    if (growHandleList == 0) {        // Increase the list of Data Blocks by this ammount each time
        char* env = NULL;

        if ((env = getenv("UDA_GROWHANDLELIST")) != NULL) {
            growHandleList = atoi(env);
        } else {
            growHandleList = GROWHANDLELIST;
        }
    }

    if (Data_Block_Count == 0) handleListSize = 0;    // Reset after an idamFreeAll

    if ((newHandleIndex = findNewHandleIndex()) < 0) { // Search for an unused handle or issue a new one

        newHandleIndex = Data_Block_Count;

        if (newHandleIndex >= handleListSize) {   // Allocate new multiple DATA_BLOCKS
            handleListSize += growHandleList;
            Data_Block = (DATA_BLOCK*)realloc(Data_Block, handleListSize * sizeof(DATA_BLOCK));

            if (!Data_Block) {
                int err = ERROR_ALLOCATING_DATA_BOCK_HEAP;
                addIdamError(&idamerrorstack, CODEERRORTYPE, "acc_getIdamNewDataHandle", err,
                             "Error Allocating Heap for Data Block");
                return -err;
            }

            int i;

            for (i = Data_Block_Count; i < handleListSize; i++) {
                initDataBlock(&Data_Block[i]);
            }
        }

        Data_Block_Count++;
        Data_Block[newHandleIndex].handle = newHandleIndex;
    } else {
        initDataBlock(&Data_Block[newHandleIndex]);
        Data_Block[newHandleIndex].handle = newHandleIndex;
    }

    putIdamThreadLastHandle(newHandleIndex);
    return newHandleIndex;
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
// Private Flags (Server to Server communication via an IDAM client server plugin)

//! Set a privateFlags property
/** Set a/multiple specific bit/s in the privateFlags property sent between IDAM servers.
*
* @param flag The bit/s to be set to 1.
* @return Void.
*/
void setIdamPrivateFlag(unsigned int flag)
{
    privateFlags = privateFlags | flag;
}

//! Reset a privateFlags property
/** Reset a/multiple specific bit/s in the privateFlags property sent between IDAM servers.
*
* @param flag The bit/s to be set to 0.
* @return Void.
*/

void resetIdamPrivateFlag(unsigned int flag)
{
    privateFlags = privateFlags & !flag;
}

//--------------------------------------------------------------
// Client Flags

//! Set a clientFlags property
/** Set a/multiple specific bit/s in the clientFlags property sent to the IDAM server.
*
* @param flag The bit/s to be set to 1.
* @return Void.
*/

void setIdamClientFlag(unsigned int flag)
{
    clientFlags = clientFlags | flag;
}

//! Reset a clientFlags property
/** Reset a/multiple specific bit/s in the clientFlags property sent to the IDAM server.
*
* @param flag The bit/s to be set to 0.
* @return Void.
*/

void resetIdamClientFlag(unsigned int flag)
{
    clientFlags = clientFlags & !flag;
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
void setIdamProperty(const char* property)
{

// User settings for Client and Server behaviour

    char name[56];
    char* value;

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) get_datadble = 1;
        if (STR_IEQUALS(property, "get_dimdble")) get_dimdble = 1;
        if (STR_IEQUALS(property, "get_timedble")) get_timedble = 1;
        if (STR_IEQUALS(property, "get_bytes")) get_bytes = 1;
        if (STR_IEQUALS(property, "get_bad")) get_bad = 1;
        if (STR_IEQUALS(property, "get_meta")) get_meta = 1;
        if (STR_IEQUALS(property, "get_asis")) get_asis = 1;
        if (STR_IEQUALS(property, "get_uncal")) get_uncal = 1;
        if (STR_IEQUALS(property, "get_notoff")) get_notoff = 1;
        if (STR_IEQUALS(property, "get_synthetic")) get_synthetic = 1;
        if (STR_IEQUALS(property, "get_scalar")) get_scalar = 1;
        if (STR_IEQUALS(property, "get_nodimdata")) get_nodimdata = 1;
    } else {
        if (property[0] == 't') {
            strncpy(name, property, 55);
            name[55] = '\0';
            TrimString(name);
            LeftTrimString(name);
            MidTrimString(name);
            strlwr(name);
            if ((value = strstr(name, "timeout=")) != NULL) {
                value = name + 8;
                if (IsNumber(value)) user_timeout = atoi(value);
            }
        } else {
            if (STR_IEQUALS(property, "verbose")) idamSetLogLevel(UDA_LOG_INFO);
            if (STR_IEQUALS(property, "debug")) idamSetLogLevel(UDA_LOG_DEBUG);
            if (STR_IEQUALS(property, "altData")) clientFlags = clientFlags | CLIENTFLAG_ALTDATA;
            if (!strncasecmp(property, "altRank", 7)) {
                strncpy(name, property, 55);
                name[55] = '\0';
                TrimString(name);
                LeftTrimString(name);
                MidTrimString(name);
                strlwr(name);
                if ((value = strcasestr(name, "altRank=")) != NULL) {
                    value = name + 8;
                    if (IsNumber(value)) altRank = atoi(value);
                }
            }
        }
        if (STR_IEQUALS(property, "reuseLastHandle")) clientFlags = clientFlags | CLIENTFLAG_REUSELASTHANDLE;
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) clientFlags = clientFlags | CLIENTFLAG_FREEREUSELASTHANDLE;
    }
    return;
}

//! Return the value of a named server property
/**
* @param property the name of the property.
* @return Void.
*/
int getIdamProperty(const char* property)
{

// User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) return get_datadble;
        if (STR_IEQUALS(property, "get_dimdble")) return get_dimdble;
        if (STR_IEQUALS(property, "get_timedble")) return get_timedble;
        if (STR_IEQUALS(property, "get_bytes")) return get_bytes;
        if (STR_IEQUALS(property, "get_bad")) return get_bad;
        if (STR_IEQUALS(property, "get_meta")) return get_meta;
        if (STR_IEQUALS(property, "get_asis")) return get_asis;
        if (STR_IEQUALS(property, "get_uncal")) return get_uncal;
        if (STR_IEQUALS(property, "get_notoff")) return get_notoff;
        if (STR_IEQUALS(property, "get_synthetic")) return get_synthetic;
        if (STR_IEQUALS(property, "get_scalar")) return get_scalar;
        if (STR_IEQUALS(property, "get_nodimdata")) return get_nodimdata;
    } else {
        if (STR_IEQUALS(property, "timeout")) return user_timeout;
        if (STR_IEQUALS(property, "altRank")) return altRank;
        if (STR_IEQUALS(property, "reuseLastHandle")) return clientFlags | CLIENTFLAG_REUSELASTHANDLE;
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) return clientFlags | CLIENTFLAG_FREEREUSELASTHANDLE;
        if (STR_IEQUALS(property, "verbose")) return idamGetLogLevel() == UDA_LOG_INFO;
        if (STR_IEQUALS(property, "debug")) return idamGetLogLevel() == UDA_LOG_DEBUG;
        if (STR_IEQUALS(property, "altData")) return clientFlags | CLIENTFLAG_ALTDATA;

    }
    return 0;
}

//! Reset a specific named data server property to its default value
/**
* @param property the name of the property.
* @return Void.
*/

void resetIdamProperty(const char* property)
{
//
// User settings for Client and Server behaviour

    if (property[0] == 'g') {
        if (STR_IEQUALS(property, "get_datadble")) get_datadble = 0;
        if (STR_IEQUALS(property, "get_dimdble")) get_dimdble = 0;
        if (STR_IEQUALS(property, "get_timedble")) get_timedble = 0;
        if (STR_IEQUALS(property, "get_bytes")) get_bytes = 0;
        if (STR_IEQUALS(property, "get_bad")) get_bad = 0;
        if (STR_IEQUALS(property, "get_meta")) get_meta = 0;
        if (STR_IEQUALS(property, "get_asis")) get_asis = 0;
        if (STR_IEQUALS(property, "get_uncal")) get_uncal = 0;
        if (STR_IEQUALS(property, "get_notoff")) get_notoff = 0;
        if (STR_IEQUALS(property, "get_synthetic")) get_synthetic = 0;
        if (STR_IEQUALS(property, "get_scalar")) get_scalar = 0;
        if (STR_IEQUALS(property, "get_nodimdata")) get_nodimdata = 0;
    } else {
        if (STR_IEQUALS(property, "verbose")) idamSetLogLevel(UDA_LOG_NONE);
        if (STR_IEQUALS(property, "debug")) idamSetLogLevel(UDA_LOG_NONE);
        if (STR_IEQUALS(property, "altData")) clientFlags = clientFlags & !CLIENTFLAG_ALTDATA;
        if (STR_IEQUALS(property, "altRank")) altRank = 0;
        if (STR_IEQUALS(property, "reuseLastHandle")) clientFlags = clientFlags & !CLIENTFLAG_REUSELASTHANDLE;
        if (STR_IEQUALS(property, "freeAndReuseLastHandle")) {
            clientFlags = clientFlags & !CLIENTFLAG_FREEREUSELASTHANDLE;
        }
    }
    return;
}

//! Reset all data server properties to their default values
/**
* @return Void.
*/
void resetIdamProperties()
{

// Reset on Both Client and Server

    get_datadble = 0;
    get_dimdble = 0;
    get_timedble = 0;
    get_bad = 0;
    get_meta = 0;
    get_asis = 0;
    get_uncal = 0;
    get_notoff = 0;
    get_synthetic = 0;
    get_scalar = 0;
    get_bytes = 0;
    get_nodimdata = 0;
    idamSetLogLevel(UDA_LOG_NONE);
    user_timeout = TIMEOUT;
    clientFlags = clientFlags & !CLIENTFLAG_ALTDATA;
    altRank = 0;
    return;
}

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
* @return CLIENT_BLOCK pointer to the data structure.
*/
CLIENT_BLOCK* getIdamProperties(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return &Data_Block[handle].client_block;
}

//! Return the client state associated with a specific data item
/** The client state information is at the time the data was accessed.
* @return CLIENT_BLOCK pointer to the data structure.
*/
CLIENT_BLOCK* getIdamDataProperties(int handle)
{
    return getIdamProperties(handle);
}

//--------------------------------------------------------------
//! Test for amount of Free heap memory and current usage
/** When the IDAM client is a server plugin, set the Client's Debug File handle to that of the Server.
* @return void
*/
#ifndef __APPLE__

int getIdamMemoryFree()
{
#ifdef A64
    return 0;
#else
    struct mallinfo stats = mallinfo();
    return (int) stats.fordblks;
#endif
}

int getIdamMemoryUsed()
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

void putIdamErrorModel(int handle, int model, int param_n, float* params)
{
    int i;
    if (handle < 0 || handle >= Data_Block_Count) return;          // No Data Block available
    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return;   // No valid Model

    Data_Block[handle].error_model = model;               // Model ID
    Data_Block[handle].error_param_n = param_n;             // Number of parameters

    if (param_n > MAXERRPARAMS) Data_Block[handle].error_param_n = MAXERRPARAMS;
    for (i = 0; i < Data_Block[handle].error_param_n; i++) Data_Block[handle].errparams[i] = params[i];
}

void putIdamDimErrorModel(int handle, int ndim, int model, int param_n, float* params)
{
    int i;
    if (handle < 0 || handle >= Data_Block_Count) return;                        // No Data Block available
    if (ndim < 0 || ndim >= Data_Block[handle].rank) return;                     // No Dim
    if (model <= ERROR_MODEL_UNKNOWN || model >= ERROR_MODEL_UNDEFINED) return;  // No valid Model

    Data_Block[handle].dims[ndim].error_model = model;                        // Model ID
    Data_Block[handle].dims[ndim].error_param_n = param_n;                      // Number of parameters

    if (param_n > MAXERRPARAMS) Data_Block[handle].dims[ndim].error_param_n = MAXERRPARAMS;
    for (i = 0; i < Data_Block[handle].dims[ndim].error_param_n; i++)
        Data_Block[handle].dims[ndim].errparams[i] = params[i];
}

//! Set the IDAM data server host name and port number
/** This takes precedence over the environment variables IDAM_HOST and IDAM_PORT.
* @param host The name of the server host computer.
* @param port The port number the server is connected to.
* @return void
*/
void putIdamServer(const char* host, int port)
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    environment->server_port = port;                             // IDAM server service port number
    strcpy(environment->server_host, host);                      // IDAM server's host name or IP address
    environment->server_reconnect = 1;                           // Create a new Server instance
    env_host = 0;                                               // Skip initialsisation at Startup if these are called first
    env_port = 0;
}

//! Set the IDAM data server host name
/** This takes precedence over the environment variables IDAM_HOST.
* @param host The name of the server host computer.
* @return void
*/
void putIdamServerHost(const char* host)
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    strcpy(environment->server_host, host);                      // IDAM server's host name or IP address
    environment->server_reconnect = 1;                           // Create a new Server instance
    env_host = 0;
}

//! Set the IDAM data server port number
/** This takes precedence over the environment variables IDAM_PORT.
* @param port The port number the server is connected to.
* @return void
*/
void putIdamServerPort(int port)
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    environment->server_port = port;                             // IDAM server service port number
    environment->server_reconnect = 1;                           // Create a new Server instance
    env_port = 0;
}

//! Specify a specific IDAM server socket connection to use
/** The client can be connected to multiple servers, distinguished by their socket id.
Select the server connection required.
* @param socket The socket ID of the server connection required.
* @return void
*/
void putIdamServerSocket(int socket)
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    environment->server_socket = socket;                         // IDAM server service socket number (Must be Open)
    environment->server_change_socket = 1;                       // Connect to an Existing Server
}

//--------------------------------------------------------------
// Standard C GET Accessor Routines

//! Return the IDAM data server host name, port number and socket connection id
/**
* @param host A preallocated string that will contain the name of the server host computer.
* @param port Returned port number.
* @param socket Returned socket id number.
* @return void
*/
void getIdamServer(char** host, int* port, int* socket)
{      // Active ...
    ENVIRONMENT* environment = getIdamClientEnvironment();
    *socket = environment->server_socket;                        // IDAM server service socket number
    *port = environment->server_port;                          // IDAM server service port number
    *host = environment->server_host;                          // IDAM server's host name or IP address
}

//! the IDAM server connection host name
/**
* @return the Name of the Host
*/
char* getIdamServerHost()
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    return environment->server_host;                             // Active IDAM server's host name or IP address
}

//! the IDAM server connection port number
/**
* @return the Name of the Host
*/
int getIdamServerPort()
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    return environment->server_port;                             // Active IDAM server service port number
}

//! the IDAM server connection socket ID
/**
* @return the connection socket ID
*/
int getIdamServerSocket()
{
    ENVIRONMENT* environment = getIdamClientEnvironment();
    return environment->server_socket;           // Active IDAM server service socket number
}

//!  returns the data access error code
/**
\param   handle   The data object handle.
\return   Return error code, if non-zero there was a problem: < 0 is client side, > 0 is server side.
*/
int getIdamErrorCode(int handle)
{              // Error Code Returned from Server
    if (handle < 0 || handle >= Data_Block_Count) {
        return getIdamServerErrorStackRecordCode(0);
    } else {
        return (int)Data_Block[handle].errcode;
    }
}

//!  returns the data access error message
/**
\param   handle   The data object handle.
\return   the error message.
*/
char* getIdamErrorMsg(int handle)
{             // Error Message returned from server
    if (handle < 0 || handle >= Data_Block_Count) {
        return getIdamServerErrorStackRecordMsg(0);
    } else {
        return Data_Block[handle].error_msg;
    }
}

//!  returns the data source quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int getIdamSourceStatus(int handle)
{           // Source Status
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].source_status;
}

//!  returns the data object quality status
/**
\param   handle   The data object handle.
\return   Quality status.
*/
int getIdamSignalStatus(int handle)
{           // Signal Status
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].signal_status;
}

int getIdamDataStatus(int handle)
{          // Data Status based on Standard Rule
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    if (getIdamSignalStatus(handle) ==
        DEFAULT_STATUS) {       // Signal Status Not Changed from Default - use Data Source Value
        return Data_Block[handle].source_status;
    } else {
        return Data_Block[handle].signal_status;
    }
}

//!  returns the last data object handle issued
/**
\return   handle.
*/
int getIdamLastHandle()
{
    return acc_getCurrentDataBlockIndex();
}

//!  returns the number of data items in the data object
/** the number of array elements
\param   handle   The data object handle
\return  the number of data items
*/
int getIdamDataNum(int handle)
{             // Data Array Size
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].data_n;
}

//!  returns the rank of the data object
/** the number of array coordinate dimensions
\param   handle   The data object handle
\return  the rank
*/
int getIdamRank(int handle)
{             // Array Rank
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return (int)Data_Block[handle].rank;
}

//!  Returns the position of the time coordinate dimension in the data object
/** For example, a rank 3 array data[time][x][y] (in Fortran and IDL this is data(y,x,time)) has time order = 0 so order is
counted from left to right in c and from right to left in Fortran and IDL.
\param   handle   The data object handle
\return  the time coordinate dimension position
*/
int getIdamOrder(int handle)
{               // Time Dimension Order in Array
    if (handle < 0 || handle >= Data_Block_Count) return -1;
    return Data_Block[handle].order;
}

/**
 * Returns the Server's Permission to locally Cache data
 * @param handle The data object handle
 * @return the permission
 */
unsigned int getIdamCachePermission(int handle)
{     // Permission to cache?
    if (handle < 0 || handle >= Data_Block_Count) return PLUGINNOTOKTOCACHE;
    return Data_Block[handle].cachePermission;
}

/**
 * Returns the total amount of data (bytes)
 *
 * @param handle The data object handle
 * @return byte count
 */
unsigned int getIdamTotalDataBlockSize(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].totalDataBlockSize;
}

//!  returns the atomic or structure type id of the data object
/**
\param   handle   The data object handle
\return  the type id
*/
int getIdamDataType(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return TYPE_UNKNOWN;
    return Data_Block[handle].data_type;
}

int getIdamDataOpaqueType(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return TYPE_UNKNOWN;
    return Data_Block[handle].opaque_type;
}

void* getIdamDataOpaqueBlock(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return TYPE_UNKNOWN;
    return Data_Block[handle].opaque_block;
}

int getIdamDataOpaqueCount(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return TYPE_UNKNOWN;
    return Data_Block[handle].opaque_count;
}

//!  returns the atomic or structure type id of the error data object
/**
\param   handle   The data object handle
\return  the type id
*/
int getIdamErrorType(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return TYPE_UNKNOWN;
    return Data_Block[handle].error_type;
}

//!  returns the atomic or structure type id of a named type
/**
\param   type   The name of the type
\return  the type id
*/
int getIdamDataTypeId(const char* type)
{ // Return the Internal Code for Data Types
    if (STR_IEQUALS(type, "dcomplex")) return TYPE_DCOMPLEX;
    if (STR_IEQUALS(type, "complex")) return TYPE_COMPLEX;
    if (STR_IEQUALS(type, "double")) return TYPE_DOUBLE;
    if (STR_IEQUALS(type, "float")) return TYPE_FLOAT;
    if (STR_IEQUALS(type, "long64")) return TYPE_LONG64;
    if (STR_IEQUALS(type, "long long")) return TYPE_LONG64;
#ifndef __APPLE__
    if (STR_IEQUALS(type, "ulong64")) return TYPE_UNSIGNED_LONG64;
    if (STR_IEQUALS(type, "unsigned long64")) return TYPE_UNSIGNED_LONG64;
    if (STR_IEQUALS(type, "unsigned long long")) return TYPE_UNSIGNED_LONG64;
#endif
    if (STR_IEQUALS(type, "long")) return TYPE_LONG;
    if (STR_IEQUALS(type, "unsigned long")) return TYPE_UNSIGNED_LONG;
    if (STR_IEQUALS(type, "int")) return TYPE_INT;
    if (STR_IEQUALS(type, "integer")) return TYPE_INT;
    if (STR_IEQUALS(type, "unsigned")) return TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "unsigned int")) return TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "unsigned integer")) return TYPE_UNSIGNED_INT;
    if (STR_IEQUALS(type, "short")) return TYPE_SHORT;
    if (STR_IEQUALS(type, "unsigned short")) return TYPE_UNSIGNED_SHORT;
    if (STR_IEQUALS(type, "char")) return TYPE_CHAR;
    if (STR_IEQUALS(type, "unsigned char")) return TYPE_UNSIGNED_CHAR;
    if (STR_IEQUALS(type, "unknown")) return TYPE_UNKNOWN;
    if (STR_IEQUALS(type, "undefined")) return TYPE_UNDEFINED;

    if (STR_IEQUALS(type, "vlen")) return TYPE_VLEN;
    if (STR_IEQUALS(type, "compound")) return TYPE_COMPOUND;
    if (STR_IEQUALS(type, "opaque")) return TYPE_OPAQUE;
    if (STR_IEQUALS(type, "enum")) return TYPE_ENUM;
    if (STR_IEQUALS(type, "string")) return TYPE_STRING;
    if (STR_IEQUALS(type, "void")) return TYPE_VOID;

    if (STR_IEQUALS(type, "string *")) return TYPE_STRING;

    return TYPE_UNKNOWN;
}

int getIdamDataTypeSize(int type)
{   // Return the size of the Data Type

    switch (type) {
        case TYPE_DCOMPLEX:
            return 0;
        case TYPE_COMPLEX:
            return 0;
        case TYPE_DOUBLE:
            return 8;
        case TYPE_FLOAT:
            return 4;
        case TYPE_LONG64:
            return 8;
        case TYPE_UNSIGNED_LONG64:
            return 8;
        case TYPE_LONG:
            return 8;
        case TYPE_UNSIGNED_LONG:
            return 8;
        case TYPE_INT:
            return 4;
        case TYPE_UNSIGNED_INT:
            return 4;
        case TYPE_SHORT:
            return 2;
        case TYPE_UNSIGNED_SHORT:
            return 2;
        case TYPE_CHAR:
            return 1;
        case TYPE_UNSIGNED_CHAR:
            return 1;
        case TYPE_UNKNOWN:
            return 0;
        case TYPE_UNDEFINED:
            return 0;
        case TYPE_VLEN:
            return 0;
        case TYPE_COMPOUND:
            return 0;
        case TYPE_OPAQUE:
            return 0;
        case TYPE_ENUM:
            return 4;
        case TYPE_STRING:
            return 0;
        case TYPE_VOID:
            return 8;
        default:
            return -1;
    }
}

void getIdamErrorModel(int handle, int* model, int* param_n, float* params)
{
    int i;
    if (handle < 0 || handle >= Data_Block_Count) {
        *model = ERROR_MODEL_UNKNOWN;
        *param_n = 0;
        return;
    }
    *model = Data_Block[handle].error_model;     // Model ID
    *param_n = Data_Block[handle].error_param_n;      // Number of parameters
    for (i = 0; i < Data_Block[handle].error_param_n; i++) params[i] = Data_Block[handle].errparams[i];
}

int getIdamErrorAsymmetry(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return (int)Data_Block[handle].errasymmetry;
}

// Return the Internal Code for a named Error Model

int getIdamErrorModelId(const char* model)
{
    int i;
    for (i = 1; i < ERROR_MODEL_UNDEFINED; i++) {
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

char* acc_getSyntheticData(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].synthetic;
}

char* acc_getSyntheticDimData(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].dims[ndim].synthetic;
}

void acc_setSyntheticData(int handle, char* data)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].synthetic = data;
}

void acc_setSyntheticDimData(int handle, int ndim, char* data)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].dims[ndim].synthetic = data;
}

char* getIdamSyntheticData(int handle)
{
    int status = getIdamDataStatus(handle);
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    if (status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return NULL;
    if (status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return NULL;
    if (!get_synthetic || Data_Block[handle].error_model == ERROR_MODEL_UNKNOWN) {
        return Data_Block[handle].data;
    }
    generateIdamSyntheticData(handle);
    return Data_Block[handle].synthetic;
}

//!  Returns a pointer to the requested data
/** The data may be synthetically generated.
\param   handle   The data object handle
\return  a pointer to the data - if the status is poor, a NULL pointer is returned unless the \e get_bad property is set.
*/
char* getIdamData(int handle)
{
    int status = getIdamDataStatus(handle);
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    if (status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return NULL;
    if (status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return NULL;
    if (!get_synthetic) {
        return Data_Block[handle].data;
    } else {
        return getIdamSyntheticData(handle);
    }
}

//! Copy the requested data block to a data buffer for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   char  A preallocated memory block to receive a copy of the data
\return  void
*/
void getIdamDataTdi(int handle, char* data)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    memcpy(data, (void*)Data_Block[handle].data, (int)Data_Block[handle].data_n);
}

char* getIdamDataErrLo(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].errlo;
}

char* getIdamDataErrHi(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].errhi;
}

int getIdamDataErrAsymmetry(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].errasymmetry;
}

void acc_setIdamDataErrAsymmetry(int handle, int asymmetry)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].errasymmetry = asymmetry;
};

void acc_setIdamDataErrType(int handle, int type)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].error_type = type;
};

void acc_setIdamDataErrLo(int handle, char* errlo)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].errlo = errlo;
};

char* getIdamDimErrLo(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].dims[ndim].errlo;
}

char* getIdamDimErrHi(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].dims[ndim].errhi;
}

int getIdamDimErrAsymmetry(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    return Data_Block[handle].dims[ndim].errasymmetry;
}

void acc_setIdamDimErrAsymmetry(int handle, int ndim, int asymmetry)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].dims[ndim].errasymmetry = asymmetry;
};

void acc_setIdamDimErrType(int handle, int ndim, int type)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].dims[ndim].error_type = type;
};

void acc_setIdamDimErrLo(int handle, int ndim, char* errlo)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    Data_Block[handle].dims[ndim].errlo = errlo;
};

char* getIdamAsymmetricError(int handle, int above)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    if (Data_Block[handle].error_type != TYPE_UNKNOWN) {
        if (above) {
            return Data_Block[handle].errhi;      // return the default error array
        } else {
            if (!Data_Block[handle].errasymmetry) {
                return Data_Block[handle].errhi;     // return the default error array if symmetric errors
            } else {
                return Data_Block[handle].errlo;
            }     // otherwise the data array must have been returned by the server or generated
        }
    } else {
        if (Data_Block[handle].error_model != ERROR_MODEL_UNKNOWN) {
            generateIdamDataError(handle);            // Create the errors from a model if the model exits
            if (above) {
                return Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                return Data_Block[handle].errhi;
            } else {
                return Data_Block[handle].errlo;
            }
        } else {

            char* errhi = NULL;    // Regular Error Component
            char* errlo = NULL;    // Asymmetric Error Component
            int i, ndata;

            ndata = Data_Block[handle].data_n;
            Data_Block[handle].error_type = Data_Block[handle].data_type;  // Error Type is Unknown so Assume Data's Data Type

            if (allocArray(Data_Block[handle].error_type, ndata, &errhi) != 0) {
                // Allocate Heap for Regular Error Data
                IDAM_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Data Errors\n");
                Data_Block[handle].errhi = NULL;
            } else {
                Data_Block[handle].errhi = errhi;
            }

            if (Data_Block[handle].errasymmetry) {           // Allocate Heap for the Asymmetric Error Data
                if (allocArray(Data_Block[handle].error_type, ndata, &errlo) != 0) {
                    IDAM_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Asymmetric Errors\n");
                    IDAM_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    Data_Block[handle].errlo = NULL;
                    Data_Block[handle].errasymmetry = 0;
                } else {
                    Data_Block[handle].errlo = errlo;
                }
            }

// Generate and return Zeros if this data is requested unless Error is Modelled

            switch (Data_Block[handle].data_type) {
                case TYPE_FLOAT: {
                    float* fh, * fl;
                    fh = (float*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) fl = (float*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(fh + i) = (float)0.0;
                        if (Data_Block[handle].errasymmetry) *(fl + i) = (float)0.0;
                    }
                    break;
                }
                case TYPE_DOUBLE: {
                    double* dh, * dl;
                    dh = (double*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) dl = (double*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(dh + i) = (double)0.0;
                        if (Data_Block[handle].errasymmetry) *(dl + i) = (double)0.0;
                    }
                    break;
                }
                case TYPE_SHORT: {
                    short* sh, * sl;
                    sh = (short*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) sl = (short*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(sh + i) = (short)0;
                        if (Data_Block[handle].errasymmetry) *(sl + i) = (short)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_SHORT: {
                    unsigned short* sh, * sl;
                    sh = (unsigned short*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) sl = (unsigned short*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (Data_Block[handle].errasymmetry) sl[i] = (unsigned short)0;
                    }
                    break;
                }
                case TYPE_INT: {
                    int* ih, * il;
                    ih = (int*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) il = (int*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(ih + i) = (int)0;
                        if (Data_Block[handle].errasymmetry) *(il + i) = (int)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_INT: {
                    unsigned int* uh, * ul;
                    uh = (unsigned int*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) ul = (unsigned int*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(uh + i) = (unsigned int)0;
                        if (Data_Block[handle].errasymmetry) *(ul + i) = (unsigned int)0;
                    }
                    break;
                }
                case TYPE_LONG: {
                    long* lh, * ll;
                    lh = (long*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) ll = (long*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(lh + i) = (long)0;
                        if (Data_Block[handle].errasymmetry) *(ll + i) = (long)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_LONG: {
                    unsigned long* lh, * ll;
                    lh = (unsigned long*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) ll = (unsigned long*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (Data_Block[handle].errasymmetry) ll[i] = (unsigned long)0;
                    }
                    break;
                }
                case TYPE_LONG64: {
                    long long int* lh, * ll;
                    lh = (long long int*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) ll = (long long int*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(lh + i) = (long long int)0;
                        if (Data_Block[handle].errasymmetry) *(ll + i) = (long long int)0;
                    }
                    break;
                }
#ifndef __APPLE__
                case TYPE_UNSIGNED_LONG64: {
                    unsigned long long int* lh, * ll;
                    lh = (unsigned long long int*) Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) ll = (unsigned long long int*) Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int) 0;
                        if (Data_Block[handle].errasymmetry) ll[i] = (unsigned long long int) 0;
                    }
                    break;
                }
#endif
                case TYPE_CHAR: {
                    char* ch, * cl;
                    ch = Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) cl = Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i] = (char)0;
                        if (Data_Block[handle].errasymmetry) cl[i] = (char)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_CHAR: {
                    unsigned char* ch, * cl;
                    ch = (unsigned char*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) cl = (unsigned char*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (Data_Block[handle].errasymmetry) cl[i] = (unsigned char)0;
                    }
                    break;
                }
                case TYPE_DCOMPLEX: {
                    DCOMPLEX* ch, * cl;
                    ch = (DCOMPLEX*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) cl = (DCOMPLEX*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i].real = (double)0.0;
                        ch[i].imaginary = (double)0.0;
                        if (Data_Block[handle].errasymmetry) {
                            cl[i].real = (double)0.0;
                            cl[i].imaginary = (double)0.0;
                        }
                    }
                    break;
                }
                case TYPE_COMPLEX: {
                    COMPLEX* ch, * cl;
                    ch = (COMPLEX*)Data_Block[handle].errhi;
                    if (Data_Block[handle].errasymmetry) cl = (COMPLEX*)Data_Block[handle].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i].real = (float)0.0;
                        ch[i].imaginary = (float)0.0;
                        if (Data_Block[handle].errasymmetry) {
                            cl[i].real = (float)0.0;
                            cl[i].imaginary = (float)0.0;
                        }
                    }
                    break;
                }
            }

            if (above) {
                return Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                return Data_Block[handle].errhi;
            } else {
                return Data_Block[handle].errlo;
            }
        }
    }
}

//!  Returns a pointer to the memory block containing the requested error data
/** The error data may be synthetically generated.
\param   handle   The data object handle
\return  a pointer to the data
*/
char* getIdamError(int handle)
{
    int above = 1;
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return getIdamAsymmetricError(handle, above);
}

//!  Returns data cast to double precision
/** The copy buffer must be preallocated and sized for the data type. The data may be synthetically generated. If the status of the data is poor, no copy to the buffer occurs unless
the property \b get_bad is set.
\param   handle   The data object handle
\param   fp A \b double pointer to a preallocated data buffer
\return  void
*/
void getIdamDoubleData(int handle, double* fp)
{      // Copy Data cast as double to User Provided Array

// **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    int status = getIdamDataStatus(handle);
    if (handle < 0 || handle >= Data_Block_Count) return;
    if (status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return;
    if (status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return;

    if (Data_Block[handle].data_type == TYPE_DOUBLE) {
        if (!get_synthetic)
            memcpy((void*)fp, (void*)Data_Block[handle].data, (size_t)Data_Block[handle].data_n * sizeof(double));
        else {
            generateIdamSyntheticData(handle);
            if (Data_Block[handle].synthetic != NULL)
                memcpy((void*)fp, (void*)Data_Block[handle].synthetic,
                       (size_t)Data_Block[handle].data_n * sizeof(double));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].data,
                       (size_t)Data_Block[handle].data_n * sizeof(double));
            return;
        }
    } else {

        char* array;
        int i, ndata;

        ndata = getIdamDataNum(handle);

        if (!get_synthetic) {
            array = Data_Block[handle].data;
        } else {
            generateIdamSyntheticData(handle);
            if (Data_Block[handle].synthetic != NULL) {
                array = Data_Block[handle].synthetic;
            } else {
                array = Data_Block[handle].data;
            }
        }

        switch (Data_Block[handle].data_type) {
            case TYPE_FLOAT: {
                float* dp = (float*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)dp[i];
                break;
            }
            case TYPE_SHORT: {
                short* sp = (short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case TYPE_UNSIGNED_SHORT: {
                unsigned short* sp = (unsigned short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case TYPE_INT: {
                int* ip = (int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)ip[i];
                break;
            }
            case TYPE_UNSIGNED_INT: {
                unsigned int* up = (unsigned int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)up[i];
                break;
            }
            case TYPE_LONG: {
                long* lp = (long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case TYPE_UNSIGNED_LONG: {
                unsigned long* lp = (unsigned long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case TYPE_LONG64: {
                long long int* lp = (long long int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64: {
                unsigned long long int* lp = (unsigned long long int*) array;
                for (i = 0; i < ndata; i++) fp[i] = (double) lp[i];
                break;
            }
#endif
            case TYPE_CHAR: {
                char* cp = (char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case TYPE_UNSIGNED_CHAR: {
                unsigned char* cp = (unsigned char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case TYPE_UNKNOWN: {
                for (i = 0; i < ndata; i++) fp[i] = (double)0.0;  // No Data !
                break;
            }
            case TYPE_DCOMPLEX: {
                int j = 0;
                DCOMPLEX* dp = (DCOMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            case TYPE_COMPLEX: {
                int j = 0;
                COMPLEX* dp = (COMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (double)dp[i].real;
                    fp[j++] = (double)dp[i].imaginary;
                }
                break;
            }
            default:
                for (i = 0; i < ndata; i++) fp[i] = (double)0.0;
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
void getIdamFloatData(int handle, float* fp)
{     // Copy Data cast as float to User Provided Array

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    int status = getIdamDataStatus(handle);
    if (handle < 0 || handle >= Data_Block_Count) return;
    if (status == MIN_STATUS && !Data_Block[handle].client_block.get_bad && !get_bad) return;
    if (status != MIN_STATUS && (Data_Block[handle].client_block.get_bad || get_bad)) return;

    if (Data_Block[handle].data_type == TYPE_FLOAT) {
        if (!get_synthetic)
            memcpy((void*)fp, (void*)Data_Block[handle].data, (size_t)Data_Block[handle].data_n * sizeof(float));
        else {
            generateIdamSyntheticData(handle);
            if (Data_Block[handle].synthetic != NULL)
                memcpy((void*)fp, (void*)Data_Block[handle].synthetic,
                       (size_t)Data_Block[handle].data_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].data,
                       (size_t)Data_Block[handle].data_n * sizeof(float));
            return;
        }
    } else {

        char* array;
        int i, ndata;

        ndata = getIdamDataNum(handle);

        if (!get_synthetic) {
            array = Data_Block[handle].data;
        } else {
            generateIdamSyntheticData(handle);
            if (Data_Block[handle].synthetic != NULL) {
                array = Data_Block[handle].synthetic;
            } else {
                array = Data_Block[handle].data;
            }
        }

        switch (Data_Block[handle].data_type) {
            case TYPE_DOUBLE: {
                double* dp = (double*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)dp[i];
                break;
            }
            case TYPE_SHORT: {
                short* sp = (short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case TYPE_UNSIGNED_SHORT: {
                unsigned short* sp = (unsigned short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case TYPE_INT: {
                int* ip = (int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)ip[i];
                break;
            }
            case TYPE_UNSIGNED_INT: {
                unsigned int* up = (unsigned int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)up[i];
                break;
            }
            case TYPE_LONG: {
                long* lp = (long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case TYPE_UNSIGNED_LONG: {
                unsigned long* lp = (unsigned long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case TYPE_LONG64: {
                long long int* lp = (long long int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64: {
                unsigned long long int* lp = (unsigned long long int*) array;
                for (i = 0; i < ndata; i++) fp[i] = (float) lp[i];
                break;
            }
#endif
            case TYPE_CHAR: {
                char* cp = (char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case TYPE_UNSIGNED_CHAR: {
                unsigned char* cp = (unsigned char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case TYPE_UNKNOWN: {
                for (i = 0; i < ndata; i++) fp[i] = (float)0.0;   // No Data !
                break;
            }
            case TYPE_DCOMPLEX: {
                int j = 0;
                DCOMPLEX* dp = (DCOMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            case TYPE_COMPLEX: {
                int j = 0;
                COMPLEX* dp = (COMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (float)dp[i].real;
                    fp[j++] = (float)dp[i].imaginary;
                }
                break;
            }
            default:
                for (i = 0; i < ndata; i++) fp[i] = (float)0.0;
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
void getIdamGenericData(int handle, void* data)
{
    switch (getIdamDataType(handle)) {
        case TYPE_FLOAT:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(float));
            break;
        case TYPE_DOUBLE:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(double));
            break;
        case TYPE_INT:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(int));
            break;
        case TYPE_UNSIGNED_INT:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(unsigned int));
            break;
        case TYPE_LONG:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(long));
            break;
        case TYPE_UNSIGNED_LONG:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(unsigned long));
            break;
        case TYPE_LONG64:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(long long int));
            break;
        case TYPE_UNSIGNED_LONG64:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(unsigned long long int));
            break;
        case TYPE_SHORT:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(short));
            break;
        case TYPE_UNSIGNED_SHORT:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(unsigned short));
            break;
        case TYPE_CHAR:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(char));
            break;
        case TYPE_UNSIGNED_CHAR:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(unsigned char));
            break;
        case TYPE_DCOMPLEX:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(DCOMPLEX));
            break;
        case TYPE_COMPLEX:
            memcpy(data, (void*)getIdamData(handle), (size_t)getIdamDataNum(handle) * sizeof(COMPLEX));
            break;
    }
    return;
}


void getIdamFloatAsymmetricError(int handle, int above, float* fp)
{    // Copy Error Data cast as float to User Provided Array

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    int i, ndata;

    if (handle < 0 || handle >= Data_Block_Count) return;

    ndata = Data_Block[handle].data_n;

    if (Data_Block[handle].error_type == TYPE_UNKNOWN) {
        getIdamAsymmetricError(handle, above);
    } // Create the Error Data prior to Casting

    switch (Data_Block[handle].error_type) {
        case TYPE_UNKNOWN:
            for (i = 0; i < ndata; i++) fp[i] = (float)0.0; // No Error Data
            break;
        case TYPE_FLOAT:
            if (above)
                memcpy((void*)fp, (void*)Data_Block[handle].errhi,
                       (size_t)Data_Block[handle].data_n * sizeof(float));
            else if (!Data_Block[handle].errasymmetry)
                memcpy((void*)fp, (void*)Data_Block[handle].errhi,
                       (size_t)Data_Block[handle].data_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].errlo,
                       (size_t)Data_Block[handle].data_n * sizeof(float));
            break;
        case TYPE_DOUBLE: {
            double* dp;
            if (above) {
                dp = (double*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                dp = (double*)Data_Block[handle].errhi;
            } else {
                dp = (double*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)dp[i];
            break;
        }
        case TYPE_SHORT: {
            short* sp;
            if (above) {
                sp = (short*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                sp = (short*)Data_Block[handle].errhi;
            } else {
                sp = (short*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            if (above) {
                sp = (unsigned short*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                sp = (unsigned short*)Data_Block[handle].errhi;
            } else {
                sp = (unsigned short*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case TYPE_INT: {
            int* ip;
            if (above) {
                ip = (int*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                ip = (int*)Data_Block[handle].errhi;
            } else {
                ip = (int*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)ip[i];
            break;
        }
        case TYPE_UNSIGNED_INT: {
            unsigned int* up;
            if (above) {
                up = (unsigned int*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                up = (unsigned int*)Data_Block[handle].errhi;
            } else {
                up = (unsigned int*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)up[i];
            break;
        }
        case TYPE_LONG: {
            long* lp;
            if (above) {
                lp = (long*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                lp = (long*)Data_Block[handle].errhi;
            } else {
                lp = (long*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            if (above) {
                lp = (unsigned long*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                lp = (unsigned long*)Data_Block[handle].errhi;
            } else {
                lp = (unsigned long*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case TYPE_LONG64: {
            long long int* lp;
            if (above) {
                lp = (long long int*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                lp = (long long int*)Data_Block[handle].errhi;
            } else {
                lp = (long long int*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64: {
            unsigned long long int* lp;
            if (above)
                lp = (unsigned long long int*) Data_Block[handle].errhi;
            else if (!Data_Block[handle].errasymmetry)
                lp = (unsigned long long int*) Data_Block[handle].errhi;
            else
                lp = (unsigned long long int*) Data_Block[handle].errlo;
            for (i = 0; i < ndata; i++) fp[i] = (float) lp[i];
            break;
        }
#endif
        case TYPE_CHAR: {
            char* cp;
            if (above) {
                cp = Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                cp = Data_Block[handle].errhi;
            } else {
                cp = Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            if (above) {
                cp = (unsigned char*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                cp = (unsigned char*)Data_Block[handle].errhi;
            } else {
                cp = (unsigned char*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case TYPE_DCOMPLEX: {
            int j = 0;
            DCOMPLEX* cp;
            if (above) {
                cp = (DCOMPLEX*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                cp = (DCOMPLEX*)Data_Block[handle].errhi;
            } else {
                cp = (DCOMPLEX*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case TYPE_COMPLEX: {
            int j = 0;
            COMPLEX* cp;
            if (above) {
                cp = (COMPLEX*)Data_Block[handle].errhi;
            } else if (!Data_Block[handle].errasymmetry) {
                cp = (COMPLEX*)Data_Block[handle].errhi;
            } else {
                cp = (COMPLEX*)Data_Block[handle].errlo;
            }
            for (i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        default:
            for (i = 0; i < ndata; i++) fp[i] = (float)0.0;
            break;

    }
    return;
}

//!  Returns error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void getIdamFloatError(int handle, float* fp)
{
    int above = 1;
    getIdamFloatAsymmetricError(handle, above, fp);
}

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**
\param   handle   The data object handle
\param   db Returned \b DATA_BLOCK pointer
\return  void
*/
void getIdamDBlock(int handle, DATA_BLOCK* db)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    *db = Data_Block[handle];
}

//!  Returns the DATA_BLOCK data structure - the data, dimension coordinates and associated meta data.
/**
\param   handle   The data object handle
\return  DATA_BLOCK pointer
*/
DATA_BLOCK* getIdamDataBlock(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return &Data_Block[handle];
}

//!  Returns the data label of a data object
/**
\param   handle   The data object handle
\return  pointer to the data label
*/
char* getIdamDataLabel(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].data_label;
}

//!  Returns the data label of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   label   preallocated string buffer to receive the copy of the data label
\return  void
*/
void getIdamDataLabelTdi(int handle, char* label)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    strcpy(label, Data_Block[handle].data_label);
}

//!  Returns the data units of a data object
/**
\param   handle   The data object handle
\return  pointer to the data units
*/
char* getIdamDataUnits(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].data_units;
}

//!  Returns the data units of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   units   preallocated string buffer to receive the copy of the data units
\return  void
*/
void getIdamDataUnitsTdi(int handle, char* units)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    strcpy(units, Data_Block[handle].data_units);
}

//!  Returns the description of a data object
/**
\param   handle   The data object handle
\return  pointer to the data description
*/
char* getIdamDataDesc(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return Data_Block[handle].data_desc;
}

//!  Returns the description of a data object for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   units   preallocated string buffer to receive the copy of the data description
\return  void
*/
void getIdamDataDescTdi(int handle, char* desc)
{
    if (handle < 0 || handle >= Data_Block_Count) return;
    strcpy(desc, Data_Block[handle].data_desc);
}

// Dimension Coordinates

//! Returns the coordinate dimension size
/** the number of elements in the coordinate array
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the dimension size
*/
int getIdamDimNum(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return 0;
    return (int)Data_Block[handle].dims[ndim].dim_n;
}

//! Returns the coordinate dimension data type
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int getIdamDimType(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return TYPE_UNKNOWN;
    return (int)Data_Block[handle].dims[ndim].data_type;
}

//! Returns the coordinate dimension error data type
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  the data type id
*/
int getIdamDimErrorType(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return TYPE_UNKNOWN;
    return (int)Data_Block[handle].dims[ndim].error_type;
}

//! Returns whether or not coordinate error data are asymmetric.
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  boolean true or false i.e. 1 or 0
*/
int getIdamDimErrorAsymmetry(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return 0;
    return (int)Data_Block[handle].dims[ndim].errasymmetry;
}

void getIdamDimErrorModel(int handle, int ndim, int* model, int* param_n, float* params)
{
    int i;
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) {

        *model = ERROR_MODEL_UNKNOWN;
        *param_n = 0;
        // *params  = NULL;
        return;
    }
    *model = Data_Block[handle].dims[ndim].error_model;      // Model ID
    *param_n = Data_Block[handle].dims[ndim].error_param_n;    // Number of parameters
    for (i = 0; i < Data_Block[handle].dims[ndim].error_param_n; i++)
        params[i] = Data_Block[handle].dims[ndim].errparams[i];
    // *params  = Data_Block[handle].dims[ndim].errparams;        // Array of Model Parameters
}

char* getIdamSyntheticDimData(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    if (!get_synthetic || Data_Block[handle].dims[ndim].error_model == ERROR_MODEL_UNKNOWN) {
        return Data_Block[handle].dims[ndim].dim;
    }
    generateIdamSyntheticDimData(handle, ndim);
    return Data_Block[handle].dims[ndim].synthetic;
}

///!  Returns a pointer to the requested coordinate data
/** The data may be synthetically generated.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data
*/
char* getIdamDimData(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    if (!get_synthetic) return Data_Block[handle].dims[ndim].dim;
    return getIdamSyntheticDimData(handle, ndim);
}

//! Returns the data label of a coordinate dimension
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data label
*/
char* getIdamDimLabel(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    return Data_Block[handle].dims[ndim].dim_label;
}
//! Returns the data units of a coordinate dimension
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  pointer to the data units
*/
char* getIdamDimUnits(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    return Data_Block[handle].dims[ndim].dim_units;
}

//!  Returns the data label of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   label   preallocated string buffer to receive the copy of the data label
\return  void
*/
void getIdamDimLabelTdi(int handle, int ndim, char* label)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
    strcpy(label, Data_Block[handle].dims[ndim].dim_label);
}

//!  Returns the data units of a coordinate dimension for use in MDS+ TDI functions
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   units   preallocated string buffer to receive the copy of the data units
\return  void
*/
void getIdamDimUnitsTdi(int handle, int ndim, char* units)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
    strcpy(units, Data_Block[handle].dims[ndim].dim_units);
}

//!  Returns coordinate data cast to double precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b double pointer to a preallocated data buffer
\return  void
*/
void getIdamDoubleDimData(int handle, int ndim, double* fp)
{

// **** The double array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
    if (Data_Block[handle].dims[ndim].data_type == TYPE_DOUBLE) {
        if (!get_synthetic)
            memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].dim,
                   (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(double));
        else {
            generateIdamSyntheticDimData(handle, ndim);
            if (Data_Block[handle].dims[ndim].synthetic != NULL)
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].synthetic,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(double));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].dim,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(double));
            return;
        }
    } else {
        char* array;
        int i, ndata;

        ndata = Data_Block[handle].dims[ndim].dim_n;
        if (!get_synthetic) {
            array = Data_Block[handle].dims[ndim].dim;
        } else {
            generateIdamSyntheticDimData(handle, ndim);
            if (Data_Block[handle].dims[ndim].synthetic != NULL) {
                array = Data_Block[handle].dims[ndim].synthetic;
            } else {
                array = Data_Block[handle].dims[ndim].dim;
            }
        }

        switch (Data_Block[handle].dims[ndim].data_type) {
            case TYPE_FLOAT: {
                float* dp = (float*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)dp[i];
                break;
            }
            case TYPE_SHORT: {
                short* sp = (short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case TYPE_UNSIGNED_SHORT: {
                unsigned short* sp = (unsigned short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)sp[i];
                break;
            }
            case TYPE_INT: {
                int* ip = (int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)ip[i];
                break;
            }
            case TYPE_UNSIGNED_INT: {
                unsigned int* up = (unsigned int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)up[i];
                break;
            }
            case TYPE_LONG: {
                long* lp = (long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case TYPE_UNSIGNED_LONG: {
                unsigned long* lp = (unsigned long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
            case TYPE_LONG64: {
                long long int* lp = (long long int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)lp[i];
                break;
            }
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64: {
                unsigned long long int* lp = (unsigned long long int*) array;
                for (i = 0; i < ndata; i++) fp[i] = (double) lp[i];
                break;
            }
#endif
            case TYPE_CHAR: {
                char* cp = (char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case TYPE_UNSIGNED_CHAR: {
                unsigned char* cp = (unsigned char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (double)cp[i];
                break;
            }
            case TYPE_DCOMPLEX: {
                int j = 0;
                DCOMPLEX* cp = (DCOMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case TYPE_COMPLEX: {
                int j = 0;
                COMPLEX* cp = (COMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (double)cp[i].real;
                    fp[j++] = (double)cp[i].imaginary;
                }
                break;
            }
            case TYPE_UNKNOWN:
                for (i = 0; i < ndata; i++) fp[i] = (double)0.0;
                break;
            default:
                for (i = 0; i < ndata; i++) fp[i] = (double)0.0;
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
void getIdamFloatDimData(int handle, int ndim, float* fp)
{

// **** The float array must be TWICE the size if the type is COMPLEX otherwise a seg fault will occur!

    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;
    if (Data_Block[handle].dims[ndim].data_type == TYPE_FLOAT) {
        if (!get_synthetic)
            memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].dim,
                   (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
        else {
            generateIdamSyntheticDimData(handle, ndim);
            if (Data_Block[handle].dims[ndim].synthetic != NULL)
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].synthetic,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].dim,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
            return;
        }
    } else {
        char* array;
        int i, ndata;

        ndata = Data_Block[handle].dims[ndim].dim_n;
        if (!get_synthetic) {
            array = Data_Block[handle].dims[ndim].dim;
        } else {
            generateIdamSyntheticDimData(handle, ndim);
            if (Data_Block[handle].dims[ndim].synthetic != NULL) {
                array = Data_Block[handle].dims[ndim].synthetic;
            } else {
                array = Data_Block[handle].dims[ndim].dim;
            }
        }

        switch (Data_Block[handle].dims[ndim].data_type) {
            case TYPE_DOUBLE: {
                double* dp = (double*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)dp[i];
                break;
            }
            case TYPE_SHORT: {
                short* sp = (short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case TYPE_UNSIGNED_SHORT: {
                unsigned short* sp = (unsigned short*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
                break;
            }
            case TYPE_INT: {
                int* ip = (int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)ip[i];
                break;
            }
            case TYPE_UNSIGNED_INT: {
                unsigned int* up = (unsigned int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)up[i];
                break;
            }
            case TYPE_LONG: {
                long* lp = (long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case TYPE_UNSIGNED_LONG: {
                unsigned long* lp = (unsigned long*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
            case TYPE_LONG64: {
                long long int* lp = (long long int*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
                break;
            }
#ifndef __APPLE__
            case TYPE_UNSIGNED_LONG64: {
                unsigned long long int* lp = (unsigned long long int*) array;
                for (i = 0; i < ndata; i++) fp[i] = (float) lp[i];
                break;
            }
#endif
            case TYPE_CHAR: {
                char* cp = (char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case TYPE_UNSIGNED_CHAR: {
                unsigned char* cp = (unsigned char*)array;
                for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
                break;
            }
            case TYPE_DCOMPLEX: {
                int j = 0;
                DCOMPLEX* cp = (DCOMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case TYPE_COMPLEX: {
                int j = 0;
                COMPLEX* cp = (COMPLEX*)array;
                for (i = 0; i < ndata; i++) {
                    fp[j++] = (float)cp[i].real;
                    fp[j++] = (float)cp[i].imaginary;
                }
                break;
            }
            case TYPE_UNKNOWN:
                for (i = 0; i < ndata; i++) fp[i] = (float)0.0;
                break;
            default:
                for (i = 0; i < ndata; i++) fp[i] = (float)0.0;
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
void getIdamGenericDimData(int handle, int ndim, void* data)
{
    switch (getIdamDimType(handle, ndim)) {
        case TYPE_FLOAT:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(float));
            break;
        case TYPE_DOUBLE:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(double));
            break;
        case TYPE_INT:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(int));
            break;
        case TYPE_LONG:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(long));
            break;
        case TYPE_LONG64:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(long long int));
            break;
        case TYPE_SHORT:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(short));
            break;
        case TYPE_CHAR:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(char));
            break;
        case TYPE_UNSIGNED_INT:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(unsigned int));
            break;
        case TYPE_UNSIGNED_LONG:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(unsigned long));
            break;
        case TYPE_UNSIGNED_LONG64:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(unsigned long long int));
            break;
        case TYPE_UNSIGNED_SHORT:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(unsigned short));
            break;
        case TYPE_UNSIGNED_CHAR:
            memcpy(data, (void*)getIdamDimData(handle, ndim),
                   (size_t)getIdamDimNum(handle, ndim) * sizeof(unsigned char));
            break;
        case TYPE_DCOMPLEX:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(DCOMPLEX));
            break;
        case TYPE_COMPLEX:
            memcpy(data, (void*)getIdamDimData(handle, ndim), (size_t)getIdamDimNum(handle, ndim) * sizeof(COMPLEX));
            break;
    }
}

//!  Returns the coordinate dimension's DIMS data structure - the coordinate data and associated meta data.
/**
\param   handle   The data object handle
\param   ndim    the position of the dimension in the data array - numbering is as data[0][1][2]
\return  DIMS pointer
*/
DIMS* getIdamDimBlock(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    return Data_Block[handle].dims + ndim;
}


char* getIdamDimAsymmetricError(int handle, int ndim, int above)
{
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    if (Data_Block[handle].dims[ndim].error_type != TYPE_UNKNOWN) {
        if (above) {
            return Data_Block[handle].dims[ndim].errhi;    // return the default error array
        } else {
            if (!Data_Block[handle].dims[ndim].errasymmetry) {
                return Data_Block[handle].dims[ndim].errhi;   // return the default error array if symmetric errors
            } else {
                return Data_Block[handle].dims[ndim].errlo;
            }   // otherwise the data array must have been returned by the server
        }                           // or generated in a previous call
    } else {
        if (Data_Block[handle].dims[ndim].error_model != ERROR_MODEL_UNKNOWN) {
            generateIdamDimDataError(handle, ndim);
            if (above) {
                return Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                return Data_Block[handle].dims[ndim].errhi;
            } else {
                return Data_Block[handle].dims[ndim].errlo;
            }
        } else {
            char* errhi = NULL;
            char* errlo = NULL;
            int i, ndata;

            ndata = Data_Block[handle].dims[ndim].dim_n;
            Data_Block[handle].dims[ndim].error_type = Data_Block[handle].dims[ndim].data_type; // Error Type is Unknown so Assume Data's Data Type


            if (allocArray(Data_Block[handle].dims[ndim].error_type, ndata, &errhi) != 0) {
                IDAM_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Data Errors\n");
                Data_Block[handle].dims[ndim].errhi = NULL;
            } else {
                Data_Block[handle].dims[ndim].errhi = errhi;
            }

            if (Data_Block[handle].dims[ndim].errasymmetry) {               // Allocate Heap for the Asymmetric Error Data
                if (allocArray(Data_Block[handle].dims[ndim].error_type, ndata, &errlo) != 0) {
                    IDAM_LOG(UDA_LOG_ERROR, "Heap Allocation Problem with Dimensional Asymmetric Errors\n");
                    IDAM_LOG(UDA_LOG_ERROR, "Switching Asymmetry Off!\n");
                    Data_Block[handle].dims[ndim].errlo = errlo;
                    Data_Block[handle].dims[ndim].errasymmetry = 0;
                } else {
                    Data_Block[handle].dims[ndim].errlo = errlo;
                }
            }

            switch (Data_Block[handle].dims[ndim].data_type) {
                case TYPE_FLOAT: {
                    float* fh, * fl;
                    fh = (float*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) fl = (float*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        fh[i] = (float)0.0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) fl[i] = (float)0.0;
                    }
                    break;
                }
                case TYPE_DOUBLE: {
                    double* dh, * dl;
                    dh = (double*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) dl = (double*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        dh[i] = (double)0.0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) dl[i] = (double)0.0;
                    }
                    break;
                }
                case TYPE_SHORT: {
                    short* sh, * sl;
                    sh = (short*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) sl = (short*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        sh[i] = (short)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) sl[i] = (short)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_SHORT: {
                    unsigned short* sh, * sl;
                    sh = (unsigned short*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) sl = (unsigned short*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        sh[i] = (unsigned short)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) sl[i] = (unsigned short)0;
                    }
                    break;
                }
                case TYPE_INT: {
                    int* ih, * il;
                    ih = (int*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) il = (int*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        ih[i] = (int)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) il[i] = (int)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_INT: {
                    unsigned int* uh, * ul;
                    uh = (unsigned int*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) ul = (unsigned int*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        uh[i] = (unsigned int)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) ul[i] = (unsigned int)0;
                    }
                    break;
                }
                case TYPE_LONG: {
                    long* lh, * ll;
                    lh = (long*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) ll = (long*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (long)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (long)0;
                    }
                    break;
                }
                case TYPE_UNSIGNED_LONG: {
                    unsigned long* lh, * ll;
                    lh = (unsigned long*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) ll = (unsigned long*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (unsigned long)0;
                    }
                    break;
                }
                case TYPE_LONG64: {
                    long long int* lh, * ll;
                    lh = (long long int*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) ll = (long long int*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (long long int)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (long long int)0;
                    }
                    break;
                }
#ifndef __APPLE__
                case TYPE_UNSIGNED_LONG64: {
                    unsigned long long int* lh, * ll;
                    lh = (unsigned long long int*) Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) ll = (unsigned long long int*) Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        lh[i] = (unsigned long long int) 0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) ll[i] = (unsigned long long int) 0;
                    }
                    break;
                }
#endif
                case TYPE_CHAR: {
                    char* ch, * cl;
                    ch = Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) cl = Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        *(ch + i) = ' ';
                        if (Data_Block[handle].dims[ndim].errasymmetry) *(cl + i) = ' ';
                    }
                    break;
                }
                case TYPE_UNSIGNED_CHAR: {
                    unsigned char* ch, * cl;
                    ch = (unsigned char*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) cl = (unsigned char*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i] = (unsigned char)0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) cl[i] = (unsigned char)0;
                    }
                    break;
                }
                case TYPE_DCOMPLEX: {
                    DCOMPLEX* ch, * cl;
                    ch = (DCOMPLEX*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) cl = (DCOMPLEX*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i].real = (double)0.0;
                        ch[i].imaginary = (double)0.0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) {
                            cl[i].real = (double)0.0;
                            cl[i].imaginary = (double)0.0;
                        }
                    }
                    break;
                }
                case TYPE_COMPLEX: {
                    COMPLEX* ch, * cl;
                    ch = (COMPLEX*)Data_Block[handle].dims[ndim].errhi;
                    if (Data_Block[handle].dims[ndim].errasymmetry) cl = (COMPLEX*)Data_Block[handle].dims[ndim].errlo;
                    for (i = 0; i < ndata; i++) {
                        ch[i].real = (float)0.0;
                        ch[i].imaginary = (float)0.0;
                        if (Data_Block[handle].dims[ndim].errasymmetry) {
                            cl[i].real = (float)0.0;
                            cl[i].imaginary = (float)0.0;
                        }
                    }
                    break;
                }
            }
            return Data_Block[handle].dims[ndim].errhi;    // Errors are Symmetric at this point
        }
    }
}

//!  Returns a pointer to the requested coordinate error data
/**
\param   handle   The data object handle
\param   ndim  the position of the dimension in the data array - numbering is as data[0][1][2]
\return  a pointer to the data
*/
char* getIdamDimError(int handle, int ndim)
{
    int above = 1;
    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return NULL;
    return getIdamDimAsymmetricError(handle, ndim, above);
}

void getIdamFloatDimAsymmetricError(int handle, int ndim, int above, float* fp)
{   // Copy Error Data cast as float to User Provided Array
    int i, ndata;

    if (handle < 0 || handle >= Data_Block_Count || ndim < 0 || ndim >= Data_Block[handle].rank) return;

    ndata = Data_Block[handle].dims[ndim].dim_n;

    if (Data_Block[handle].dims[ndim].error_type == TYPE_UNKNOWN) {
        getIdamDimAsymmetricError(handle, ndim, above);
    }     // Create the Error Data prior to Casting

    switch (Data_Block[handle].dims[ndim].error_type) {
        case TYPE_UNKNOWN:
            for (i = 0; i < ndata; i++) fp[i] = (float)0.0; // No Error Data
            break;
        case TYPE_FLOAT:
            if (above)
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].errhi,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
            else if (!Data_Block[handle].dims[ndim].errasymmetry)
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].errhi,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
            else
                memcpy((void*)fp, (void*)Data_Block[handle].dims[ndim].errlo,
                       (size_t)Data_Block[handle].dims[ndim].dim_n * sizeof(float));
            break;
        case TYPE_DOUBLE: {
            double* dp;                          // Return Zeros if this data is requested unless Error is Modelled
            if (above) {
                dp = (double*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                dp = (double*)Data_Block[handle].dims[ndim].errhi;
            } else {
                dp = (double*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)dp[i];
            break;
        }
        case TYPE_SHORT: {
            short* sp;
            if (above) {
                sp = (short*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                sp = (short*)Data_Block[handle].dims[ndim].errhi;
            } else {
                sp = (short*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case TYPE_UNSIGNED_SHORT: {
            unsigned short* sp;
            if (above) {
                sp = (unsigned short*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                sp = (unsigned short*)Data_Block[handle].dims[ndim].errhi;
            } else {
                sp = (unsigned short*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)sp[i];
            break;
        }
        case TYPE_INT: {
            int* ip;
            if (above) {
                ip = (int*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                ip = (int*)Data_Block[handle].dims[ndim].errhi;
            } else {
                ip = (int*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)ip[i];
            break;
        }
        case TYPE_UNSIGNED_INT: {
            unsigned int* up;
            if (above) {
                up = (unsigned int*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                up = (unsigned int*)Data_Block[handle].dims[ndim].errhi;
            } else {
                up = (unsigned int*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)up[i];
            break;
        }
        case TYPE_LONG: {
            long* lp;
            if (above) {
                lp = (long*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                lp = (long*)Data_Block[handle].dims[ndim].errhi;
            } else {
                lp = (long*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case TYPE_UNSIGNED_LONG: {
            unsigned long* lp;
            if (above) {
                lp = (unsigned long*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                lp = (unsigned long*)Data_Block[handle].dims[ndim].errhi;
            } else {
                lp = (unsigned long*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)lp[i];
            break;
        }
        case TYPE_CHAR: {
            char* cp;
            if (above) {
                cp = Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                cp = Data_Block[handle].dims[ndim].errhi;
            } else {
                cp = Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case TYPE_UNSIGNED_CHAR: {
            unsigned char* cp;
            if (above) {
                cp = (unsigned char*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                cp = (unsigned char*)Data_Block[handle].dims[ndim].errhi;
            } else {
                cp = (unsigned char*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) fp[i] = (float)cp[i];
            break;
        }
        case TYPE_DCOMPLEX: {
            int j = 0;
            DCOMPLEX* cp;
            if (above) {
                cp = (DCOMPLEX*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                cp = (DCOMPLEX*)Data_Block[handle].dims[ndim].errhi;
            } else {
                cp = (DCOMPLEX*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
        case TYPE_COMPLEX: {
            int j = 0;
            COMPLEX* cp;
            if (above) {
                cp = (COMPLEX*)Data_Block[handle].dims[ndim].errhi;
            } else if (!Data_Block[handle].dims[ndim].errasymmetry) {
                cp = (COMPLEX*)Data_Block[handle].dims[ndim].errhi;
            } else {
                cp = (COMPLEX*)Data_Block[handle].dims[ndim].errlo;
            }
            for (i = 0; i < ndata; i++) {
                fp[j++] = (float)cp[i].real;
                fp[j++] = (float)cp[i].imaginary;
            }
            break;
        }
    }
    return;
}

//!  Returns coordinate error data cast to single precision
/** The copy buffer must be preallocated and sized for the data type.
\param   handle   The data object handle
\param   ndim  the position of the dimension in the data array - numbering is as data[0][1][2]
\param   fp A \b float pointer to a preallocated data buffer
\return  void
*/
void getIdamFloatDimError(int handle, int ndim, float* fp)
{
    int above = 1;
    getIdamFloatDimAsymmetricError(handle, ndim, above, fp);
}

//!  Returns a pointer to the DATA_SYSTEM Meta Data structure
/** A copy of the \b Data_System database table record
\param   handle   The data object handle
\return  DATA_SYSTEM pointer
*/
DATA_SYSTEM* getIdamDataSystem(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return (DATA_SYSTEM*)Data_Block[handle].data_system;
}
//!  Returns a pointer to the SYSTEM_CONFIG Meta Data structure
/** A copy of the \b system_config database table record
\param   handle   The data object handle
\return  SYSTEM_CONFIG pointer
*/
SYSTEM_CONFIG* getIdamSystemConfig(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return (SYSTEM_CONFIG*)Data_Block[handle].system_config;
}
//!  Returns a pointer to the DATA_SOURCE Meta Data structure
/** A copy of the \b data_source database table record - the location of data
\param   handle   The data object handle
\return  DATA_SOURCE pointer
*/
DATA_SOURCE* getIdamDataSource(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return (DATA_SOURCE*)Data_Block[handle].data_source;
}
//!  Returns a pointer to the SIGNAL Meta Data structure
/** A copy of the \b signal database table record
\param   handle   The data object handle
\return  SIGNAL pointer
*/
SIGNAL* getIdamSignal(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return (SIGNAL*)Data_Block[handle].signal_rec;
}
//!  Returns a pointer to the SIGNAL_DESC Meta Data structure
/** A copy of the \b signal_desc database table record - a description of the data signal/object
\param   handle   The data object handle
\return  SIGNAL_DESC pointer
*/
SIGNAL_DESC* getIdamSignalDesc(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    return (SIGNAL_DESC*)Data_Block[handle].signal_desc;
}

//!  Returns a pointer to the File Format string returned in the DATA_SOURCE metadata record
/** Dependent on the server property \b get_meta
\param   handle   The data object handle
\return  pointer to the data file format
*/
char* getIdamFileFormat(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return NULL;
    DATA_SOURCE* data_source = getIdamDataSource(handle);
    if (data_source == NULL) return NULL;
    return data_source->format;
}



//-----------------------------------------------------------------------------------------------------------
// Various Utilities

void initIdamDataBlock(DATA_BLOCK* str)
{
    initDataBlock(str);
    return;
}

void initIdamRequestBlock(REQUEST_BLOCK* str)
{
    initRequestBlock(str);
    return;
}

int idamDataCheckSum(void* data, int data_n, int type)
{
    int i, sum = 0;
    switch (type) {
        case TYPE_FLOAT: {
            float fsum = 0.0;
            float* dp = (float*)data;
            for (i = 0; i < data_n; i++) if (isfinite(dp[i])) fsum = fsum + dp[i];
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case TYPE_DOUBLE: {
            double fsum = 0.0;
            double* dp = (double*)data;
            for (i = 0; i < data_n; i++) if (isfinite(dp[i])) fsum = fsum + dp[i];
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case TYPE_COMPLEX: {
            float fsum = 0.0;
            COMPLEX* dp = (COMPLEX*)data;
            for (i = 0; i < data_n; i++)
                if (isfinite(dp[i].real) && isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }
        case TYPE_DCOMPLEX: {
            double fsum = 0.0;
            DCOMPLEX* dp = (DCOMPLEX*)data;
            for (i = 0; i < data_n; i++)
                if (isfinite(dp[i].real) && isfinite(dp[i].imaginary)) {
                    fsum = fsum + dp[i].real + dp[i].imaginary;
                }
            sum = (int)fsum;
            if (sum == 0) sum = (int)(1000000.0 * fsum);      // Rescale
            break;
        }

        case TYPE_CHAR: {
            char* dp = (char*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_SHORT: {
            short int* dp = (short int*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_INT: {
            int* dp = (int*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_LONG: {
            long* dp = (long*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_LONG64: {
            long long int* dp = (long long int*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_UNSIGNED_CHAR: {
            unsigned char* dp = (unsigned char*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_UNSIGNED_SHORT: {
            unsigned short int* dp = (unsigned short int*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_UNSIGNED_INT: {
            unsigned int* dp = (unsigned int*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
        case TYPE_UNSIGNED_LONG: {
            unsigned long* dp = (unsigned long*)data;
            for (i = 0; i < data_n; i++) sum = sum + (int)dp[i];
            break;
        }
#ifndef __APPLE__
        case TYPE_UNSIGNED_LONG64: {
            unsigned long long int* dp = (unsigned long long int*) data;
            for (i = 0; i < data_n; i++) sum = sum + (int) dp[i];
            break;
        }
#endif
        default:
            sum = 0;
    }
    return sum;
}

int getIdamDataCheckSum(int handle)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    if (Data_Block[handle].errcode != 0) return 0;

    return (idamDataCheckSum((void*)Data_Block[handle].data, Data_Block[handle].data_n,
                             Data_Block[handle].data_type));
}

int getIdamDimDataCheckSum(int handle, int ndim)
{
    if (handle < 0 || handle >= Data_Block_Count) return 0;
    if (Data_Block[handle].errcode != 0) return 0;
    if (ndim < 0 || ndim >= Data_Block[handle].rank) return 0;

    return (idamDataCheckSum((void*)Data_Block[handle].dims[ndim].dim, Data_Block[handle].dims[ndim].dim_n,
                             Data_Block[handle].dims[ndim].data_type));
}


//===========================================================================================================
// Access to (De)Serialiser

void getIdamClientSerialisedDataBlock(int handle, void** object, size_t* objectSize, char** key, size_t* keySize)
{

// Extract the serialised Data Block from Cache or serialise it if not cached (hash key in Data Block, empty if not cached)
// Use Case: extract data in object form for storage in external data object store, e.g. CEPH, HDF5
/*
TODO

1> Add cache key to DATA_BLOCK
2> Investigate creation of cache key when REQUEST_BLOCK is out of scope!
3> is keySize useful if the key is always a string!
*/

    char* buffer;
    size_t bufsize = 0;

    FILE* memfile = open_memstream(&buffer, &bufsize);

    XDR xdrs;
    xdrstdio_create(&xdrs, memfile, XDR_ENCODE);

    int token;

    USERDEFINEDTYPELIST* userdefinedtypelist = getIdamUserDefinedTypeList(handle);
    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);
    protocol2(&xdrs, PROTOCOL_DATA_BLOCK, XDR_SEND, &token, logmalloclist, userdefinedtypelist, (void*)getIdamDataBlock(handle));

    xdr_destroy(&xdrs);     // Destroy before the  file otherwise a segmentation error occurs
    fclose(memfile);

    // return the serialised data object and key

    *object = buffer;
    *objectSize = bufsize;
    *key = NULL;
    *keySize = 0;
}

//---------------------------------------------------------------
// Accessor Functions to General/Arbitrary Data Structures
//----------------------------------------------------------------

int setIdamDataTree(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;    // Return FALSE
    if (getIdamData(handle) == NULL) return 0;

    fullNTree = (NTREE*)getIdamData(handle); // Global pointer
    void* opaque_block = getIdamDataOpaqueBlock(handle);
    setUserDefinedTypeList(((GENERAL_BLOCK*)opaque_block)->userdefinedtypelist);
    setLogMallocList(((GENERAL_BLOCK*)opaque_block)->logmalloclist);
    setLastMallocIndexValue(&(((GENERAL_BLOCK*)opaque_block)->lastMallocIndex));
    return 1; // Return TRUE
}

// Return a specific data tree

NTREE* getIdamDataTree(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    return (NTREE*)getIdamData(handle);
}

// Return a user defined data structure definition

USERDEFINEDTYPE* getIdamUserDefinedType(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    void* opaque_block = getIdamDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->userdefinedtype;
}

USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    void* opaque_block = getIdamDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->userdefinedtypelist;
}

LOGMALLOCLIST* getIdamLogMallocList(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    void* opaque_block = getIdamDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK*)opaque_block)->logmalloclist;
}

NTREE* findIdamNTreeStructureDefinition(NTREE* node, const char* target)
{
    return findNTreeStructureDefinition(node, target);
}
