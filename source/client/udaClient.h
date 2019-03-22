#ifndef UDA_CLIENT_UDACLIENT_H
#define UDA_CLIENT_UDACLIENT_H

// TODO: remove this and the XDR globals
#include <rpc/rpc.h>

#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define idamClient idamClientFat
#  define idamFreeAll idamFreeAllFat
#endif

//--------------------------------------------------------------
// Client Side API Error Codes

#define NO_SOCKET_CONNECTION            (-10000)
#define PROBLEM_OPENING_LOGS            (-11000)
#define FILE_FORMAT_NOT_SUPPORTED       (-12000)
#define ERROR_ALLOCATING_DATA_BOCK_HEAP (-13000)
#define SERVER_BLOCK_ERROR              (-14000)
#define SERVER_SIDE_ERROR               (-14001)
#define DATA_BLOCK_RECEIPT_ERROR        (-15000)
#define ERROR_CONDITION_UNKNOWN         (-16000)

#define NO_EXP_NUMBER_SPECIFIED         (-18005)

#define MIN_STATUS          (-1)        // Deny Access to Data if this Status Value
#define DATA_STATUS_BAD     (-17000)    // Error Code if Status is Bad

extern int altRank;

extern time_t tv_server_start;  // Server Startup Time
extern time_t tv_server_end;    // Current Time & Server Age

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

// XDR data Streams

extern XDR* clientInput;           // XDR Input Stream handle
extern XDR* clientOutput;          // XDR Output Stream handle

int idamClient(REQUEST_BLOCK* request_block);

void updateClientBlock(CLIENT_BLOCK* str);

void idamFree(int handle);

void idamFreeAll();

const char* getIdamServerHost();

int getIdamServerPort();

int getIdamServerSocket();

const char* getIdamClientDOI();

const char* getIdamServerDOI();

const char* getIdamClientOSName();

const char* getIdamServerOSName();

int getIdamClientVersion();

int getIdamServerVersion();

int getIdamServerErrorCode();

const char* getIdamServerErrorMsg();

int getIdamServerErrorStackSize();

int getIdamServerErrorStackRecordType(int record);

int getIdamServerErrorStackRecordCode(int record);

const char* getIdamServerErrorStackRecordLocation(int record);

const char* getIdamServerErrorStackRecordMsg(int record);

void setUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist);

void setLogMallocList(LOGMALLOCLIST* logmalloclist_in);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDACLIENT_H