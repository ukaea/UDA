#ifndef UDA_CLIENTSERVER_FREEDATABLOCK_H
#define UDA_CLIENTSERVER_FREEDATABLOCK_H

#include "udaStructs.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct LogMallocList LOGMALLOCLIST;
typedef struct UserDefinedTypeList USERDEFINEDTYPELIST;

void freeIdamDataBlock(DATA_BLOCK* data_block);
void freeDataBlock(DATA_BLOCK* data_block);
void freeReducedDataBlock(DATA_BLOCK* data_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_FREEDATABLOCK_H