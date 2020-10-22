#ifndef UDA_CLIENTSERVER_IDAMDEFINES_H
#define UDA_CLIENTSERVER_IDAMDEFINES_H

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------
// Size Definitions

#define WORKDIR         tmp    // Location of the Work Directories

#define MAXDATE         12    // Ensure same as DATE_LENGTH
#define MAXNAME         1024    // Same as STRING_LENGTH
#define MAXFILENAME     1024    // Same as STRING_LENGTH
#define MAXSERVER       1024    // Same as STRING_LENGTH
#define MAXPATH         1024    // Same as STRING_LENGTH
#define MAXMODE         8
#define MAXFORMAT       1024    // Same as STRING_LENGTH

#define MAXDESC         1024    // Same as STRING_LENGTH
#define MAXNAMELISTREC  1024

#define MAXKEY          56
#define MAXSQL          21*1024
#define MAXRECLENGTH    256

#define MAXMETA        10*1024

#define MAXERRPARAMS    8

#define MAXRANK2        10    // Number of subsetting dimensions

#define NUMSIGNALS      2048
#define NUMPULSES       2048

#define MAXNAMARRAY     1024
#define MAXVARNAME      56
#define MAXVARVALUE     56

#define MAXFILES        2048
#define MAXDIRS         2048
#define MAXITEM         12

//#define MINYEAR  "1980"        // for Date Problems

#define MD5_SIZE                16

#define STRING_LENGTH           1024
#define MAX_STRING_LENGTH       1024    // Ensure same as STRING_LENGTH
#define DATE_LENGTH             12

#define MAXLOOP                 10000
#define MAXBLOCK                1000    // msecs

#define DB_READ_BLOCK_SIZE      32*1024 //16384
#define DB_WRITE_BLOCK_SIZE     32*1024 //16384

#define GROWPUTDATABLOCKLIST    10

//--------------------------------------------------------
// Client Specified Properties

#define TIMEOUT            600    // Server Shutdown after this time (Secs)
#define COMPRESS_DIM         1    // Compress regular dimensional data

//--------------------------------------------------------
// Private Flags: Properties passed to IDAM clients called by servers (32 bits)

#define PRIVATEFLAG_FULLRESET   (unsigned int)-1    // ffff - 0010    Reset flags except EXTERNAL
#define PRIVATEFLAG_XDRFILE     (unsigned int)1     // 0001        Use an intermediate file containing the XDR data rather than a data stream
#define PRIVATEFLAG_EXTERNAL    (unsigned int)2     // 0010        The originating server is an External Facing server
#define PRIVATEFLAG_CACHE       (unsigned int)4     // 0100        Cache all data

#define PRIVATEFLAG_XDROBJECT    8   // 1000        Use an intermediate XDR data object rather than a data stream

extern unsigned int privateFlags;

//--------------------------------------------------------
// Client Flags: Client specified local properties (32 bits)

#define CLIENTFLAG_FULLRESET -1             // ffff     Reset flags
#define CLIENTFLAG_ALTDATA    1u            // 0000 0001
#define CLIENTFLAG_XDRFILE    2u            // 0000 0010    Use an intermediate file with the XDR data rather than a data stream
#define CLIENTFLAG_CACHE      4u            // 0000 0100    Access data from the local cache and write new data to cache
#define CLIENTFLAG_CLOSEDOWN  8u            // 0000 1000    Immediate Closedown
#define CLIENTFLAG_XDROBJECT  16u           // 0001 0000    Use a XDR object in memory
#define CLIENTFLAG_REUSELASTHANDLE     32u  // 0010 0000    Reuse the last issued handle value (for this thread) - assume application has freed heap
#define CLIENTFLAG_FREEREUSELASTHANDLE 64u  // 0100 0000    Free the heap associated with the last issued handle and reuse the handle value
#define CLIENTFLAG_FILECACHE 128u           // 1000 0000    Access data from and save data to local cache files

extern unsigned int clientFlags;

//--------------------------------------------------------
// Error Models - added dgm 07Nov2013 from idamclientprivate.h

#define ERROR_MODEL_UNKNOWN            0    // Defined in idamclientserver.h
#define ERROR_MODEL_DEFAULT            1
#define ERROR_MODEL_DEFAULT_ASYMMETRIC 2
#define ERROR_MODEL_GAUSSIAN           3
#define ERROR_MODEL_RESEED             4
#define ERROR_MODEL_GAUSSIAN_SHIFT     5
#define ERROR_MODEL_POISSON            6
#define ERROR_MODEL_UNDEFINED          7

//--------------------------------------------------------
// Character used to separate directory file path elements

#ifndef _WIN32
#define PATH_SEPARATOR  "/"
#else
#define PATH_SEPARATOR  "\\"
#endif

//--------------------------------------------------------
// QA Status

#define DEFAULT_STATUS 1   // Default Signal and Data_Source Status value

//--------------------------------------------------------
// Not defined functions for Windows MSVC

#if defined(_WIN32)

#if !defined(MINGW)
LIBRARY_API int gettimeofday(struct timeval* tp, struct timezone* tzp);
#endif

#endif


#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_IDAMDEFINES_H
