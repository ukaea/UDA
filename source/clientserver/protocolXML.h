#ifndef UDA_CLIENTSERVER_PROTOCOLXML_H
#define UDA_CLIENTSERVER_PROTOCOLXML_H

#include <stdio.h> // this must be included before rpc.h
#include <rpc/rpc.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

int protocolXML(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOLXML_H
