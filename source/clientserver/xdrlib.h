#pragma once

#include "udaStructs.h"

#include <rpc/types.h>
#include <rpc/xdr.h>

#ifdef __APPLE__
#  define xdr_uint64_t xdr_u_int64_t
#endif

namespace uda::client_server
{

enum class XDRStreamDirection {
    Send = 0,
    Receive = 1,
    FreeHeap = 2,
};

//-----------------------------------------------------------------------
// Test version's type passing capability

int protocol_version_type_test(int protocolVersion, int type);

int wrap_string(XDR* xdrs, char* sp);

int wrap_xdr_string(XDR* xdrs, const char* sp, int maxlen);

bool_t xdr_meta(XDR* xdrs, DataBlock* str);

bool_t xdr_security_block1(XDR* xdrs, SecurityBlock* str);

bool_t xdr_security_block2(XDR* xdrs, SecurityBlock* str);

bool_t xdr_client(XDR* xdrs, ClientBlock* str, int protocolVersion);

bool_t xdr_server(XDR* xdrs, ServerBlock* str);

bool_t xdr_server1(XDR* xdrs, ServerBlock* str, int protocolVersion);

bool_t xdr_server2(XDR* xdrs, ServerBlock* str);

bool_t xdr_request(XDR* xdrs, RequestBlock* str, int protocolVersion);

bool_t xdr_request_data(XDR* xdrs, RequestData* str, int protocolVersion);

bool_t xdr_putdatablocklist_block(XDR* xdrs, PutDataBlockList* str);

bool_t xdr_putdata_block1(XDR* xdrs, PutDataBlock* str);

bool_t xdr_putdata_block2(XDR* xdrs, PutDataBlock* str);

bool_t xdr_data_block_list(XDR* xdrs, DataBlockList* str, int protocolVersion);

bool_t xdr_data_block1(XDR* xdrs, DataBlock* str, int protocolVersion);

bool_t xdr_data_block2(XDR* xdrs, DataBlock* str);

bool_t xdr_data_block3(XDR* xdrs, DataBlock* str);

bool_t xdr_data_block4(XDR* xdrs, DataBlock* str);

bool_t xdr_data_dim1(XDR* xdrs, DataBlock* str);

bool_t xdr_data_dim2(XDR* xdrs, DataBlock* str);

bool_t xdr_data_dim3(XDR* xdrs, DataBlock* str);

bool_t xdr_data_dim4(XDR* xdrs, DataBlock* str);

bool_t xdr_data_object1(XDR* xdrs, DataObject* str);

bool_t xdr_data_object2(XDR* xdrs, DataObject* str);

//-----------------------------------------------------------------------
// From DataSystem Table
bool_t xdr_data_system(XDR* xdrs, DataSystem* str);

//-----------------------------------------------------------------------
// From SystemConfig Table
bool_t xdr_system_config(XDR* xdrs, SystemConfig* str);

//-----------------------------------------------------------------------
// From DataSource Table
bool_t xdr_data_source(XDR* xdrs, DataSource* str);

//-----------------------------------------------------------------------
// From Signal Table
bool_t xdr_signal(XDR* xdrs, Signal* str);

//-----------------------------------------------------------------------
// From SignalDesc Table
bool_t xdr_signal_desc(XDR* xdrs, SignalDesc* str);

} // namespace uda::client_server
