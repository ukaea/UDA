#ifndef IdamGenStructPrivateInclude
#define IdamGenStructPrivateInclude

// Private definitions of Data Structures used by the General Structure Passing system
//
// Change History
//
// 08Jan2010    D.G.Muir    Original Version
// 21Sep2011    dgm     Change prototypes from static to extern when macro PLUGINTEST is defined
// 10Oct2012    dgm     Corrected bug: removed FullNTree and assignment of fullNTree
// 20Nov2013    dgm     PLUGINTEST selections commented out
//              DEV_08NOV2012 selections commented out
// 28Nov2013    dgm     Moved PACKAGE_XDRFILE, PACKAGE_STRUCTDATA from protocolXML to here
// 09Oct2014    dgm     Added PACKAGE_XDROBJECT
//---------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------------
// Globals

#include "idamgenstructpublic.h"
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
