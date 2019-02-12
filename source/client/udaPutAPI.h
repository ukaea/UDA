#ifndef UDA_CLIENT_UDAPUTAPI_H
#define UDA_CLIENT_UDAPUTAPI_H

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

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAPUTAPI_H
