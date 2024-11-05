#pragma once

namespace uda::client_server {

//-------------------------------------------------------
// Client Server Conversation Protocol XDR Errors

enum class ProtocolError : int {
    Error1  = 1,
    Error2  = 2,
    Error3  = 3,
    Error4  = 4,
    Error5  = 5,
    Error61 = 61,
    Error62 = 62,
    Error63 = 63,
    Error64 = 64,
    Error65 = 65,
    Error7  = 7,
    Error8  = 8,
    Error9  = 9,
    Error10 = 10,
    Error11 = 11,
    Error12 = 12,
    Error13 = 13,
    Error14 = 14,
    Error15 = 15,
    Error16 = 16,
    Error17 = 17,
    Error18 = 18,
    Error19 = 19,
    Error20 = 20,
    Error21 = 21,
    Error22 = 22,
    Error23 = 23,
    Error24 = 24,
    Error9999 = 9999,
};

//-------------------------------------------------------
// Server Side Error Codes

enum class ServerSideError : int {
    UncompressAllocatingHeap = 40,
    UnknownDataType = 41,
    ErrorAllocatingHeap = 42,
    ErrorAllocatingMetaDataHeap = 42,
};

//-------------------------------------------------------
// Fatal Server Errors

enum class FatalError : int {
    Logs = 666,
};

//-------------------------------------------------------
// Request specific errors

enum class RequestError : int {
    SignalArgTooLong = 18000,
    SourceArgTooLong = 18001,
    ArchiveNameTooLong = 18002,
    DeviceNameTooLong = 18003,
    NoServerSpecified = 18004,
};

}
