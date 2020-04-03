#ifndef UDA_CLIENTSERVER_PROTOCOLXML2_H
#define UDA_CLIENTSERVER_PROTOCOLXML2_H

#include <stdio.h>
#include <rpc/rpc.h>
#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

LIBRARY_API int protocolXML2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                 USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML2_H

