#ifndef UDA_CLIENT_H
#define UDA_CLIENT_H

// TODO: remove this and the XDR globals
#include <rpc/rpc.h>

#include "export.h"
#include "genStructs.h"
#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define udaClient udaClientFat
#  define udaFreeAll udaFreeAllFat
#  define udaClientFlags udaClientFlagsFat
#endif

//--------------------------------------------------------------
// Client Side API Error Codes

#define NO_SOCKET_CONNECTION (-10000)
#define PROBLEM_OPENING_LOGS (-11000)
#define FILE_FORMAT_NOT_SUPPORTED (-12000)
#define ERROR_ALLOCATING_DATA_BOCK_HEAP (-13000)
#define SERVER_BLOCK_ERROR (-14000)
#define SERVER_SIDE_ERROR (-14001)
#define DATA_BLOCK_RECEIPT_ERROR (-15000)
#define ERROR_CONDITION_UNKNOWN (-16000)

#define NO_EXP_NUMBER_SPECIFIED (-18005)

#define MIN_STATUS (-1)          // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD (-17000) // Error Code if Status is Bad

typedef struct ClientFlags {
    int get_dimdble;  // (Server Side) Return Dimensional Data in Double Precision
    int get_timedble; // (Server Side) Server Side cast of time dimension to double precision if in compresed format
    int get_scalar;   // (Server Side) Reduce Rank from 1 to 0 (Scalar) if time data are all zero
    int get_bytes;    // (Server Side) Return IDA Data in native byte or integer array without IDA signal's
                      // calibration factor applied
    int get_meta;     // (Server Side) return All Meta Data
    int get_asis;     // (Server Side) Apply no XML based corrections to Data or Dimensions
    int get_uncal;    // (Server Side) Apply no XML based Calibrations to Data
    int get_notoff;   // (Server Side) Apply no XML based Timing Corrections to Data
    int get_nodimdata;

    int get_datadble;  // (Client Side) Return Data in Double Precision
    int get_bad;       // (Client Side) return data with BAD Status value
    int get_synthetic; // (Client Side) Return Synthetic Data if available instead of Original data

    uint32_t flags;

    int user_timeout;
    int alt_rank;
} CLIENT_FLAGS;

LIBRARY_API void udaFree(int handle);

LIBRARY_API void udaFreeAll();

LIBRARY_API CLIENT_FLAGS* udaClientFlags();
LIBRARY_API unsigned int* udaPrivateFlags();

/**
 * Get the version of the client c-library.
 */
LIBRARY_API const char* udaGetBuildVersion();

/**
 * Get the date that the client c-library was built.
 */
LIBRARY_API const char* udaGetBuildDate();

LIBRARY_API const char* udaGetServerHost(); 

LIBRARY_API int udaGetServerPort(); 

LIBRARY_API int udaGetServerSocket(); 

LIBRARY_API const char* udaGetClientDOI(); 

LIBRARY_API const char* udaGetServerDOI(); 

LIBRARY_API const char* udaGetClientOSName(); 

LIBRARY_API const char* udaGetServerOSName(); 

LIBRARY_API int udaGetClientVersion(); 

LIBRARY_API int udaGetServerVersion(); 

LIBRARY_API int udaGetServerErrorCode(); 

LIBRARY_API const char* udaGetServerErrorMsg(); 

LIBRARY_API int udaGetServerErrorStackSize(); 

LIBRARY_API int udaGetServerErrorStackRecordType(int record); 

LIBRARY_API int udaGetServerErrorStackRecordCode(int record); 

LIBRARY_API const char* udaGetServerErrorStackRecordLocation(int record); 

LIBRARY_API const char* udaGetServerErrorStackRecordMsg(int record); 

LIBRARY_API void udaCloseAllConnections();

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H
