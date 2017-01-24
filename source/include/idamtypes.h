//! $LastChangedRevision: 353 $
//! $LastChangedDate: 2013-11-18 15:32:28 +0000 (Mon, 18 Nov 2013) $
//! $LastChangedBy: dgm $
//! $HeadURL: https://fussvn.fusion.culham.ukaea.org.uk/svnroot/IDAM/development/source/include/idamtypes.h $

#ifndef IdamTypesInclude
#define IdamTypesInclude

// Client and ServerData Types
//
// Change History
//
// 01Sep2006 dgm    Original version
// 02Apr2008 dgm    C++ test added for inclusion of extern "C"
// 11Aug2008 dgm    Added remaining data types: unsigned char, unsigned short, unsigned long
// 04Nov2009 dgm    Type String added
// 09Nov2009 dgm    VLEN, STRING, COMPOUND, OPAQUE and ENUM type added
// 10Dec2009 dgm    VOID type added
//----------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
