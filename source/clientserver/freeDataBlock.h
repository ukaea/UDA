//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/clientserver/freeDataBlock.c $

// Free Heap Memory
//
// Change History:
//
// 13Dec2006	dgm	errparams now fixed length array rather than heap
// 29Sep2008	dgm	free Opaque Data structures added
// 04Nov2008	dgm	reset opaque_count on free
// 11Feb2010	dgm	Free Generalised Structure Data
// 01Mar2010	dgm	created freeReducedDataBlock to manage heap for fatclients accessing generalised data structures
// 28Apr2010	dgm	OPAQUE_TYPE_XDRFILE added
// 18Nov2013	dgm	PUTDATA functionality included as standard rather than with a compiler option
//-----------------------------------------------------------------------------

#ifndef IDAM_FREEDATABLOCK_H
#define IDAM_FREEDATABLOCK_H

#include "idamStructs.h"

// Forward declarations
struct LOGMALLOCLIST;
struct USERDEFINEDTYPELIST;

void freeIdamDataBlock(DATA_BLOCK *data_block);
void freeMallocLogList(struct LOGMALLOCLIST *str);
void freeUserDefinedTypeList(struct USERDEFINEDTYPELIST *userdefinedtypelist);
void freeDataBlock(DATA_BLOCK *data_block);
void freeReducedDataBlock(DATA_BLOCK *data_block);

#endif // IDAM_FREEDATABLOCK_H

