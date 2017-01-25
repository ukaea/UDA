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
// Change History
//
// v0.01 15Jun2006 dgmuir   netCDF Request Added
// v0.02 10Jul2006 dgmuir   Server Lifetime TIMEOUT changed from 10 secs to 10 mins
// v0.03 05Oct2006 dgmuir   Changes to Structures: Status data type - Byte to Int
//       06Sep2006 dgm      Error data added to Signal_Desc structure
//       26Oct2006 dgm      Global var clientname added to improve performance: only 1 system call
//       22Jan2007 dgm      Client_Block structure extended to include get_timedble and get_scalar
//   24Jan2007 dgm      get_dimdble anded and get_dble changed to get_datadble
//   28Feb2007 dgm      Add a copy of the client_block structure to the data_block to preserve
//              the properties state values when the data were acquired.
//       09Mar2007 dgm      REQUEST_READ_HDF changed to REQUEST_READ_HDF5 and REQUEST_READ_XML added
//              REQUEST_BLOCK altered: signal string lengthened from STRING_LENGTH to MAXMETA
//              STRING_LENGTH increased from 256 to 1024
//   21Mar2007 dgm      Socket management function prototype changed
//              Socketlist external declaration moved to client header
//              Server specific Socket management function prototypes moved to Server.h
//              DB_Socket commented out
//              initClientBlock and initServerBlock prototypes changed to add version and name args
//              externals version and clientname moved to client and server .h files
//              XDR externals Input, Output moved to client and server .h files
//  22Mar2007       performance external moved to server and client .h files
//  18Apr2007 dgm       For SOLARIS systems, optional header files included
//  16May2007 dgm       Additional MDS+ Error Code Added for TYPE
//  22May2007 dgm       REQUEST_READ_SQL added for Continuously Measured Data
//  31May2007 dgm       Hierarchical Data Structures added to Data_Block
//  09Jul2007 dgm       Globals debugon and verbose moved from idamclient.h
//      09Jul2007 dgm       IO FDs: dbgout,errout moved from idamclient.h and idamserver.h
//      10Jul2007 dgm       GENERIC_ENABLE moved from idamclient.h and idamserver.h
//      23Oct2007 dgm       ERRORSTACK option added with utility prototypes and modification of SERVER_BLOCK
//      08Nov2007 dgm       ENVIRONMENT structure moved from idamclient.h
//  11Dec2007 dgm       MAXFILES and MAXDIRS increased from 256 to 2048
//  07Jan2008 dgm       ENVIRONMENT structure modified to include: data_path_id
//  10Jan2008 dgm       reverseString prototype added
//  02Apr2008 dgm       C++ test added for inclusion of extern "C"
//              REQUEST_READ_SERVERSIDE added for Server Side Functions
//      04Nov2008 dgm       CLIENT_VERSION commented out as not used
//              external int protocolVersion added
//              opaque_count added to DATA_BLOCK
// 08Dec2008 dgm        Removed Public Structure Definitions to idamclientserverpublic.h
// 19Dec2008 dgm        min(x,y) commented out
// 10Jun2009 dgm        COMPLEX and DCOMPLEX structured types added
// 07Jul2009 dgm    Moved error model ids to idamclient.h
//          Added more string utility prototypes
//          sys/time.h changed to time.h
// 30SEP2009 dgm    Added private_path_target and private_path_substitute to ENVIRONMENT structure
// 07OCT2009 dgm    Added external_user to ENVIRONMENT structure (same effect as compiler option EXTERNAL_USER)
// 13Nov2009 dgm    Added string.h
// 25Nov2009 dgm    Added OPAQUE_TYPE_STRUCTURES and PROTOCOL_STRUCTURES
// 22Jan2010 dgm    Added protocolVersionTypeTest prototype
// 01Mar2010 dgm    Added freeReducedDataBlock prototype
// 05Mar2010 dgm    Added second host and port to environment structure
// 26Apr2010 dgm    Added sendXDRFile, receiveXDRFile prototypes
// 27Apr2010 dgm    added OPAQUE_TYPE_XDRFILE macro value
// 11Nov2010 dgm    added clientFlags and altRank to ENVIRONMENT data structure
// 14Jan2011 dgm    added IDAM_PLUGIN_INTERFACE data structure
// 31Jan2011    dgm Windows sockets implementation
// 11Feb2011    dgm Add server_proxy and server_this to environment variable to redirect client requests
// 21Sep2011    dgm Added initNameValueList prototype
// 12Mar2012    dgm Removed TESTCODE compiler option - legacy code deleted.
// 06Feb2013 dgm    closeIdamError scope changed to extern
// 18Nov2013 dgm    PUTDATA functionality included as standard rather than with a compiler option
// 28Apr2014 dgm    Added cache permissions PLUGINOKTOCACHE, PLUGINNOTOKTOCACHE
// 09Oct2014 dgm    Added OPAQUE_TYPE_XDROBJECT
// 13Jul2015 dgm    Added protocolXML2Put
//---------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <clientserver/idamDefines.h>
#include <clientserver/idamStructs.h>

#include <netinet/tcp.h>

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

