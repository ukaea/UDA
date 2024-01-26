#ifndef UDA_CLIENT_H
#define UDA_CLIENT_H

// TODO: remove this and the XDR globals
#include <rpc/rpc.h>

#include "export.h"
#include "genStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamClient idamClientFat
#  define idamFreeAll idamFreeAllFat
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

LIBRARY_API void udaFree(int handle);

LIBRARY_API void udaFreeAll();

/**
 * Get the version of the client c-library.
 */
LIBRARY_API const char* udaGetBuildVersion();

/**
 * Get the date that the client c-library was built.
 */
LIBRARY_API const char* udaGetBuildDate();

LIBRARY_API const char* getIdamServerHost();

LIBRARY_API int getIdamServerPort();

LIBRARY_API int getIdamServerSocket();

LIBRARY_API const char* getIdamClientDOI();

LIBRARY_API const char* getIdamServerDOI();

LIBRARY_API const char* getIdamClientOSName();

LIBRARY_API const char* getIdamServerOSName();

LIBRARY_API int getIdamClientVersion();

LIBRARY_API int getIdamServerVersion();

LIBRARY_API int getIdamServerErrorCode();

LIBRARY_API const char* getIdamServerErrorMsg();

LIBRARY_API int getIdamServerErrorStackSize();

LIBRARY_API int getIdamServerErrorStackRecordType(int record);

LIBRARY_API int getIdamServerErrorStackRecordCode(int record);

LIBRARY_API const char* getIdamServerErrorStackRecordLocation(int record);

LIBRARY_API const char* getIdamServerErrorStackRecordMsg(int record);

LIBRARY_API void closeAllConnections();

LIBRARY_API int udaNumErrors(void);
LIBRARY_API const char* udaGetErrorMessage(int err_num);
LIBRARY_API int udaGetErrorCode(int err_num);
LIBRARY_API const char* udaGetErrorLocation(int err_num);

typedef struct LogMallocList LOGMALLOCLIST;
typedef struct UserDefinedTypeList USERDEFINEDTYPELIST;

LIBRARY_API LOGMALLOCLIST* getIdamLogMallocList(int handle);
LIBRARY_API USERDEFINEDTYPELIST* getIdamUserDefinedTypeList(int handle);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_H
