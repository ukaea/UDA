#ifndef UDA_CLIENTSERVER_PROTOCOL_H
#define UDA_CLIENTSERVER_PROTOCOL_H

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <structures/genStructs.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------
// Client Server Conversation Protocols

#define PROTOCOL_REGULAR_START      0       // Identifies Regular Data Protocol Group
#define PROTOCOL_REQUEST_BLOCK      1
#define PROTOCOL_DATA_BLOCK_LIST    2
#define PROTOCOL_NEXT_PROTOCOL      3
#define PROTOCOL_DATA_SYSTEM        4
#define PROTOCOL_SYSTEM_CONFIG      5
#define PROTOCOL_DATA_SOURCE        6
#define PROTOCOL_SIGNAL             7
#define PROTOCOL_SIGNAL_DESC        8
#define PROTOCOL_SPARE1             9
#define PROTOCOL_CLIENT_BLOCK       10
#define PROTOCOL_SERVER_BLOCK       11
#define PROTOCOL_SPARE2             12
#define PROTOCOL_CLOSEDOWN          13
#define PROTOCOL_SLEEP              14
#define PROTOCOL_WAKE_UP            15
#define PROTOCOL_PUTDATA_BLOCK_LIST 16
#define PROTOCOL_SECURITY_BLOCK     17
#define PROTOCOL_OBJECT             18
#define PROTOCOL_SERIALISE_OBJECT   19
#define PROTOCOL_SERIALISE_FILE     20
#define PROTOCOL_DATAOBJECT         21
#define PROTOCOL_DATAOBJECT_FILE    22
#define PROTOCOL_REGULAR_STOP       99

#define PROTOCOL_OPAQUE_START       100         // Identifies Legacy Hierarchical Data Protocol Group
#define PROTOCOL_STRUCTURES         101
#define PROTOCOL_META               102
#define PROTOCOL_EFIT               103
#define PROTOCOL_PFCOILS            104
#define PROTOCOL_PFPASSIVE          105
#define PROTOCOL_PFSUPPLIES         106
#define PROTOCOL_FLUXLOOP           107
#define PROTOCOL_MAGPROBE           108
#define PROTOCOL_PFCIRCUIT          109
#define PROTOCOL_PLASMACURRENT      110
#define PROTOCOL_DIAMAGNETIC        111
#define PROTOCOL_TOROIDALFIELD      112
#define PROTOCOL_LIMITER            113
#define PROTOCOL_OPAQUE_STOP        200

//---------------------------------------------------------------------------------------------------
// Client Server XDR data Streams (DON'T CHANGE ORDER or Legacy client won't work!)

LIBRARY_API int protocol(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                         USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion, NTREE* full_ntree,
                         LOGSTRUCTLIST* log_struct_list);
LIBRARY_API int protocol2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
                          USERDEFINEDTYPELIST* userdefinedtypelist, void* str, int protocolVersion, NTREE* full_ntree,
                          LOGSTRUCTLIST* log_struct_list);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOL_H
