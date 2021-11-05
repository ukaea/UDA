#ifndef UDA_CLIENT_UDAPUTAPI_H
#define UDA_CLIENT_UDAPUTAPI_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef FATCLIENT
#  define udaPutListAPI udaPutListAPIFat
#  define udaPutAPI udaPutAPIFat
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaPutListAPI(const char* put_instruction, PUTDATA_BLOCK_LIST* putdata_block_list_in);
LIBRARY_API int udaPutAPI(const char* put_instruction, PUTDATA_BLOCK* putdata_block_in);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENT_UDAPUTAPI_H
