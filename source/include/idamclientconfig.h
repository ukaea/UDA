
#ifndef IDAM_INCLUDE_IDAMCLIENTCONFIG_H
#define IDAM_INCLUDE_IDAMCLIENTCONFIG_H

// Client Side Header: Locally configure the client
//
// Change History:
//
// 17Nov2011 dgmuir   original version
//---------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------
#ifdef MASTSERVER

#  define IDAM_SERVER_PORT        56565
#  define IDAM_SERVER_HOST        "idam1"     // principal host
#  define IDAM_SERVER_PORT2       56565
#  define IDAM_SERVER_HOST2       "fuslwi"    // secondary host

#  define API_PARSE_STRING        "::"
#  define API_DEVICE              "MAST"
#  define API_ARCHIVE             "MAST"
#  define API_FILE_FORMAT         "IDA3"      // Legacy - not used

#  define IDAM_WORK_DIR           "/tmp"      // Temporary File Work Directory

// Mapping local directories to network directories

#  define SCRATCHDIR              "scratch"
#  define NETPREFIX               "net"

#endif

//----------------------------------------------------------------
#ifdef MAST64SERVER

#  define IDAM_SERVER_PORT        56565
#  define IDAM_SERVER_HOST        "shed3"     // principal host
#  define IDAM_SERVER_PORT2       56565
#  define IDAM_SERVER_HOST2       "idam1"     // secondary host

#  define API_PARSE_STRING        "::"
#  define API_DEVICE              "MAST64"
#  define API_ARCHIVE             "MAST"
#  define API_FILE_FORMAT         "NETCDF"    // Legacy - not used

#  define IDAM_WORK_DIR           "/tmp"      // Temporary File Work Directory

// Mapping local directories to network directories

#  define SCRATCHDIR              "scratch"
#  define NETPREFIX               "net"

#endif

//----------------------------------------------------------------
#ifdef JETSERVER

#  define IDAM_SERVER_PORT        56565
#  define IDAM_SERVER_HOST        "jac-2.jet.uk"  // principal host
#  define IDAM_SERVER_PORT2       56565
#  define IDAM_SERVER_HOST2       "jac-2.jet.uk"  // secondary host

#  define API_PARSE_STRING        "::"
#  define API_DEVICE              "JET"
#  define API_ARCHIVE             "PPF"
#  define API_FILE_FORMAT         "PPF"       // Legacy - not used

#  define IDAM_WORK_DIR           "/tmp"      // Temporary File Work Directory

#  define NOHOSTPREFIX
#endif

//----------------------------------------------------------------
#ifdef ITERSERVER

#define IDAM_SERVER_PORT        56564       //56565
#define IDAM_SERVER_HOST        "localhost" //"rca.fusion.org.uk"   // principal host
#define IDAM_SERVER_PORT2       0
#define IDAM_SERVER_HOST2       ""          // secondary host

#define API_PARSE_STRING    "::"
#define API_DEVICE      "ITER"      //"MAST"
#define API_ARCHIVE     "ITER"      //"MAST"
#define API_FILE_FORMAT     ""          // Legacy - not used

#define IDAM_WORK_DIR       "/tmp"          // Temporary File Work Directory

#define NOHOSTPREFIX
#endif

//----------------------------------------------------------------
#ifdef OTHERSERVER

#  define IDAM_SERVER_PORT        56565
#  define IDAM_SERVER_HOST        "mast.fusion.org.uk"    // principal host
#  define IDAM_SERVER_PORT2       0
#  define IDAM_SERVER_HOST2       ""                      // secondary host

#  define API_PARSE_STRING        "::"
#  define API_DEVICE              "MAST"
#  define API_ARCHIVE             "MAST"
#  define API_FILE_FORMAT         "IDA3"                      // Legacy - not used

#  define IDAM_WORK_DIR           "/tmp"                  // Temporary File Work Directory

#  define NOHOSTPREFIX
#endif

//----------------------------------------------------------------
#ifdef PROXYSERVER

#  define IDAM_SERVER_PORT        56565
#  define IDAM_SERVER_HOST        "fuslwn"    // principal host
#  define IDAM_SERVER_PORT2       56565
#  define IDAM_SERVER_HOST2       "fuslwn"    // secondary host

#  define API_PARSE_STRING        "::"
#  define API_DEVICE              "MAST"
#  define API_ARCHIVE             "MAST"
#  define API_FILE_FORMAT         "NETCDF"    // Legacy - not used

#  define IDAM_WORK_DIR           "/tmp"      // Temporary File Work Directory

//#define NOHOSTPREFIX
#endif

//---------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // IDAM_INCLUDE_IDAMCLIENTCONFIG_H
