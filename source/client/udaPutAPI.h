#ifndef IDAM_CLIENT_IDAMPUTAPI_H
#define IDAM_CLIENT_IDAMPUTAPI_H

#include <clientserver/udaStructs.h>

#ifdef FATCLIENT
#  define idamPutListAPI idamPutListAPIFat
#  define idamPutAPI idamPutAPIFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

int idamPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList);
int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

void freeIdamClientPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENT_IDAMPUTAPI_H
