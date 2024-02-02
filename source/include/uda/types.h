#ifndef UDA_TYPES_H
#define UDA_TYPES_H

#ifdef __cplusplus
#  include <cstdlib>
#else
#  include <stdlib.h>
#endif

#include "include/uda/export.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define NO_SOCKET_CONNECTION (-10000)
//#define PROBLEM_OPENING_LOGS (-11000)
//#define FILE_FORMAT_NOT_SUPPORTED (-12000)
//#define ERROR_ALLOCATING_DATA_BOCK_HEAP (-13000)
//#define SERVER_BLOCK_ERROR (-14000)
//#define SERVER_SIDE_ERROR (-14001)
//#define DATA_BLOCK_RECEIPT_ERROR (-15000)
//#define ERROR_CONDITION_UNKNOWN (-16000)
//
//#define NO_EXP_NUMBER_SPECIFIED (-18005)
//
//#define MIN_STATUS (-1)          // Deny Access to Data if this Status Value
//#define DATA_STATUS_BAD (-17000) // Error Code if Status is Bad

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
    UDA_TYPE_VOID = 21,
    UDA_TYPE_CAPNP = 22,
    UDA_TYPE_STRING2 = 99
} UDA_TYPE;

/**
 * Opaque Structure Types
 */
typedef enum UdaOpaqueTypes {
    UDA_OPAQUE_TYPE_UNKNOWN = 0,
    UDA_OPAQUE_TYPE_XML_DOCUMENT = 1,
    UDA_OPAQUE_TYPE_STRUCTURES = 2,
    UDA_OPAQUE_TYPE_XDRFILE = 3,
    UDA_OPAQUE_TYPE_XDROBJECT = 4
} UDA_OPAQUE_TYPE;

LIBRARY_API size_t getSizeOf(UDA_TYPE data_type);
LIBRARY_API size_t getPtrSizeOf(UDA_TYPE data_type);

#define SCALARDOUBLE 1
#define ARRAYDOUBLE 2
#define SCALARFLOAT 3
#define ARRAYFLOAT 4
#define SCALARLONG64 5
#define ARRAYLONG64 6
#define SCALARULONG64 7
#define ARRAYULONG64 8
#define SCALARINT 9
#define ARRAYINT 10
#define SCALARUINT 11
#define ARRAYUINT 12
#define SCALARSHORT 13
#define ARRAYSHORT 14
#define SCALARUSHORT 15
#define ARRAYUSHORT 16
#define SCALARCHAR 17
#define ARRAYCHAR 18
#define SCALARSTRING 19
#define ARRAYSTRING 20
#define ARRAYVOID 21
#define SCALARUCHAR 22
#define ARRAYUCHAR 23
#define SCALARCOMPOUND 24
#define ARRAYCOMPOUND 25

typedef struct NTree NTREE;
typedef struct NTreeList NTREELIST;
typedef struct LogMallocList LOGMALLOCLIST;
typedef struct LogMalloc LOGMALLOC;
typedef struct LogStruct LOGSTRUCT;
typedef struct LogStructList LOGSTRUCTLIST;
typedef struct CompoundField COMPOUNDFIELD;
typedef struct UserDefinedType USERDEFINEDTYPE;
typedef struct UserDefinedTypeList USERDEFINEDTYPELIST;
typedef struct UdaErrorStack UDA_ERROR_STACK;
typedef struct UdaPluginInterface UDA_PLUGIN_INTERFACE;
typedef struct CompoundField COMPOUNDFIELD;
typedef struct UserDefinedType USERDEFINEDTYPE;
typedef struct PutDataBlockList PUTDATA_BLOCK_LIST;
typedef struct PutDataBlock PUTDATA_BLOCK;

#ifdef __cplusplus
}
#endif

#endif // UDA_TYPES_H
