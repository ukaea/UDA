#pragma once

#include <rpc/rpc.h>
#include <stdio.h>

#include "structures/genStructs.h"
#include "xdrlib.h"

namespace uda::protocol
{

enum class ProtocolId;

int xdr_user_defined_data_put(XDR* xdrs, structures::LogMallocList* logmalloclist,
                              structures::LogStructList* log_struct_list,
                              structures::UserDefinedTypeList* userdefinedtypelist,
                              structures::UserDefinedType* userdefinedtype, void** data, int datacount,
                              int structRank, int* structShape, int index, structures::NTree** NTree,
                              int protocolVersion, int malloc_source);

// Send/Receive Array of Structures

bool_t xdr_user_defined_type(XDR* xdrs, structures::UserDefinedTypeList* userdefinedtypelist,
                             structures::UserDefinedType* str);

bool_t xdr_user_defined_type_list_put(XDR* xdrs, structures::UserDefinedTypeList* str);

int protocol_xml2_put(XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token,
                      structures::LogMallocList* logmalloclist,
                      structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                      structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace uda::client_server
