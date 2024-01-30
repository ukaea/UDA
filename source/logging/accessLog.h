#ifndef UDA_LOGGING_ACCESSLOG_H
#define UDA_LOGGING_ACCESSLOG_H

#include "clientserver/udaStructs.h"

#define HOSTNAMELENGTH 20
#define DATELENGTH 27

#ifdef __cplusplus
extern "C" {
#endif

unsigned int countDataBlockListSize(const DATA_BLOCK_LIST* data_block_list, CLIENT_BLOCK* client_block);
unsigned int countDataBlockSize(const DATA_BLOCK* data_block, CLIENT_BLOCK* client_block);

void udaAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request_block,
                              SERVER_BLOCK server_block, unsigned int total_datablock_size);

#ifdef __cplusplus
}
#endif

#endif // UDA_LOGGING_ACCESSLOG_H