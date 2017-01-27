#ifndef IDAM_CLIENTSERVER_PROTOCOLXML_H
#define IDAM_CLIENTSERVER_PROTOCOLXML_H

#include <stdio.h> // this must be included before rpc.h
#include <rpc/rpc.h>

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

int protocolXML(XDR * xdrs, int protocol_id, int direction, int * token, void * str);

#endif // IDAM_CLIENTSERVER_PROTOCOLXML_H

