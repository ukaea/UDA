//! $LastChangedRevision: 115 $
//! $LastChangedDate: 2009-11-09 09:55:16 +0000 (Mon, 09 Nov 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamclient.h $


#ifndef IdamClientInclude
#define IdamClientInclude

// Client Side Header
//
// Change History:
//
// 08Jul2009    dgm Split into idamclientpublic.h and idamclientprivate.h
//--------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include "idamclientpublic.h"
#include "idamclientprivate.h"
#include "idamgenstructpublic.h"

extern int initEnvironment;
extern int altRank;

extern LOGMALLOCLIST * logmalloclist;
extern int malloc_source;

extern unsigned int lastMallocIndex;
extern unsigned int * lastMallocIndexValue;

#ifdef __cplusplus
}
#endif

#endif
