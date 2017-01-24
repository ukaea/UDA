
#ifndef IDAM_PROTOCOL2_H
#define IDAM_PROTOCOL2_H

#include <rpc/types.h>
#include <rpc/xdr.h>

int protocol2(XDR *xdrs, int protocol_id, int direction, int *token, void *str);

#endif // IDAM_PROTOCOL2_H
