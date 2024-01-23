#ifndef UDA_CLIENT_UDAPUTAPI_H
#define UDA_CLIENT_UDAPUTAPI_H

#include "export.h"
#include "udaStructs.h"

#ifdef FATCLIENT
#  define udaPutListAPI udaPutListAPIFat
#  define udaPutAPI udaPutAPIFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaPutListAPI(const char* putInstruction, PUTDATA_BLOCK_LIST* inPutDataBlockList);
LIBRARY_API int udaPutAPI(const char* putInstruction, PUTDATA_BLOCK* inPutData);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAPUTAPI_H
