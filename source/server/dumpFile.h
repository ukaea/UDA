#ifndef IDAM_SERVER_DUMPFILE_H
#define IDAM_SERVER_DUMPFILE_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int dumpFile(REQUEST_BLOCK request_block, DATA_BLOCK *data_block);

#ifdef __cplusplus
}
#endif

#endif // IDAM_SERVER_DUMPFILE_H
