#ifndef UDA_SERVER_SERVERSUBSETDATA_H
#define UDA_SERVER_SERVERSUBSETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <structures/genStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int serverSubsetData(DATA_BLOCK *data_block, ACTION action, LOGMALLOCLIST* logmalloclist);
LIBRARY_API int serverParseServerSide(REQUEST_BLOCK *request_block, ACTIONS *actions_serverside);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERSUBSETDATA_H
