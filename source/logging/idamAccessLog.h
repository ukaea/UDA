#ifndef IDAM_IDAMACCESSLOG_H
#define IDAM_IDAMACCESSLOG_H

#include <include/idamplugin.h>

#define HOSTNAMELENGTH    20
#define DATELENGTH    27

int idamSizeOf(int data_type);

unsigned int countDataBlockSize(DATA_BLOCK* data_block, CLIENT_BLOCK* client_block);

void idamAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request,
                   SERVER_BLOCK server_block, DATA_BLOCK data_block);

#endif // IDAM_IDAMACCESSLOG_H

