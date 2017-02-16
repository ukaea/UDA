// Free Heap Memory
//
//-----------------------------------------------------------------------------

#ifndef IDAM_CLIENTSERVER_FREEDATABLOCK_H
#define IDAM_CLIENTSERVER_FREEDATABLOCK_H

#include "udaStructs.h"

// Forward declarations
struct LOGMALLOCLIST;
struct USERDEFINEDTYPELIST;

void freeIdamDataBlock(DATA_BLOCK *data_block);
void freeDataBlock(DATA_BLOCK *data_block);
void freeReducedDataBlock(DATA_BLOCK *data_block);

#endif // IDAM_CLIENTSERVER_FREEDATABLOCK_H

