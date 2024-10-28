#pragma once

//-------------------------------------------------------
// Request specific errors

#define SIGNAL_ARG_TOO_LONG 18000
#define SOURCE_ARG_TOO_LONG 18001
#define ARCHIVE_NAME_TOO_LONG 18002
#define DEVICE_NAME_TOO_LONG 18003
#define NO_SERVER_SPECIFIED 18004

//-------------------------------------------------------
// Fatal Server Errors

#define FATAL_ERROR_LOGS 666

//-------------------------------------------------------
// Client Server Conversation Protocol XDR Errors

namespace uda::client_server {

enum class ProtocolError : int {

};

}

#define UDA_PROTOCOL_ERROR_1 1
#define UDA_PROTOCOL_ERROR_2 2
#define UDA_PROTOCOL_ERROR_3 3
#define UDA_PROTOCOL_ERROR_4 4
#define UDA_PROTOCOL_ERROR_5 5
#define UDA_PROTOCOL_ERROR_61 61
#define UDA_PROTOCOL_ERROR_62 62
#define UDA_PROTOCOL_ERROR_63 63
#define UDA_PROTOCOL_ERROR_64 64
#define UDA_PROTOCOL_ERROR_65 65
#define UDA_PROTOCOL_ERROR_7 7
#define UDA_PROTOCOL_ERROR_8 8
#define UDA_PROTOCOL_ERROR_9 9
#define UDA_PROTOCOL_ERROR_10 10
#define UDA_PROTOCOL_ERROR_11 11
#define UDA_PROTOCOL_ERROR_12 12
#define UDA_PROTOCOL_ERROR_13 13
#define UDA_PROTOCOL_ERROR_14 14
#define UDA_PROTOCOL_ERROR_15 15
#define UDA_PROTOCOL_ERROR_16 16
#define UDA_PROTOCOL_ERROR_17 17
#define UDA_PROTOCOL_ERROR_18 18
#define UDA_PROTOCOL_ERROR_19 19
#define UDA_PROTOCOL_ERROR_20 20
#define UDA_PROTOCOL_ERROR_21 21
#define UDA_PROTOCOL_ERROR_22 22
#define UDA_PROTOCOL_ERROR_23 23
#define UDA_PROTOCOL_ERROR_24 24

#define UDA_PROTOCOL_ERROR_9999 9999

//-------------------------------------------------------
// Server Side Error Codes

#define UNCOMPRESS_ALLOCATING_HEAP 40
#define UNKNOWN_DATA_TYPE 41
#define ERROR_ALLOCATING_HEAP 42
#define ERROR_ALLOCATING_META_DATA_HEAP 43