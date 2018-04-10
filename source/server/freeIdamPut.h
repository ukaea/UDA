#ifndef IDAM_SERVER_FREEIDAMPUT_H
#define IDAM_SERVER_FREEIDAMPUT_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

void freeIdamServerPutDataBlock(PUTDATA_BLOCK *str);
void freeIdamServerPutDataBlockList(PUTDATA_BLOCK_LIST *putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_FREEIDAMPUT_H
