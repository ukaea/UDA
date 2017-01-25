#ifndef IDAM_CLIENTSERVER_PROTOCOL_H
#define IDAM_CLIENTSERVER_PROTOCOL_H

#include <rpc/types.h>
#include <rpc/xdr.h>

int protocol (XDR *xdrs, int protocol_id, int direction, int *token, void *str);

#endif // IDAM_CLIENTSERVER_PROTOCOL_H
