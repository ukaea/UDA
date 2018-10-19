#ifndef UDA_CLIENTSERVER_UDATYPES_H
#define UDA_CLIENTSERVER_UDATYPES_H

// Client and ServerData Types
//
//----------------------------------------------------------------

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Data Type Codes
typedef enum UdaType {
    UDA_TYPE_UNKNOWN = 0,
    UDA_TYPE_CHAR = 1,
    UDA_TYPE_SHORT = 2,
    UDA_TYPE_INT = 3,
    UDA_TYPE_UNSIGNED_INT = 4,
    UDA_TYPE_LONG = 5,
    UDA_TYPE_FLOAT = 6,
    UDA_TYPE_DOUBLE = 7,
    UDA_TYPE_UNSIGNED_CHAR = 8,
    UDA_TYPE_UNSIGNED_SHORT = 9,
    UDA_TYPE_UNSIGNED_LONG = 10,
    UDA_TYPE_LONG64 = 11,
    UDA_TYPE_UNSIGNED_LONG64 = 12,
    UDA_TYPE_COMPLEX = 13,
    UDA_TYPE_DCOMPLEX = 14,
    UDA_TYPE_UNDEFINED = 15,
    UDA_TYPE_VLEN = 16,
    UDA_TYPE_STRING = 17,
    UDA_TYPE_COMPOUND = 18,
    UDA_TYPE_OPAQUE = 19,
    UDA_TYPE_ENUM = 20,
    UDA_TYPE_VOID  = 21,
    UDA_TYPE_STRING2 = 99
} UDA_TYPE;

//-------------------------------------------------------
// Opaque Structure Types

typedef enum UdaOpaqueTypes {
    UDA_OPAQUE_TYPE_UNKNOWN = 0,
    UDA_OPAQUE_TYPE_XML_DOCUMENT = 1,
    UDA_OPAQUE_TYPE_STRUCTURES =  2,
    UDA_OPAQUE_TYPE_XDRFILE = 3,
    UDA_OPAQUE_TYPE_XDROBJECT = 4,
    UDA_OPAQUE_TYPE_EFIT = 100,
    UDA_OPAQUE_TYPE_PFCOILS = 101,
    UDA_OPAQUE_TYPE_PFPASSIVE = 102,
    UDA_OPAQUE_TYPE_PFSUPPLIES = 103,
    UDA_OPAQUE_TYPE_FLUXLOOP = 104,
    UDA_OPAQUE_TYPE_MAGPROBE  = 105,
    UDA_OPAQUE_TYPE_PFCIRCUIT = 106,
    UDA_OPAQUE_TYPE_PLASMACURRENT = 107,
    UDA_OPAQUE_TYPE_DIAMAGNETIC = 108,
    UDA_OPAQUE_TYPE_TOROIDALFIELD = 109,
    UDA_OPAQUE_TYPE_LIMITER = 110
} UDA_OPAQUE_TYPE;

size_t getSizeOf(UDA_TYPE data_type);
size_t getPtrSizeOf(UDA_TYPE data_type);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_UDATYPES_H
