#ifndef UDA_SERVER_DUMPFILE_H
#define UDA_SERVER_DUMPFILE_H

#include <clientserver/udaStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int dumpFile(REQUEST_BLOCK request_block, DATA_BLOCK *data_block);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_DUMPFILE_H
