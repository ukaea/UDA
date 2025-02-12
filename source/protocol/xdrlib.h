#pragma once

#include "clientserver/uda_structs.h"

#include <rpc/types.h>
#include <rpc/xdr.h>

namespace uda::protocol
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

bool_t xdr_meta(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_security_block1(XDR* xdrs, client_server::SecurityBlock* str);

bool_t xdr_security_block2(XDR* xdrs, client_server::SecurityBlock* str);

bool_t xdr_client(XDR* xdrs, client_server::ClientBlock* str, int protocolVersion);

bool_t xdr_server(XDR* xdrs, client_server::ServerBlock* str);

bool_t xdr_server1(XDR* xdrs, client_server::ServerBlock* str, int protocolVersion);

bool_t xdr_server2(XDR* xdrs, client_server::ServerBlock* str);

bool_t xdr_request(XDR* xdrs, client_server::RequestBlock* str, int protocolVersion);

bool_t xdr_request_data(XDR* xdrs, client_server::RequestData* str, int protocolVersion);

bool_t xdr_putdatablocklist_block(XDR* xdrs, client_server::PutDataBlockList* str);

bool_t xdr_putdata_block1(XDR* xdrs, client_server::PutDataBlock* str);

bool_t xdr_putdata_block2(XDR* xdrs, client_server::PutDataBlock* str);

bool_t xdr_data_block_list(XDR* xdrs, std::vector<client_server::DataBlock>* str, int protocolVersion);

bool_t xdr_data_block1(XDR* xdrs, client_server::DataBlock* str, int protocolVersion);

bool_t xdr_data_block2(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_block3(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_block4(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_dim1(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_dim2(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_dim3(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_dim4(XDR* xdrs, client_server::DataBlock* str);

bool_t xdr_data_object1(XDR* xdrs, client_server::DataObject* str);

bool_t xdr_data_object2(XDR* xdrs, client_server::DataObject* str);

bool_t xdr_metadata(XDR* xdrs, client_server::MetaData* str);

bool_t xdr_metadata_field(XDR* xdrs, client_server::MetaDataField* str);

} // namespace uda::client_server
