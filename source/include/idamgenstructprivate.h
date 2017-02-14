#ifndef IdamGenStructPrivateInclude
#define IdamGenStructPrivateInclude

// Private definitions of Data Structures used by the General Structure Passing system

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------------
// Globals

#include <structures/genStructs.h>

extern LOGSTRUCTLIST logstructlist; // List of all Passed Data Structures
extern NTREE * fullNTree;
extern NTREELIST NTreeList;

extern int enable_malloc_log;

#define MALLOCSOURCENONE    0
#define MALLOCSOURCESOAP    1
#define MALLOCSOURCEDOM     2
#define MALLOCSOURCENETCDF  3

extern int malloc_source;

#define PACKAGE_XDRFILE     1
#define PACKAGE_STRUCTDATA  2
#define PACKAGE_XDROBJECT   3

#ifdef __cplusplus
}
#endif

#endif
