#ifndef IDAM_CLIENTSERVER_PROTOCOLXML2_H
#define IDAM_CLIENTSERVER_PROTOCOLXML2_H

#include <stdio.h>
#include <rpc/rpc.h>
#include <structures/genStructs.h>

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

int protocolXML2(XDR * xdrs, int protocol_id, int direction, int * token, LOGMALLOCLIST* logmalloclist,
                 USERDEFINEDTYPELIST* userdefinedtypelist, void * str);

#endif // IDAM_CLIENTSERVER_PROTOCOLXML2_H

