#ifndef UDA_STRUCTURES_GENSTRUCTS_H
#define UDA_STRUCTURES_GENSTRUCTS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <rpc/xdr.h>


//-------------------------------------------------------------------------------------------------------
// Macro definitions

#define MAXELEMENTS 256        // Max number of structure elements
#define MAXRANK 7              // Max rank of arrays within structure
#define MAXELEMENTNAME 256     // structure element name
#define MAXSOAPSTACKSTRING 256 // SOAP strings allocated on the stack must be less than this size

#define GROWMALLOCLIST 20 // Each time realloc is called, increase the heap by this number of array elements

#define POINTER_SIZE32 4 // Pointer size (bytes) on 32-bit architecture

#define MAXRECURSIVEDEPTH 30

typedef intptr_t VOIDTYPE;
typedef uintptr_t UVOIDTYPE;

#ifdef A64
#  define ALIGNMENT 1 // Default Byte Boundary used for Structure Packing
#else
#  define ALIGNMENT 1 // Default Byte Boundary used for Structure Packing
#endif

typedef char STRING;

//-------------------------------------------------------------------------------------------------------
// Structure types

typedef struct SArray {          // This structure must be parsed to create a structure definition.
                                 // Its function is to send or receive arrays of user defined structures and
                                 // atomic types. Single user defined structures can be passed directly.
    int count;                   // Number of data array elements
    int rank;                    // Rank of the data array
    int* shape;                  // Shape of the data array
    void* data;                  // Location of the Structure Array
    STRING type[MAXELEMENTNAME]; // The Structure Array Element's type name (Must be Unique)
} SARRAY;

typedef struct EnumMember {
    char name[MAXELEMENTNAME]; // The Enumeration member name
    long long value;           // The value of the member
} ENUMMEMBER;

typedef struct EnumList {
    char name[MAXELEMENTNAME];     // The Enumeration name
    int type;                      // The integer base type
    int count;                     // The number of members of this enumeration class
    ENUMMEMBER* enummember;        // Array of enum members
    unsigned long long* enumarray; // Widest integer class to transport all integer type arrays
    int enumarray_rank;
    int enumarray_count;
    int* enumarray_shape;
} ENUMLIST;

typedef struct VLenType { // Variable length (ragged) arrays
    unsigned int len;     // The array element count
    void* data;           // Array data
} VLENTYPE;

typedef struct LogMalloc {
    int count;                 // Number of elements allocated
    int rank;                  // Dimensionality
    size_t size;               // Size of each element allocated
    int freed;                 // Freed flag
    char type[MAXELEMENTNAME]; // The type name (Atomic or User Defined)
    void* heap;                // Heap address of the allocated memory
    int* shape;                // Dimensional lengths. Only required when rank > 1.
} LOGMALLOC;

typedef struct LogMallocList {
    int listcount;        // Number of mallocs logged
    int listsize;         // Size of List
    LOGMALLOC* logmalloc; // List of individual mallocs in allocation order
} LOGMALLOCLIST;

typedef struct LogStruct {
    int id;                    // Structure Identity Number
    char type[MAXELEMENTNAME]; // The structure's type name
    void* heap;                // The structure's Heap address
} LOGSTRUCT;

typedef struct LogStructList {
    int listcount;        // Number of Structures logged
    int listsize;         // Size of List
    LOGSTRUCT* logstruct; // List of individual structures in dispatch order
} LOGSTRUCTLIST;

typedef struct CompoundField {
    int size;                  // The size of the field type: Atomic or User Defined
    int offset;                // Data byte position within the structure (Architecture dependent)
    int offpad;                // Structure Packing before this element's position
    int alignment;             // Alignment Byte Boundary used for Structure Packing
    int atomictype;            // Reference Type ID when atomic
    int pointer;               // Flags this field is a pointer type
    int rank;                  // The rank of the field
    int count;                 // The total count of elements of the field
    int* shape;                // The shape/organisation of the elements of the field
    char type[MAXELEMENTNAME]; // The Element's type name (Atomic or User Defined)
    char name[MAXELEMENTNAME]; // The Element's name (may be different to its type), i.e., variable name
    char desc[MAXELEMENTNAME]; // Description or Comment about the structure element
} COMPOUNDFIELD;

typedef struct UserDefinedType {
    int idamclass;                // IDAM user defined class
    char name[MAXELEMENTNAME];    // Name of the user defined Type (must be unique)
    char source[MAXELEMENTNAME];  // Source of the Structure Definition (header file or data file)
    int imagecount;               // Length of the image blob
    char* image;                  // An textual image of the c code used to define the structure type
    int ref_id;                   // Unique reference ID tag (Database Key etc.)
    int size;                     // The size of the type in local 32/64 bit architecture
    int fieldcount;               // the Number of data fields in the structure
    COMPOUNDFIELD* compoundfield; // Array of field details
} USERDEFINEDTYPE;

typedef struct UserDefinedTypeList {
    int listCount;                    // Count of all User Structured Types
    USERDEFINEDTYPE* userdefinedtype; // Array of User defined types
} USERDEFINEDTYPELIST;

// ***** How many structures are in data (rank, shape, etc?)

typedef struct NTree {         // N-ary Tree linking all related user defined data structures: definitions and data
    int branches;              // Children (Branch) Count from this node
    char name[MAXELEMENTNAME]; // The Structure's and also the tree node's name
    USERDEFINEDTYPE* userdefinedtype; // Definition of the data
    void* data;                       // the Data
    struct NTree* parent;             // Pointer to the parent node
    struct NTree** children;          // Pointer Array of sibling tree nodes (branches).
} NTREE;

typedef struct NTreeList {
    int listCount;  // Count of all Individual Trees
    NTREE* forrest; // Array of Tree list structures
} NTREELIST;

typedef struct GeneralBlock {                 // Generalised Data Structures: Client Side Only
    USERDEFINEDTYPE* userdefinedtype;         // User defined type of the Data
    USERDEFINEDTYPELIST* userdefinedtypelist; // List of Known Structure Definitions
    LOGMALLOCLIST* logmalloclist;             // List of Heap Mallocs
    unsigned int lastMallocIndex;             // Associate last search entry position found in the Malloc Log
} GENERAL_BLOCK;

#define UDA_MALLOC_SOURCE_NONE 0
#define UDA_MALLOC_SOURCE_SOAP 1
#define UDA_MALLOC_SOURCE_DOM 2
#define UDA_MALLOC_SOURCE_NETCDF 3

#define UDA_PACKAGE_XDRFILE 1
#define UDA_PACKAGE_STRUCTDATA 2
#define UDA_PACKAGE_XDROBJECT 3

int xdrAtomicData(LOGMALLOCLIST* logmalloclist, XDR* xdrs, const char* type, int count, int size, char** data);
void printUserDefinedTypeListTable(USERDEFINEDTYPELIST str);
void initSArray(SARRAY* str);

#endif // UDA_STRUCTURES_GENSTRUCTS_H
