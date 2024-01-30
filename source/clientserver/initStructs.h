#ifndef UDA_CLIENTSERVER_INITSTRUCTS_H
#define UDA_CLIENTSERVER_INITSTRUCTS_H

#include "udaStructs.h"

void initNameValueList(NAMEVALUELIST* nameValueList);
void initRequestData(REQUEST_DATA* str);
void initRequestBlock(REQUEST_BLOCK* str);
void initClientBlock(CLIENT_BLOCK* str, int version, const char* clientname);
void initServerBlock(SERVER_BLOCK* str, int version);
void initDataBlock(DATA_BLOCK* str);
void initDataBlockList(DATA_BLOCK_LIST* str);
void initDimBlock(DIMS* str);
void initDataSystem(DATA_SYSTEM* str);
void initSystemConfig(SYSTEM_CONFIG* str);
void initDataSource(DATA_SOURCE* str);
void initSignal(SIGNAL* str);
void initSignalDesc(SIGNAL_DESC* str);
void initIdamPutDataBlock(PUTDATA_BLOCK* str);
void initPutDataBlockList(PUTDATA_BLOCK_LIST* putDataBlockList);

#endif // UDA_CLIENTSERVER_INITSTRUCTS_H
