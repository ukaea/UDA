#ifndef IdamClientServerPrivateInclude
#define IdamClientServerPrivateInclude

// Client and Server Header
//
// Notes:
//
// On JET there is no Generic Signal lookup so comment out the GENERIC_ENABLE macro
//
// to examine symbols use: nm libx.so | grep <symbol>
// to check exec's libraries use: ldd <exec>
//
//---------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <clientserver/idamDefines.h>
#include <clientserver/idamStructs.h>

#ifdef __APPLE__
#  include <rpc/types.h>
#endif

#include <rpc/xdr.h>

//--------------------------------------------------------
// Character used to separate directory file path elements

#ifndef _WIN32
#define PATH_SEPARATOR  "/"
#else
#define PATH_SEPARATOR  "\\"
#endif

//--------------------------------------------------------
// Compiler Options for Both Server and Client

#ifndef NOTGENERICENABLED
#define GENERIC_ENABLE          // Generic Signal Name Lookup via SQL Database is Enabled
#endif

//--------------------------------------------------------
// Error Management

#define SYSTEMERRORTYPE     1
#define CODEERRORTYPE       2
#define PLUGINERRORTYPE     3

//--------------------------------------------------------
// QA Status (Also in idamclientprivate.h)

#ifndef DEFAULT_STATUS
#define DEFAULT_STATUS      1   // Default Signal and Data_Source Status value
#endif

// Cache permissions

#define PLUGINNOTOKTOCACHE  0   // Plugin state management incompatible with client side cacheing 
#define PLUGINOKTOCACHE     1   // Data are OK for the Client to Cache

#define PLUGINCACHEDEFAULT  PLUGINOKTOCACHE // The cache permission to use as the default

#define PLUGINNOCACHETYPE   0
#define PLUGINMEMCACHETYPE  1
#define PLUGINFILECACHETYPE 2

#define MAXCACHEDATABLOCKSIZE   100*1024

//---------------------------------------------------------------------------------------------------
// Variables with Static Scope (None should be extern on the client side: Seen only by functions within the same file)

#ifdef SERVERBUILD

extern ENVIRONMENT environment;         // Environment Variables
extern IDAMERRORSTACK idamerrorstack;   // Error Stack

extern int protocolVersion;             // Client or Server Version number for Protocol Configuration

extern PERFORMANCE performance;

#else

#  ifdef ROIDAMBUILD
extern ENVIRONMENT environment;         // Environment Variables
extern IDAMERRORSTACK idamerrorstack;   // Error Stack
#  else
#    ifdef ARGSTACK
static FILE * argstack;                 // Log all arguments passed
#    endif
extern ENVIRONMENT environment;         // Environment Variables
#  endif

//static PERFORMANCE performance;

extern int protocolVersion; // Client or Server Version number for Protocol Configuration

#endif

//---------------------------------------------------------------------------------------------------
// Client Server XDR data Streams (DON'T CHANGE ORDER or Legacy client won't work!)

enum REQUEST {
    REQUEST_SHUTDOWN = 1,
    REQUEST_READ_GENERIC,       // Generic Signal via the IDAM Database
    REQUEST_READ_IDA,           // an IDA File
    REQUEST_READ_MDS,           // an MDSPlus Server
    REQUEST_READ_IDAM,          // a Remote IDAM server
    REQUEST_READ_FORMAT,        // Server to Choose Plugin for Requested Format
    REQUEST_READ_CDF,           // netCDF File
    REQUEST_READ_HDF5,          // HDF5 FIle
    REQUEST_READ_XML,           // XML Document defining a Signal
    REQUEST_READ_UFILE,     // TRANSP UFile
    REQUEST_READ_FILE,          // Read a File: A Container of Bytes!
    REQUEST_READ_SQL,           // Read from an SQL Data Source
    REQUEST_READ_PPF,           // JET PPF
    REQUEST_READ_JPF,           // JET JPF
    REQUEST_READ_NEW_PLUGIN,
    REQUEST_READ_NOTHING,       // Immediate Return without Error: Client Server Timing Tests
    REQUEST_READ_BLOCKED,       // Disable Server Option for External Users (Not a client side option)
    REQUEST_READ_HDATA,         // Hierarchical Data Structures
    REQUEST_READ_SERVERSIDE,        // Server Side Functions
    REQUEST_READ_UNKNOWN,       // Plugin Not Known
    REQUEST_READ_WEB,           // a Remote or Local web server
    REQUEST_READ_BIN,           // Binary file
    REQUEST_READ_HELP,          // Help file
    REQUEST_READ_DEVICE     // Request to an External Device's data server
};

//-------------------------------------------------------
// XDR Stream Directions

#define XDR_SEND        0
#define XDR_RECEIVE     1
#define XDR_FREE_HEAP   2

//-------------------------------------------------------
// SERVER Types

#define TYPE_UNKNOWN_SERVER 0
#define TYPE_IDAM_SERVER    1
#define TYPE_MDSPLUS_SERVER 2

#ifdef __cplusplus
}
#endif

#endif
