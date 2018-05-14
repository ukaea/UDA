#ifndef IDAM_SERVER_IDAMSERVERGETDATA2_H
#define IDAM_SERVER_IDAMSERVERGETDATA2_H

#ifdef NOTGENERICENABLED
typedef int PGconn;
#else
#include <libpq-fe.h>
#endif

#include <clientserver/parseXML.h>

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

#endif // IDAM_SERVER_IDAMSERVERGETDATA2_H

