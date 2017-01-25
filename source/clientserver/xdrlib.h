#ifndef IDAM_CLIENTSERVER_XDRLIB_H
#define IDAM_CLIENTSERVER_XDRLIB_H

#include "idamStructs.h"

#include <rpc/types.h>
#include <rpc/xdr.h>

//-----------------------------------------------------------------------
// Test version's type passing capability

int protocolVersionTypeTest(int protocolVersion, int type);

int wrap_string(XDR *xdrs, char *sp);

int WrapXDRString(XDR *xdrs, char *sp, int maxlen);

bool_t xdr_meta(XDR* xdrs, DATA_BLOCK* str);
bool_t xdr_securityBlock1(XDR *xdrs, SECURITY_BLOCK *str);
bool_t xdr_securityBlock2(XDR *xdrs, SECURITY_BLOCK *str);
bool_t xdr_client(XDR *xdrs, CLIENT_BLOCK *str);
bool_t xdr_server(XDR *xdrs, SERVER_BLOCK *str);
bool_t xdr_server1(XDR *xdrs, SERVER_BLOCK *str);
bool_t xdr_server2(XDR *xdrs, SERVER_BLOCK *str);
bool_t xdr_request(XDR *xdrs, REQUEST_BLOCK *str);
bool_t xdr_putdatablocklist_block(XDR *xdrs, PUTDATA_BLOCK_LIST *str);
bool_t xdr_putdata_block1XXX(XDR *xdrs, PUTDATA_BLOCK *str);
bool_t xdr_putdata_block1(XDR *xdrs, PUTDATA_BLOCK *str);
bool_t xdr_putdata_block2(XDR *xdrs, PUTDATA_BLOCK *str);
bool_t xdr_data_block1(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_block2(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_block3(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_block4(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_dim1(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_dim2(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_dim3(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_dim4(XDR *xdrs, DATA_BLOCK *str);
bool_t xdr_data_object1(XDR *xdrs, DATA_OBJECT *str);
bool_t xdr_data_object2(XDR *xdrs, DATA_OBJECT *str);

//-----------------------------------------------------------------------
// From DATA_SYSTEM Table
bool_t xdr_data_system(XDR *xdrs, DATA_SYSTEM *str);

//-----------------------------------------------------------------------
// From SYSTEM_CONFIG Table
bool_t xdr_system_config(XDR *xdrs, SYSTEM_CONFIG *str);

//-----------------------------------------------------------------------
// From DATA_SOURCE Table
bool_t xdr_data_source(XDR *xdrs, DATA_SOURCE *str);

//-----------------------------------------------------------------------
// From SIGNAL Table
bool_t xdr_signal(XDR *xdrs, SIGNAL *str);

//-----------------------------------------------------------------------
// From SIGNAL_DESC Table
bool_t xdr_signal_desc(XDR *xdrs, SIGNAL_DESC *str);

#endif // IDAM_CLIENTSERVER_XDRLIB_H

