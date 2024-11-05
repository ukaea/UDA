#pragma once

#include "structures/genStructs.h"
#include "xdrlib.h"

#include <cstdio>
#include <rpc/rpc.h>

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

namespace uda::client_server
{

enum class ProtocolId;

int protocol_xml2(XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, uda::structures::LogMallocList* logmalloclist,
                  uda::structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                  uda::structures::LogStructList* log_struct_list, unsigned int private_flags, int malloc_source);

int xdr_user_defined_type_data(XDR* xdrs, uda::structures::LogMallocList* logmalloclist,
                               uda::structures::UserDefinedTypeList* userdefinedtypelist,
                               uda::structures::UserDefinedType* userdefinedtype, void** data, int protocolVersion,
                               bool xdr_stdio_flag, uda::structures::LogStructList* log_struct_list, int malloc_source);

bool_t xdr_user_defined_type_list(XDR* xdrs, uda::structures::UserDefinedTypeList* str, bool xdr_stdio_flag);

} // namespace uda::client_server
