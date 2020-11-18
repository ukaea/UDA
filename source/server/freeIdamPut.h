#ifndef UDA_SERVER_FREEIDAMPUT_H
#define UDA_SERVER_FREEIDAMPUT_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API void freeServerPutDataBlockList(PUTDATA_BLOCK_LIST *putDataBlockList);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_FREEIDAMPUT_H
