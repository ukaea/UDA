#ifndef IDAM_IDAMFILESERVERPROCESSING_H
#define IDAM_IDAMFILESERVERPROCESSING_H

#include <clientserver/udaStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int serverProcessing(CLIENT_BLOCK client_block, DATA_BLOCK *data_block);

#ifdef __cplusplus
}
#endif

#endif // IDAM_IDAMFILESERVERPROCESSING_H

