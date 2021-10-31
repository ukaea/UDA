#ifndef UDA_CLIENTSERVER_UDATYPES_H
#define UDA_CLIENTSERVER_UDATYPES_H

#ifdef __cplusplus
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Data Type Codes
 */
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

/**
 * Opaque Structure Types
 */
typedef enum UdaOpaqueTypes {
    UDA_OPAQUE_TYPE_UNKNOWN = 0,
    UDA_OPAQUE_TYPE_XML_DOCUMENT = 1,
    UDA_OPAQUE_TYPE_STRUCTURES =  2,
    UDA_OPAQUE_TYPE_XDRFILE = 3,
    UDA_OPAQUE_TYPE_XDROBJECT = 4
} UDA_OPAQUE_TYPE;

LIBRARY_API size_t getSizeOf(UDA_TYPE data_type);
LIBRARY_API size_t getPtrSizeOf(UDA_TYPE data_type);

#ifdef __cplusplus
}
#endif

#endif // UDA_CLIENTSERVER_UDATYPES_H
