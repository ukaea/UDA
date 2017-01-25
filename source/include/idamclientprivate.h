#ifndef IdamClientPrivateInclude
#define IdamClientPrivateInclude

#ifdef __cplusplus
extern "C" {
#endif

#include "idamclientconfig.h"
#include "idamclientserverprivate.h"
#include <clientserver/idamStructs.h>

#define XDEBUG                  0       // Socket Streams Only
#define CLIENT_DEBUG_LEVEL      0       // Granularity of Client Side Debug Output 

#define MAX_SOCKET_DELAY        20      // maximum random delay (secs) between attempts to connect a socket
#define MAX_SOCKET_ATTEMPTS     10      // maximum number of attempts to connect a socket per host

#ifndef DEFAULT_STATUS
#define DEFAULT_STATUS          1       // Default Signal and Data_Source Status value (Also in idamclientserverprivate.h)
#endif

//-------------------------------------------------------
// Error Models (This order must be preserved to align it with the getModelId function)

#define ERROR_MODEL_SEED            12345

//#define ERROR_MODEL_UNKNOWN               0    // Defined in idamclientserver.h

// commented out dgm 07Nov2013 - moved to idamclientserverpublic.h

//#define ERROR_MODEL_DEFAULT               1
//#define ERROR_MODEL_DEFAULT_ASYMMETRIC    2
//#define ERROR_MODEL_GAUSSIAN              3
//#define ERROR_MODEL_RESEED                4
//#define ERROR_MODEL_GAUSSIAN_SHIFT        5
//#define ERROR_MODEL_POISSON               6
//#define ERROR_MODEL_UNDEFINED             7

//--------------------------------------------------------------
// Varaibles with Internal Scope Only

extern SOCKETLIST client_socketlist;        // List of Data Server Sockets
extern int clientSocket;                    // The tcp/ip fd handle
extern int clientVersion;                   // Client Library Version
extern char clientUsername[STRING_LENGTH];  // Only obtain userid once

// XDR data Streams

extern XDR * clientInput;           // XDR Input Stream handle
extern XDR * clientOutput;          // XDR Output Stream handle
//static XDR *clientFreeput;        // XDR Free Stream handle    - Not used as all heap managed by code

extern unsigned int XDRstdioFlag;

// File Cache

#ifdef FILECACHE
extern REQUEST_BLOCK * request_block_ptr;   // Pointer to the User REQUEST_BLOCK
#endif

// Data

extern CLIENT_BLOCK client_block;   // Client Properties
extern SERVER_BLOCK server_block;   // Server Status Block
//extern DATA_BLOCK * Data_Block;     // Data Block
//extern int Data_Block_Count ;       // The Number of Data Block Handles Issued

extern int reopen_logs;         // Flags whether or Not Logs need to be Re-Opened

extern int user_timeout;        // user specified Server Lifetime

// Control Flags

extern int get_dimdble;         // (Server Side) Return Dimensional Data in Double Precision
extern int get_timedble;        // (Server Side) Server Side cast of time dimension to double precision if in compresed format
extern int get_scalar;          // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
extern int get_bytes;           // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
// calibration factor applied
extern int get_meta;            // (Server Side) return All Meta Data
extern int get_asis;            // (Server Side) Apply no XML based corrections to Data or Dimensions
extern int get_uncal;           // (Server Side) Apply no XML based Calibrations to Data
extern int get_notoff;          // (Server Side) Apply no XML based Timing Corrections to Data
extern int get_nodimdata;

extern int get_datadble;        // (Client Side) Return Data in Double Precision
extern int get_bad;             // (Client Side) return data with BAD Status value
extern int get_synthetic;       // (Client Side) Return Synthetic Data if available instead of Original data

extern time_t tv_server_start;  // Server Startup Time
extern time_t tv_server_end;    // Current Time & Server Age

extern int env_host;            // Flags to Read Environment variable at startup time
extern int env_port;

#ifdef PERFORMANCETEST
static PERFORMANCE clientPerformance;
#endif

//--------------------------------------------------------------
// Client Side API Error Codes

#define    NO_SOCKET_CONNECTION             -10000
#define PROBLEM_OPENING_LOGS                -11000
#define FILE_FORMAT_NOT_SUPPORTED           -12000
#define    ERROR_ALLOCATING_DATA_BOCK_HEAP  -13000
#define    SERVER_BLOCK_ERROR               -14000
#define    SERVER_SIDE_ERROR                -14001
#define    DATA_BLOCK_RECEIPT_ERROR         -15000
#define    ERROR_CONDITION_UNKNOWN          -16000

#define NO_EXP_NUMBER_SPECIFIED             -18005

#ifdef __cplusplus
}
#endif

#endif
