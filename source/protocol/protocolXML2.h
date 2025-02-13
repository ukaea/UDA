#pragma once

#include "structures/genStructs.h"
#include "xdrlib.h"

#include <cstdio>
#include <rpc/rpc.h>

#ifdef FATCLIENT
#  define protocolXML2 protocolXML2Fat
#endif

namespace uda::protocol
{

enum class ProtocolId;

int protocol_xml2(std::vector<client_server::UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token,
                  structures::LogMallocList* logmalloclist, structures::UserDefinedTypeList* userdefinedtypelist,
                  void* str, int protocolVersion, structures::LogStructList* log_struct_list,
                  unsigned int private_flags, int malloc_source);

int xdr_user_defined_type_data(std::vector<client_server::UdaError>& error_stack, XDR* xdrs, structures::LogMallocList* logmalloclist,
                               structures::UserDefinedTypeList* userdefinedtypelist,
                               structures::UserDefinedType* userdefinedtype, void** data, int protocolVersion,
                               bool xdr_stdio_flag, structures::LogStructList* log_struct_list, int malloc_source);

bool_t xdr_user_defined_type_list(XDR* xdrs, structures::UserDefinedTypeList* str, bool xdr_stdio_flag);

} // namespace uda::client_server
