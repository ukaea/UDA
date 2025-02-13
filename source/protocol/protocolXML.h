#pragma once

#include <cstdio> // this must be included before rpc.h
#include <rpc/rpc.h>
#include <tuple>

#include "structures/genStructs.h"
#include "xdrlib.h"

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

namespace uda::protocol
{

enum class ProtocolId;

struct IoData;

using CreateXDRStreams = std::pair<XDR*, XDR*> (*)(IoData*);

int protocol_xml(std::vector<client_server::UdaError>& error_stack, XDR* xdrs, ProtocolId protocol_id,
                 XDRStreamDirection direction, ProtocolId* token, structures::LogMallocList* logmalloclist,
                 structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                 structures::LogStructList* log_struct_list, IoData* io_data, unsigned int private_flags,
                 int malloc_source, CreateXDRStreams create_xdr_streams);

} // namespace uda::client_server
