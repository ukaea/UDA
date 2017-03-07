#ifndef IDAM_LOGGING_IDAMACCESSLOG_H
#define IDAM_LOGGING_IDAMACCESSLOG_H

#include <plugins/udaPlugin.h>

#define HOSTNAMELENGTH    20
#define DATELENGTH    27

unsigned int countDataBlockSize(DATA_BLOCK* data_block, CLIENT_BLOCK* client_block);

void idamAccessLog(int init, CLIENT_BLOCK client_block, REQUEST_BLOCK request, SERVER_BLOCK server_block, const PLUGINLIST* pluginlist);

#endif // IDAM_LOGGING_IDAMACCESSLOG_H

