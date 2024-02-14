#pragma once

#include <rpc/rpc.h>
#include <stdio.h>

#include "structures/genStructs.h"

namespace uda::client_server
{

int xdr_user_defined_data_put(XDR* xdrs, LogMallocList* logmalloclist, LogStructList* log_struct_list,
                          UserDefinedTypeList* userdefinedtypelist, UserDefinedType* userdefinedtype, void** data,
                          int datacount, int structRank, int* structShape, int index, NTree** NTree,
                          int protocolVersion, int malloc_source);

// Send/Receive Array of Structures

bool_t xdr_user_defined_type(XDR* xdrs, UserDefinedTypeList* userdefinedtypelist, UserDefinedType* str);

bool_t xdr_user_defined_type_list_put(XDR* xdrs, UserDefinedTypeList* str);

int protocol_xml2_put(XDR* xdrs, int protocol_id, int direction, int* token, LogMallocList* logmalloclist,
                    UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                      LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

} // namespace uda::client_server
