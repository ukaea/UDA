#ifndef UDA_CLIENT_UDAPUTAPI_H
#define UDA_CLIENT_UDAPUTAPI_H

#include "export.h"

typedef struct PutDataBlockList PUTDATA_BLOCK_LIST;
typedef struct PutDataBlock PUTDATA_BLOCK;

#ifdef FATCLIENT
#  define idamPutListAPI idamPutListAPIFat
#  define idamPutAPI idamPutAPIFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int idamPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList);
LIBRARY_API int idamPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAPUTAPI_H
