// Change History:
//
// 29Jan2007    dgmuir      Environment variable changes
// 30Aug2007    dgmuir      Number of Directories and files increased from 256 to 2048
// 25Feb2008    dgmuir      MAXLENGTH and MAXARGS added
// 17Aug2009    dgmuir      Incresed MAXRECLENGTH from 256 to 1024
//              Added TARGET_ZSHOT
// 18Aug2009    dgmuir      MAXNAME, MAXPATH increased from 56 to 1024, MAXFILENAME from 56 to 256
// 14Oct2009    dgmuir      Added definitions for signal_alias name properties
// 19Oct2009    dgmuir      Added signal_alias_type argument to LoadSignalDesc and data_signals functions
//              getSignalDescSQL, updateSignalDesc added
// 11Oct2009    dgmuir      Added valid range of source status values
// 11Mar2011    dgmuir      Changed MAXDIRALIAS from 3 to 56
//-----------------------------------------------------------------------------

#ifndef IdamSchedulerInclude
#define IdamSchedulerInclude

#ifndef IDAMDLM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <libpq-fe.h>

#include <ida3.h>

extern FILE* dbglog;            // Debug Log
extern FILE* errlog;            // Error Log
extern FILE* stdlog;            // Standard Log Output

extern int webout;              // Enables Standard Web Reporting of Errors

#define LOGIT           0       // Switch Standard Logging ON/OFF - 1:0
#define DEBUG           0       // Switch Debug ON/OFF - 1:0
// Will cause the Web Server routines to fail if ON

#define NOUPDATE        0       // Don't Update the IDAM Database (Test Mode)

#define WORKDIR         tmp     // Location of the Work Directories

#define NEWM01FILE      19026   // Last Shot number using the mast####.m01 file

#ifndef IdamClientServerPublicInclude
#define MAXDATE         11
#define MAXSERVER       256
#define MAXFORMAT       56
#define MAXFILENAME     256
#define MAXRECLENGTH    1024
#define NUMSIGNALS      10240
#endif

#define MAXDEVICE       256
#define MAXTIME         9
#define MAXNAME         1024
#define MAXDESC         1024
#define MAXNAMELISTREC  1024

#define MAXPATH         1024
#define MAXKEY          56
#define MAXTYPE         56
#define MAXSQL          21*1024

#define MAXDIRALIAS     56  // Length of Source Alias Strings
#define MAXMAPDEPTH     12  // Recursive depth limit

#define MAXMETA         10*1024


#define NUMPULSES       2048

#define MAXNAMARRAY     1024
#define MAXVARNAME      56
#define MAXVARVALUE     56

#define MAXARGS         56
#define MAXLENGTH       10*1024


#define MAXFILES 2048
#define MAXDIRS  2048
#define MAXITEM  12
#define MINYEAR  "1980"     // for Date Problems

#define DEFAULTMODELREASON      1
#define DEFAULTPROCESSREASON    2
#define DEFAULTREPASSREASON     3

#define DEFAULTSIGNALSTATUS     1
#define DEFAULTSOURCESTATUS     1
#define DEFAULTREASONCODE       -1
#define DEFAULTIMPACTCODE       0
#define DEFAULTSTATUSDESCID     0

#define MAXSOURCESTATUS         3
#define MINSOURCESTATUS         -1

// Default Environment Variables

#define IDAM_SQLHOST        "fuslwn"
#define IDAM_SQLPORT        "56566"
#define IDAM_SQLDBNAME      "idam"
#define IDAM_SQLUSER        "mast_db"

#define IDAM_WEBURL     "fuslwn"

// Scheduler Data File Types

#define TARGET_IDA      0
#define TARGET_TRANSP   1
#define TARGET_EFIT     2
#define TARGET_IMAGES   3
#define TARGET_ZSHOT    4

// Scheduler File Formats

#define FORMAT_IDA      0
#define FORMAT_CDF      1
#define FORMAT_HDF      2
#define FORMAT_MDS      3
#define FORMAT_UNKNOWN  -1

// File Properties

#define FILE_REGULAR    0
#define FILE_OFFLINE    1
#define FILE_LINK       2

// Database signal_alias name properties (bit toggle values for signal_alias_type)

#define PREFIXCASE      3
#define PREFIXNOCASE    1
#define NOPREFIXCASE    2
#define NOPREFIXNOCASE  0
#define ISPREFIX        1
#define ISCASE          2

#endif
#endif
