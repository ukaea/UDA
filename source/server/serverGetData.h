#ifndef UDA_SERVER_SERVERGETDATA_H
#define UDA_SERVER_SERVERGETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <clientserver/socketStructs.h>
#include <structures/genStructs.h>
#include <plugins/pluginStructs.h>

#include "sqllib.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaGetData(REQUEST_BLOCK request_block, CLIENT_BLOCK client_block, DATA_BLOCK* data_block, DATA_SOURCE* data_source,
               SIGNAL* signal_rec, SIGNAL_DESC* signal_desc, const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
               USERDEFINEDTYPELIST* userdefinedtypelist);

LIBRARY_API int idamserverSwapSignalError(DATA_BLOCK* data_block, DATA_BLOCK* data_block2, int asymmetry);

LIBRARY_API int idamserverSwapSignalDim(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2);

LIBRARY_API int idamserverSwapSignalDimError(DIMCOMPOSITE dimcomposite, DATA_BLOCK* data_block, DATA_BLOCK* data_block2,
                                 int asymmetry);

LIBRARY_API int idamserverReadData(PGconn* DBConnect, REQUEST_BLOCK* request_block, CLIENT_BLOCK client_block,
                       DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                       const PLUGINLIST* pluginlist, LOGMALLOCLIST* logmalloclist,
                       USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERGETDATA_H

