#ifndef UDA_SERVER_FREEIDAMPUT_H
#define UDA_SERVER_FREEIDAMPUT_H

#include <clientserver/udaStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void freeIdamServerPutDataBlock(PUTDATA_BLOCK *str);
LIBRARY_API void freeIdamServerPutDataBlockList(PUTDATA_BLOCK_LIST *putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_FREEIDAMPUT_H
