//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamserver.h $

#ifndef IdamServer
#define IdamServer

// Server Side Header
//
// Change History
//
// v0.01 15Jun2006 dgm  readCDF Function Prototype Added
// v1.01 10Jul2006 dgm  user_timeout declared as EXTERN
// v1.02 21Mar2007 dgm  Externals renamed with server_ prefix
//              External serverSocket added
//              External serverVersion and serverUsername added
//          External XDR streams serverInput, serverOutput added
// 10Apr2007    dgm FATCLIENT Compiler Option Added
// 12Apr2007    dgm GENERIC_ENABLE Compiler Option Added
// 17Apr2007    dgm IDA_ENABLE Compiler Option Added
// 16May2007    dgm Local MDS+ Server Host defined
// 09Jul2007    dgm IO FDs: dbgout,errout moved to clientserver.h
// 10Jul2007    dgm FATCLIENT commented out & GENERIC_ENABLE moved to idamclientserver.h
// 23Oct2007    dgm ERRORSTACK Components added
// 15Feb2008    dgm added tpass to sqlGeneric prototype
//          added sqlLatestPass prototype
// 02Apr2008    dgm C++ test added for inclusion of extern "C"
// 08Jul2009    dgm All globals changed from extern to static: to protect namespace
// 17Feb2010    dgm Added FORMAT_LEGACY and FORMAT_MODERN data file formats default values
// 20May2010    dgm TESTCODE additions: environment data
// 16Nov2010    dgm Prototypes added: sqlDataSourceAlias, sqlSignalDesc, sqlAltData
// 25Mar2011    dgm MAXMAPDEPTH added to block infinite recursion in sqlMapData
// 02Jun2011    dgm IDA_ENABLE Compiler Option Removed
// 05Sep2011    dgm Added Name-Value pair function prototypes
// 19Sep2011    dgm Accessors to local globals for use by plugins
// 17Nov2011    dgm Added idamserverconfig.h file for local configuration
// 12Mar2012    dgm Removed TESTCODE compiler option - legacy code deleted.
// 18Sep2012    dgm Added sqlMapPrivateData prototype.
// 21Jan2016	dgm	Added idamServerMetaDataPlugin prototype
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
