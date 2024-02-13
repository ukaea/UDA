#pragma once

#include <cstdio> // this must be included before rpc.h
#include <rpc/rpc.h>
#include <tuple>

#include "structures/genStructs.h"

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

struct IoData;

using CreateXDRStreams = std::pair<XDR*, XDR*> (*)(IoData*);

namespace uda::client_server
{

int protocolXML(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                LOGSTRUCTLIST* log_struct_list, IoData* io_data, unsigned int private_flags, int malloc_source,
                CreateXDRStreams create_xdr_streams);

}
