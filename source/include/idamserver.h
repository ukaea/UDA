#ifndef IdamServer
#define IdamServer

// Server Side Header
//----------------------------------------------------------------
// The Server utilises two Log Files. All data accesses and errors
// are reported to the Standard Log File.
// Debug Output is sent to the Debug Log.
//
// Both Log files are opened within the Main Server program.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <include/idamclientserver.h>
#include <include/idamgenstruct.h>

#define ACCESSLOG           // Enable Data Access Logging

#define MAXOPENFILEDESC 50  // Maximum number of Open File Descriptors

#define MAXMAPDEPTH     10  // Maximum number of chained signal name mappings (Recursive depth)
#define MAXREQDEPTH     4   // Maximum number of Device Name to Server Protocol and Host substitutions

#define XDEBUG          0   // Socket Streams

//--------------------------------------------------------------
// Static Global variables

extern SOCKETLIST server_socketlist;    // List of Data Server Sockets

extern unsigned int totalDataBlockSize;
extern int serverVersion;
extern int altRank;
extern unsigned int lastMallocIndex;
extern unsigned int* lastMallocIndexValue;

extern IDAMERRORSTACK idamerrorstack;

extern XDR* serverInput;
extern XDR* serverOutput;
extern int server_tot_block_time;
extern int server_timeout;

extern LOGMALLOCLIST* logmalloclist;
extern unsigned int XDRstdioFlag;

extern struct USERDEFINEDTYPELIST parseduserdefinedtypelist;

IDAMERRORSTACK* getIdamServerPluginErrorStack();
USERDEFINEDTYPELIST* getIdamServerUserDefinedTypeList();
extern void copyIdamServerEnvironment(ENVIRONMENT* environ);
void putIdamServerEnvironment(ENVIRONMENT environ);
LOGMALLOCLIST* getIdamServerLogMallocList();
USERDEFINEDTYPELIST* getIdamServerParsedUserDefinedTypeList();

#ifdef __cplusplus
}
#endif

#endif
