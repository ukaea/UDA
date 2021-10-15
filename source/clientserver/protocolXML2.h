#ifndef UDA_CLIENTSERVER_PROTOCOLXML2_H
#define UDA_CLIENTSERVER_PROTOCOLXML2_H

#include <stdio.h>
#include <rpc/rpc.h>
#include <structures/genStructs.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

LIBRARY_API int protocolXML2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                             USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                             NTREE* full_ntree,
                             LOGSTRUCTLIST* log_struct_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML2_H

