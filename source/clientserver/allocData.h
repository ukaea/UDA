#ifndef UDA_CLIENTSERVER_ALLOCDATA_H
#define UDA_CLIENTSERVER_ALLOCDATA_H

#include "udaStructs.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int allocArray(int data_type, size_t ndata, char** ap);
int allocData(DATA_BLOCK* data_block);
int allocDim(DATA_BLOCK* data_block);
int allocPutData(PUTDATA_BLOCK* putData);
void addIdamPutDataBlockList(PUTDATA_BLOCK* putDataBlock, PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_ALLOCDATA_H
