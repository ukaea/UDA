#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#define UDA_SERVER_SERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>

int idamserverSubsetData(DATA_BLOCK *data_block, ACTION action, LOGMALLOCLIST* logmalloclist);

int idamserverParseServerSide(REQUEST_BLOCK *request_block, ACTIONS *actions_serverside);

#endif // UDA_SERVER_SERVERSUBSETDATA_H
