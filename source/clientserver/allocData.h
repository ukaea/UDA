
#ifndef IDAM_ALLOCDATA_H
#define IDAM_ALLOCDATA_H

#include "idamStructs.h"

int allocArray(int data_type, int ndata, char **ap);
int allocData(DATA_BLOCK *data_block);
int allocDim(DATA_BLOCK * data_block);
int allocPutData(PUTDATA_BLOCK *putData);
void addIdamPutDataBlockList(PUTDATA_BLOCK *putDataBlock, PUTDATA_BLOCK_LIST *putDataBlockList);

#endif // IDAM_ALLOC_DATA_H
