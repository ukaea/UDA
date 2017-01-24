//! $LastChangedRevision: 226 $
//! $LastChangedDate: 2011-02-15 10:28:26 +0000 (Tue, 15 Feb 2011) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamproxy.h $

#ifndef IdamProxy
#define IdamProxy

// IDAM Proxy Server Include file
//
// Change History:
//
// 26Sep2007    D.G.Muir    Original Version
// 02Sep2009    D.G.Muir    MAXNAME and MAXPATH increased to 1024 consistent with IDAM client and server
//----------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/param.h>
#include <setjmp.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <strings.h>
#include <crypt.h>
#include <signal.h>
#include <netdb.h>
#include <stddef.h>
#include <math.h>
#include <float.h>

#define IDAM_SERVER_PORT        56565
#define IDAM_SERVER_HOST        "fuslwn"    // "idam1"
#define TIMEOUT                 600     // Server Shutdown after this time (Secs)
#define SOCKET_BLOCK_SIZE       128*1024
#define MAXNAME                 1024
#define MAXMODE                 8
#define MAXPATH                 1024
#define STRING_LENGTH           1024
#define PROXY_BUFFER_SIZE       128*1024

//--------------------------------------------------------------

//struct ENVIRONMENT {
//    int server_port;
//    char server_host[MAXNAME];
//    int timeout;            // Server Lifetime
//    int block_size;         // Socket Block Size
//    char logpath[MAXPATH];
//    char logmode[MAXMODE];
//    int debug;
//    int verbose;
//};
//typedef struct ENVIRONMENT ENVIRONMENT;

//--------------------------------------------------------------
// Externals

//extern int server_block_time;       // Accumulated Blocking Time from Last Data I/O

//extern ENVIRONMENT environment;

//--------------------------------------------------------------
// Proxy Server Error Codes

#define ERROR_ENVIRONMENT       100
#define ERROR_LOGFILES          200
#define ERROR_SOCKET_CONNECTION 300
#define PROXY_READ_ERROR        400
#define PROXY_WRITE_ERROR       500
#define PROXY_TIMEOUT           600

#endif
