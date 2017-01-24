#ifndef IDAM_CLIENTSERVER_PROTOCOLXML2PUT_H
#define IDAM_CLIENTSERVER_PROTOCOLXML2PUT_H

#include <stdio.h>
#include <rpc/rpc.h>
#include <include/idamgenstruct.h>

int xdrUserDefinedDataPut(XDR *xdrs, USERDEFINEDTYPE *userdefinedtype, void **data, int datacount, int structRank, int* structShape,
                          int index, NTREE **NTree);

// Send/Receive Array of Structures

int xdrUserDefinedTypeDataPut(XDR *xdrs, USERDEFINEDTYPE *userdefinedtype, void **data);

bool_t xdr_userdefinedtypelistPut(XDR *xdrs, USERDEFINEDTYPELIST *str);

int protocolXML2Put(XDR *xdrs, int protocol_id, int direction, int *token, void *str);

#endif // IDAM_CLIENTSERVER_PROTOCOLXML2PUT_H
