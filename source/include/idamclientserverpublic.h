#ifndef IdamClientServerPublicInclude
#define IdamClientServerPublicInclude

// Public definitions of Data Structures
//
// Change History
//
// 08Dec2008 dgm derived from original idamclientserver.h
// 18Aug2009 dgm Increased size definitions for MAXNAME, MAXFILENAME, MAXFORMAT from 56 to 1024,
//          MAXSERVER, MAXPATH from 256 to 1024, MAXDATE from 56 to 12 (Same as DATE_LENGTH)
// 20Aug2009 dgm CLIENT_BLOCK element debug_level renamed to get_nodimdata: was redundant as is now used to
//         minimise network traffic by not sending dimensional data if it has already been received.
//         SERVER_DEBUG_LEVEL deleted
// 09Nov2009 dgm VLEN type added
// 23Apr2010 dgm privateFlags added to CLIENT_BLOCK
// 11May2010 dgm source and api_delim added to REQUEST_BLOCK (client & server version 6)
// 02Nov2010 dgm new bit flags added for passing via properties in CLIENT_BLOCK
//         verbose structure element renamed clientFlags and type changed from int to unsigned int
//         debug structure element renamed altRank
// 11Nov2010 dgm <stdarg> added for variable length argument lists to IDAM API
// 21Feb2011 dgm signal_alias_type, signal_map_id added to SIGNAL_DESC.
// 18Apr2011 dgm DATASUBSET structure created
//         subset and datasubset added to REQUEST_BLOCK structure
//         MAXRANK2 created - similar role to MAXRANK in idamgenstructpublic.h !!!
// 05Sep2011 dgm Added Name-Value pair data structures: NAMEVALUELIST, NAMEVALUE
//         Modified REQUEST_BLOCK to include NAMEVALUELIST
// 21Sep2011 dgm Added string 'function' to REQUEST_BLOCK
// 28Jun2013 dgm Added SECURITY_BLOCK data structure (enabled via the SECURITYENABLED compiler macro)
//         Added SECURITY_BLOCK element to both the CLIENT_BLOCK and SERVER_BLOCK data structures
// 18Nov2013 dgm PUTDATA functionality included as standard rather than with a compiler option
// 28Apr2014 dgm Added totalDataBlockSize and cachePermission to DATA_BLOCK
// 09Oct2014 dgm Added PRIVATEFLAG_XDROBJECT
// 12Feb2015 dgm Added DATA_OBJECT structure definition
// 30Nov2016 dgm Added client flag properties
//---------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------
// IDAM Data Types

#ifndef IdamDataTypesInclude
#  define IdamDataTypesInclude
#  include "idamtypes.h"
#endif

//--------------------------------------------------------

// Size Definitions

#define WORKDIR        tmp    // Location of the Work Directories

//#define MAXDEVICE    256

#ifdef ROLLBACK
#  define MAXDATE         56
#  define MAXNAME         56
#  define MAXFILENAME     56
#  define MAXSERVER       256
#  define MAXPATH         256
#  define MAXFORMAT       56
#else
#  define MAXDATE         12    // Ensure same as DATE_LENGTH
#  define MAXNAME         1024    // Same as STRING_LENGTH
#  define MAXFILENAME     1024    // Same as STRING_LENGTH
#  define MAXSERVER       1024    // Same as STRING_LENGTH
#  define MAXPATH         1024    // Same as STRING_LENGTH
#  define MAXFORMAT       1024    // Same as STRING_LENGTH
#endif

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

#define MD5_SIZE        16

#define STRING_LENGTH        1024
#define MAX_STRING_LENGTH    1024    // Ensure same as STRING_LENGTH
#define DATE_LENGTH        12

#define MAXLOOP             10000
#define MAXBLOCK            1000    // msecs

#define DB_READ_BLOCK_SIZE  32*1024 //16384    
#define DB_WRITE_BLOCK_SIZE 32*1024 //16384  

#define GROWPUTDATABLOCKLIST    10

//--------------------------------------------------------
// Client Specified Properties

#define TIMEOUT            600    // Server Shutdown after this time (Secs)
#define COMPRESS_DIM         1    // Compress regular dimensional data

//--------------------------------------------------------
// Private Flags: Properties passed to IDAM clients called by servers (32 bits)

#define PRIVATEFLAG_FULLRESET -1    // ffff - 0010    Reset flags except EXTERNAL
#define PRIVATEFLAG_XDRFILE    1    // 0001        Use an intermediate file containing the XDR data rather than a data stream
#define PRIVATEFLAG_EXTERNAL   2    // 0010        The originating server is an External Facing server
#define PRIVATEFLAG_CACHE      4    // 0100        Cache all data

#define PRIVATEFLAG_XDROBJECT   8   //  1000        Use an intermediate XDR data object rather than a data stream

extern unsigned int privateFlags;

//--------------------------------------------------------
// Client Flags: Client specified local properties (32 bits)

#define CLIENTFLAG_FULLRESET -1    // ffff        Reset flags
#define CLIENTFLAG_ALTDATA    1    // 0001
#define CLIENTFLAG_XDRFILE    2    // 0010        Use an intermediate file with the XDR data rather than a data stream
#define CLIENTFLAG_CACHE      4    // 0100        Access data from the local cache and write new data to cache
#define CLIENTFLAG_CLOSEDOWN  8    // 1000        Immediate Closedown

#define CLIENTFLAG_XDROBJECT    16  // 10000        Use a XDR object in memory

#define CLIENTFLAG_REUSELASTHANDLE     32      // 100000       Reuse the last issued handle value (for this thread) - assume application has freed heap
#define CLIENTFLAG_FREEREUSELASTHANDLE 64      // 1000000      Free the heap associated with the last issued handle and reuse the handle value

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

// SQL Structures

struct LISTSIGNALS {
    char * signal_alias;
    char * generic_name;
    char * type;
    char * description;
    char * source_alias;
    char * signal_class;
    char * signal_owner;
};
typedef struct LISTSIGNALS LISTSIGNALS ;

#ifdef __cplusplus
}
#endif

#endif
