#include "freeIdamPut.h"

#include <cstdlib>

#include <clientserver/initStructs.h>

void freeServerPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    initIdamPutDataBlockList(putDataBlockList);
}

