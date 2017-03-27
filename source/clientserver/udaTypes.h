#ifndef IDAM_CLIENTSERVER_IDAMTYPES_H
#define IDAM_CLIENTSERVER_IDAMTYPES_H

// Client and ServerData Types
//
//----------------------------------------------------------------

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FALSE (0)
#define TRUE (1)
#define BOOLEAN unsigned short

// Data Type Codes

#define TYPE_UNKNOWN                0
#define TYPE_CHAR                   1
#define TYPE_SHORT                  2
#define TYPE_INT                    3
#define TYPE_UNSIGNED               4
#define TYPE_UNSIGNED_INT           4
#define TYPE_LONG                   5       // Same as INT on most systems, but may be different!
#define TYPE_FLOAT                  6
#define TYPE_DOUBLE                 7

#define TYPE_UNSIGNED_CHAR          8
#define TYPE_UNSIGNED_SHORT         9
#define TYPE_UNSIGNED_LONG          10

#define TYPE_LONG64                 11
#define TYPE_UNSIGNED_LONG64        12 // __APPLE__ unable to handle this type - preserve for Linux server sources

#define TYPE_COMPLEX                13
#define TYPE_DCOMPLEX               14

#define TYPE_UNDEFINED              15  // *** This needs to be dropped!!!

#define TYPE_VLEN                   16
#define TYPE_STRING                 17
#define TYPE_COMPOUND               18
#define TYPE_OPAQUE                 19
#define TYPE_ENUM                   20
#define TYPE_VOID                   21

#define TYPE_STRING2                99

//-------------------------------------------------------
// Opaque Structure Types

#define OPAQUE_TYPE_UNKNOWN         0
#define OPAQUE_TYPE_XML_DOCUMENT    1
#define OPAQUE_TYPE_STRUCTURES      2
#define OPAQUE_TYPE_XDRFILE         3
#define OPAQUE_TYPE_XDROBJECT       4

#define OPAQUE_TYPE_EFIT            100
#define OPAQUE_TYPE_PFCOILS         101
#define OPAQUE_TYPE_PFPASSIVE       102
#define OPAQUE_TYPE_PFSUPPLIES      103
#define OPAQUE_TYPE_FLUXLOOP        104
#define OPAQUE_TYPE_MAGPROBE        105
#define OPAQUE_TYPE_PFCIRCUIT       106
#define OPAQUE_TYPE_PLASMACURRENT   107
#define OPAQUE_TYPE_DIAMAGNETIC     108
#define OPAQUE_TYPE_TOROIDALFIELD   109
#define OPAQUE_TYPE_LIMITER         110

size_t getSizeOf(int data_type);

#ifdef __cplusplus
}
#endif

#endif // IDAM_CLIENTSERVER_IDAMTYPES_H