#define ERRORSTACK          // Enables Error Stack Components

//#define HIERARCHICAL_DATA     // Enables ITM-Like Data Structure Passing Protocol

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

//--------------------------------------------------------
// Socket Management

typedef struct Sockets {
    int type;               // Type Code (e.g.,1=>IDAM;2=>MDS+);
    char host[MAXSERVER];   // Server's Host Name or IP Address
    int port;
    int status;             // Open (1) or Closed (0)
    int fh;                 // Socket to Server File Handle
    int user_timeout;       // Server's timeout value (self-destruct)
    time_t tv_server_start; // Server Startup Clock Time
    XDR* Input;             // Client Only XDR input Stream;
    XDR* Output;            // Client Only XDR Output Stream;
} SOCKETS;

typedef struct SocketList {
    int nsocks;             // Number of Sockets
    SOCKETS* sockets;      // Array of Socket Management Data
} SOCKETLIST;

//---------------------------------------------------------------------------------------------------
// System Environment Variables

typedef struct Environment {
    int server_port;                            // Principal IDAM server port
    int server_port2;                           // Backup IDAM server port
    int sql_port;
    int server_reconnect;                       // If the client changes to a different IDAM server then open a new socket
    int server_change_socket;                   // Connect to a Running Server
    int server_socket;                          // Clients must keep track of the sockets they open
    int data_path_id;                           // Identifies the algorithm that defines the default path to the standard data source.
    int external_user;                          // Flags this service as accessible by external users: Disable some access formats for security.
    unsigned int clientFlags;                   // Use legacy Name substitution
    int altRank;                                // Use specific set of legacy name substitutes
    char logdir[MAXPATH];
    char logmode[2];
    int loglevel;
    char server_host[MAXNAME];                   // Principal IDAM server host
    char server_host2[MAXNAME];                  // Backup IDAM server host
    char server_proxy[MAXNAME];                  // host:port - Running as a Proxy IDAM server: Prefix 'IDAM::host:port/' to redirect request
    char server_this[MAXNAME];                   // host:port - The current server. Used to trap potential infinite redirects
    char sql_host[MAXNAME];
    char sql_dbname[MAXNAME];
    char sql_user[MAXNAME];
    char api_delim[MAXNAME];                     // Default API Signal and Source Delimiter Sub-String
    char api_device[STRING_LENGTH];              // API Default Device name
    char api_archive[STRING_LENGTH];             // API Default Archive name
    char api_format[STRING_LENGTH];              // API Default Client File Format
    char private_path_target[STRING_LENGTH];     // Target this path to private files
    char private_path_substitute[STRING_LENGTH]; // and substitute with this path (so the server can locate them!)
    char initialised; // Environment already initialised.
    char _padding[1];
} ENVIRONMENT;

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

//-------------------------------------------------------
// Client Server Conversaton Protocols

#define PROTOCOL_REGULAR_START      0       // Identifies Regular Data Protocol Group
#define PROTOCOL_REQUEST_BLOCK      1
#define PROTOCOL_DATA_BLOCK         2
#define PROTOCOL_NEXT_PROTOCOL      3
#define PROTOCOL_DATA_SYSTEM        4
#define PROTOCOL_SYSTEM_CONFIG      5
#define PROTOCOL_DATA_SOURCE        6
#define PROTOCOL_SIGNAL             7
#define PROTOCOL_SIGNAL_DESC        8
#define PROTOCOL_SPARE1             9
#define PROTOCOL_CLIENT_BLOCK       10
#define PROTOCOL_SERVER_BLOCK       11
#define PROTOCOL_SPARE2             12
#define PROTOCOL_CLOSEDOWN          13
#define PROTOCOL_SLEEP              14
#define PROTOCOL_WAKE_UP            15
#define PROTOCOL_PUTDATA_BLOCK_LIST 16
#define PROTOCOL_SECURITY_BLOCK     17
#define PROTOCOL_OBJECT             18
#define PROTOCOL_SERIALISE_OBJECT   19
#define PROTOCOL_SERIALISE_FILE     20
#define PROTOCOL_DATAOBJECT         21
#define PROTOCOL_DATAOBJECT_FILE    22
#define PROTOCOL_REGULAR_STOP       99

#define PROTOCOL_OPAQUE_START       100         // Identifies Legacy Hierarchical Data Protocol Group
#define PROTOCOL_STRUCTURES         101
#define PROTOCOL_META               102
#define PROTOCOL_EFIT               103
#define PROTOCOL_PFCOILS            104
#define PROTOCOL_PFPASSIVE          105
#define PROTOCOL_PFSUPPLIES         106
#define PROTOCOL_FLUXLOOP           107
#define PROTOCOL_MAGPROBE           108
#define PROTOCOL_PFCIRCUIT          109
#define PROTOCOL_PLASMACURRENT      110
#define PROTOCOL_DIAMAGNETIC        111
#define PROTOCOL_TOROIDALFIELD      112
#define PROTOCOL_LIMITER            113
#define PROTOCOL_OPAQUE_STOP        200

#ifdef __cplusplus
}
#endif

#endif
