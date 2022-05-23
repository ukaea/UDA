#pragma once

#ifndef UDA_CLIENTSERVER_UDA_PROTOCOL_H
#define UDA_CLIENTSERVER_UDA_PROTOCOL_H

#include <rpc/types.h>
#include <rpc/xdr.h>

#include <structures/genStructs.h>

#include "export.h"

//-------------------------------------------------------
// Client Server Conversation Protocols

#define UDA_PROTOCOL_REGULAR_START      0       // Identifies Regular Data Protocol Group
#define UDA_PROTOCOL_REQUEST_BLOCK      1
#define UDA_PROTOCOL_DATA_BLOCK_LIST    2
#define UDA_PROTOCOL_NEXT_PROTOCOL      3
#define UDA_PROTOCOL_DATA_SYSTEM        4
#define UDA_PROTOCOL_SYSTEM_CONFIG      5
#define UDA_PROTOCOL_DATA_SOURCE        6
#define UDA_PROTOCOL_SIGNAL             7
#define UDA_PROTOCOL_SIGNAL_DESC        8
#define UDA_PROTOCOL_SPARE1             9
#define UDA_PROTOCOL_CLIENT_BLOCK       10
#define UDA_PROTOCOL_SERVER_BLOCK       11
#define UDA_PROTOCOL_SPARE2             12
#define UDA_PROTOCOL_CLOSEDOWN          13
#define UDA_PROTOCOL_SLEEP              14
#define UDA_PROTOCOL_WAKE_UP            15
#define UDA_PROTOCOL_PUTDATA_BLOCK_LIST 16
#define UDA_PROTOCOL_SECURITY_BLOCK     17
#define UDA_PROTOCOL_OBJECT             18
#define UDA_PROTOCOL_SERIALISE_OBJECT   19
#define UDA_PROTOCOL_SERIALISE_FILE     20
#define UDA_PROTOCOL_DATAOBJECT         21
#define UDA_PROTOCOL_DATAOBJECT_FILE    22
#define UDA_PROTOCOL_REGULAR_STOP       99

#define UDA_PROTOCOL_OPAQUE_START       100         // Identifies Legacy Hierarchical Data Protocol Group
#define UDA_PROTOCOL_STRUCTURES         101
#define UDA_PROTOCOL_META               102
#define UDA_PROTOCOL_EFIT               103
#define UDA_PROTOCOL_PFCOILS            104
#define UDA_PROTOCOL_PFPASSIVE          105
#define UDA_PROTOCOL_PFSUPPLIES         106
#define UDA_PROTOCOL_FLUXLOOP           107
#define UDA_PROTOCOL_MAGPROBE           108
#define UDA_PROTOCOL_PFCIRCUIT          109
#define UDA_PROTOCOL_PLASMACURRENT      110
#define UDA_PROTOCOL_DIAMAGNETIC        111
#define UDA_PROTOCOL_TOROIDALFIELD      112
#define UDA_PROTOCOL_LIMITER            113
#define UDA_PROTOCOL_OPAQUE_STOP        200

struct IoData;

//---------------------------------------------------------------------------------------------------
// Client Server XDR data Streams (DON'T CHANGE ORDER or Legacy client won't work!)

LIBRARY_API int protocol(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                         LOGSTRUCTLIST* log_struct_list,
                         IoData* io_data, unsigned int private_flags, int malloc_source);
LIBRARY_API int protocol2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion,
                          LOGSTRUCTLIST* log_struct_list,
                          unsigned int private_flags, int malloc_source);

#endif // UDA_CLIENTSERVER_UDA_PROTOCOL_H
