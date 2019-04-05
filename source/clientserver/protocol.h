#ifndef UDA_CLIENTSERVER_PROTOCOL_H
#define UDA_CLIENTSERVER_PROTOCOL_H

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <structures/genStructs.h>

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------
// Client Server Conversation Protocols

#define PROTOCOL_REGULAR_START      0       // Identifies Regular Data Protocol Group
#define PROTOCOL_REQUEST_BLOCK      1
#define PROTOCOL_DATA_BLOCK         2
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

enum REQUEST {
    REQUEST_SHUTDOWN = 1,
    REQUEST_READ_GENERIC,       // Generic Signal via the IDAM Database
    REQUEST_READ_IDA,           // an IDA File
    REQUEST_READ_MDS,           // an MDSPlus Server
    REQUEST_READ_IDAM,          // a Remote IDAM server
    REQUEST_READ_FORMAT,        // Server to Choose Plugin for Requested Format
    REQUEST_READ_CDF,           // netCDF File
    REQUEST_READ_HDF5,          // HDF5 FIle
    REQUEST_READ_XML,           // XML Document defining a Signal
    REQUEST_READ_UFILE,         // TRANSP UFile
    REQUEST_READ_FILE,          // Read a File: A Container of Bytes!
    REQUEST_READ_SQL,           // Read from an SQL Data Source
    REQUEST_READ_PPF,           // JET PPF
    REQUEST_READ_JPF,           // JET JPF
    REQUEST_READ_NEW_PLUGIN,
    REQUEST_READ_NOTHING,       // Immediate Return without Error: Client Server Timing Tests
    REQUEST_READ_BLOCKED,       // Disable Server Option for External Users (Not a client side option)
    REQUEST_READ_HDATA,         // Hierarchical Data Structures
    REQUEST_READ_SERVERSIDE,    // Server Side Functions
    REQUEST_READ_UNKNOWN,       // Plugin Not Known
    REQUEST_READ_WEB,           // a Remote or Local web server
    REQUEST_READ_BIN,           // Binary file
    REQUEST_READ_HELP,          // Help file
    REQUEST_READ_DEVICE         // Request to an External Device's data server
};

extern int protocolVersion; // Client or Server Version number for Protocol Configuration

int protocol(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
             USERDEFINEDTYPELIST* userdefinedtypelist, void* str);
int protocol2(XDR* xdrs, int protocol_id, int direction, int* token, LOGMALLOCLIST* logmalloclist,
              USERDEFINEDTYPELIST* userdefinedtypelist, void* str);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_PROTOCOL_H
