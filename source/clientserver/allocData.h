#ifndef UDA_CLIENTSERVER_ALLOCDATA_H
#define UDA_CLIENTSERVER_ALLOCDATA_H

#include "udaStructs.h"
#include "export.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int allocArray(int data_type, size_t ndata, char** ap);
LIBRARY_API int allocData(DATA_BLOCK* data_block);
LIBRARY_API int allocDim(DATA_BLOCK* data_block);
LIBRARY_API int allocPutData(PUTDATA_BLOCK* putData);
LIBRARY_API void addIdamPutDataBlockList(PUTDATA_BLOCK* putDataBlock, PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_ALLOCDATA_H
