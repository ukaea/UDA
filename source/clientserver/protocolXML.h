#pragma once

#include <cstdio> // this must be included before rpc.h
#include <rpc/rpc.h>
#include <tuple>

#include "structures/genStructs.h"
#include "xdrlib.h"

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

namespace uda::client_server
{

enum class ProtocolId;

struct IoData;

using CreateXDRStreams = std::pair<XDR*, XDR*> (*)(uda::client_server::IoData*);

int protocol_xml(XDR* xdrs, ProtocolId protocol_id, XDRStreamDirection direction, ProtocolId* token, uda::structures::LogMallocList* logmalloclist,
                 uda::structures::UserDefinedTypeList* userdefinedtypelist, void* str, int protocolVersion,
                 uda::structures::LogStructList* log_struct_list, IoData* io_data, unsigned int private_flags,
                 int malloc_source, CreateXDRStreams create_xdr_streams);

} // namespace uda::client_server
