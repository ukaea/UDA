#ifndef UDA_CLIENTSERVER_PROTOCOLXML2_H
#define UDA_CLIENTSERVER_PROTOCOLXML2_H

#include "export.h"
#include "genStructs.h"
#include <rpc/rpc.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

LIBRARY_API int protocolXML2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                             USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                             LOGSTRUCTLIST* log_struct_list, unsigned int private_flags, int malloc_source);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML2_H
