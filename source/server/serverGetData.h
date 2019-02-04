#ifndef UDA_SERVER_SERVERGETDATA_H
#define UDA_SERVER_SERVERGETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/sqllib.h>
#include <clientserver/udaStructs.h>
#include <clientserver/socketStructs.h>
#include <plugins/pluginStructs.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

int idamserverGetData(PGconn* DBConnect, int* depth, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                      DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                      ACTIONS* actions_desc, ACTIONS* actions_sig, const PLUGINLIST* pluginlist,
                      LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list);

int idamserverSwapSignalError(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry);

int idamserverSwapSignalDim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2);

int idamserverSwapSignalDimError(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry);

int idamserverReadData(PGconn* DBConnect, REQUEST_BLOCK request_block, CLIENT_BLOCK client_block,
                       DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                       const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERGETDATA_H

