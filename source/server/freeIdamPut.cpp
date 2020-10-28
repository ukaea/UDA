#include "freeIdamPut.h"

#include <cstdlib>

#include <clientserver/initStructs.h>

void freeIdamServerPutDataBlock(PUTDATA_BLOCK* str)
{
    //str->opaque_block  = NULL;
    //str->data          = NULL;		// Client app is responsible for freeing these heap variables
    //str->blockName     = NULL;		// as IDAM does not copy the data (to be reviewed!)

    //if(str->count > 0 && str->data  != NULL) free((void *)str->data);
    //if(str->rank  > 1 && str->shape != NULL) free((void *)str->shape);
    //if(str->blockNameLength  > 1 && str->blockName != NULL) free((void *)str->blockName);
}

void freeServerPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList)
{
    if (putDataBlockList->putDataBlock != nullptr && putDataBlockList->blockListSize > 0) {
        free(putDataBlockList->putDataBlock);
    }
    initIdamPutDataBlockList(putDataBlockList);
}

