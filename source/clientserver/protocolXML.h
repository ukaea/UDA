#pragma once

#ifndef UDA_CLIENTSERVER_PROTOCOLXML_H
#define UDA_CLIENTSERVER_PROTOCOLXML_H

#include <cstdio> // this must be included before rpc.h
#include <rpc/rpc.h>
#include <tuple>

#include <structures/genStructs.h>
#include "export.h"

#ifdef FATCLIENT
#  define protocolXML protocolXMLFat
#endif

struct IoData;

using CreateXDRStreams = std::pair<XDR*, XDR*> (*)(IoData*);

int protocolXML(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion, NTREE* full_ntree,
                LOGSTRUCTLIST* log_struct_list, IoData* io_data, unsigned int private_flags, int malloc_source,
                CreateXDRStreams create_xdr_streams);

#endif // UDA_CLIENTSERVER_PROTOCOLXML_H
