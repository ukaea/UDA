//! $LastChangedRevision: 115 $
//! $LastChangedDate: 2009-11-09 09:55:16 +0000 (Mon, 09 Nov 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamclientserver.h $

#ifndef IdamClientServerInclude
#define IdamClientServerInclude

// Client and Server Header
//
// Change History
//
// 08Jul2009    dgm Split into idamclientserverpublic.h and idamclientserverprivate.h
//---------------------------------------------------------------------------------------------

#include "idamclientserverpublic.h"
#include "idamclientserverprivate.h"

#include "idamgenstructpublic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int XDRstdioFlag;
extern LOGMALLOCLIST * logmalloclist;
extern unsigned int lastMallocIndex;
extern unsigned int * lastMallocIndexValue;

#ifdef __cplusplus
}
#endif

#endif
