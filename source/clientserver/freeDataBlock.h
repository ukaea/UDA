#ifndef UDA_CLIENTSERVER_FREEDATABLOCK_H
#define UDA_CLIENTSERVER_FREEDATABLOCK_H

#include <structures/genStructs.h>
#include "udaStructs.h"

// Forward declarations

#ifdef __cplusplus
extern "C" {
#endif

void freeDataBlock(DATA_BLOCK* data_block);
void freeReducedDataBlock(DATA_BLOCK* data_block);
void freeIdamClientPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_FREEDATABLOCK_H

