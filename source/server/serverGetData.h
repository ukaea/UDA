#ifndef UDA_SERVER_SERVERGETDATA_H
#define UDA_SERVER_SERVERGETDATA_H

#include <clientserver/parseXML.h>
#include <clientserver/udaStructs.h>
#include <clientserver/socketStructs.h>
#include <structures/genStructs.h>
#include <plugins/pluginStructs.h>
#include <clientserver/export.h>

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API int udaGetData(int* depth, REQUEST_DATA* request_data, CLIENT_BLOCK client_block,
                           DATA_BLOCK* data_block, DATA_SOURCE* data_source, SIGNAL* signal_rec, SIGNAL_DESC* signal_desc,
                           ACTIONS* actions_desc, ACTIONS* actions_sig, const PLUGINLIST* pluginlist,
                           LOGMALLOCLIST* logmalloclist, USERDEFINEDTYPELIST* userdefinedtypelist, SOCKETLIST* socket_list,
                           int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_SERVER_SERVERGETDATA_H

