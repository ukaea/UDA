#ifndef UDA_CLIENT_UDAPUTAPI_H
#define UDA_CLIENT_UDAPUTAPI_H

#include <clientserver/udaStructs.h>

#ifdef FATCLIENT
#  define idamPutListAPI idamPutListAPIFat
#  define idamPutAPI idamPutAPIFat
#endif

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
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
