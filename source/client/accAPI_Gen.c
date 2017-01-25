//---------------------------------------------------------------
// Accessor Functions to General/Arbitrary Data Structures
//----------------------------------------------------------------

#ifdef GENERALSTRUCTS

#include <accessors.h>
#include <structures/struct.h>
#include <include/idamclientserver.h>
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
