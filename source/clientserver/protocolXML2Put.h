#pragma once

#include <rpc/rpc.h>
#include <stdio.h>

#include "structures/genStructs.h"

namespace uda::client_server
{

int xdr_user_defined_data_put(XDR* xdrs, uda::structures::LogMallocList* logmalloclist,
                              uda::structures::LogStructList* log_struct_list,
                              uda::structures::UserDefinedTypeList* userdefinedtypelist,
                              uda::structures::UserDefinedType* userdefinedtype, void** data, int datacount,
                              int structRank, int* structShape, int index, uda::structures::NTree** NTree,
                              int protocolVersion, int malloc_source);

// Send/Receive Array of Structures

bool_t xdr_user_defined_type(XDR* xdrs, uda::structures::UserDefinedTypeList* userdefinedtypelist,
                             uda::structures::UserDefinedType* str);

bool_t xdr_user_defined_type_list_put(XDR* xdrs, uda::structures::UserDefinedTypeList* str);

int protocol_xml2_put(XDR* xdrs, int protocol_id, int direction, int* token,
                      uda::structures::LogMallocList* logmalloclist,
                      uda::structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                      uda::structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace uda::client_server
