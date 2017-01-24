//! $LastChangedRevision: 107 $
//! $LastChangedDate: 2009-10-07 15:18:57 +0100 (Wed, 07 Oct 2009) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/client/accAPI_Gen.c $

//---------------------------------------------------------------
// Accessor Functions to General/Arbitrary Data Structures
//
// Change History
//
// 28Jan2010    dgm Created
// 25Oct2012    dgm Added findIdamNTreeStructureDefinition
// 06Nov2012    dgm Added global lastMallocIndex and lastMallocIndexValue to pass and update the
//                  next search position for findMalloc calls
//----------------------------------------------------------------

#ifdef GENERALSTRUCTS

#include <accessors.h>
#include <structures/struct.h>
#include "accAPI_Gen.h"
#include "accAPI_C.h"

#ifdef FATCLIENT
#  include "idamserver.h"
#endif

// Set the Data Tree Global Pointer to a specific data tree

int setIdamDataTree(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;    // Return FALSE
    if (getIdamData(handle) == NULL) return 0;

    fullNTree = (NTREE *) getIdamData(handle); // Global pointer
    void * opaque_block = getIdamDataOpaqueBlock(handle);
    userdefinedtypelist = ((GENERAL_BLOCK *) opaque_block)->userdefinedtypelist;
    logmalloclist = ((GENERAL_BLOCK *) opaque_block)->logmalloclist;
    lastMallocIndexValue = &(((GENERAL_BLOCK *) opaque_block)->lastMallocIndex);
    lastMallocIndex = *lastMallocIndexValue;
    return 1; // Return TRUE
}

// Return a specific data tree

NTREE * getIdamDataTree(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    return (NTREE *) getIdamData(handle);
}

// **** name typo - elliminate ASAP
NTREE * getIdamTreeData(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    return (NTREE *) getIdamData(handle);
}

// Return a user defined data structure definition

USERDEFINEDTYPE * getIdamUserDefinedType(int handle)
{
    if (getIdamDataOpaqueType(handle) != OPAQUE_TYPE_STRUCTURES) return 0;
    void * opaque_block = getIdamDataOpaqueBlock(handle);
    return ((GENERAL_BLOCK *)opaque_block)->userdefinedtype;
}

NTREE * findIdamNTreeStructureDefinition(NTREE * node, const char * target)
{
    return findNTreeStructureDefinition(node, target);
}

#endif
