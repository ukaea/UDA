#ifndef UDA_STRUCTURES_GENSTRUCTS_H
#define UDA_STRUCTURES_GENSTRUCTS_H

// needs to be included before xdr.h
#include <stdio.h>

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

typedef struct CNTree {} NTREE;
typedef struct CNTreeList {} NTREELIST;
typedef struct CLogMallocList {} LOGMALLOCLIST;
typedef struct CLogMalloc {} LOGMALLOC;
typedef struct CLogStruct {} LOGSTRUCT;
typedef struct CLogStructList {} LOGSTRUCTLIST;
typedef struct CCompoundField {} COMPOUNDFIELD;
typedef struct CUserDefinedType {} USERDEFINEDTYPE;
typedef struct CUserDefinedTypeList {} USERDEFINEDTYPELIST;

typedef char String;

//-------------------------------------------------------------------------------------------------------
// Structure types

struct SArray {          // This structure must be parsed to create a structure definition.
                                 // Its function is to send or receive arrays of user defined structures and
                                 // atomic types. Single user defined structures can be passed directly.
    int count;                   // Number of data array elements
    int rank;                    // Rank of the data array
    int* shape;                  // Shape of the data array
    void* data;                  // Location of the Structure Array
    String type[MAXELEMENTNAME]; // The Structure Array Element's type name (Must be Unique)
};

struct EnumMember {
    char name[MAXELEMENTNAME]; // The Enumeration member name
    long long value;           // The value of the member
};

struct EnumList {
    char name[MAXELEMENTNAME];     // The Enumeration name
    int type;                      // The integer base type
    int count;                     // The number of members of this enumeration class
    EnumMember* enummember;        // Array of enum members
    unsigned long long* enumarray; // Widest integer class to transport all integer type arrays
    int enumarray_rank;
    int enumarray_count;
    int* enumarray_shape;
};

struct VLenType { // Variable length (ragged) arrays
    unsigned int len;     // The array element count
    void* data;           // Array data
};

struct LogMalloc {
    int count;                 // Number of elements allocated
    int rank;                  // Dimensionality
    size_t size;               // Size of each element allocated
    int freed;                 // Freed flag
    char type[MAXELEMENTNAME]; // The type name (Atomic or User Defined)
    void* heap;                // Heap address of the allocated memory
    int* shape;                // Dimensional lengths. Only required when rank > 1.
};

struct LogMallocList : LOGMALLOCLIST {
    int listcount;        // Number of mallocs logged
    int listsize;         // Size of List
    LogMalloc* logmalloc; // List of individual mallocs in allocation order
};

struct LogStruct {
    int id;                    // Structure Identity Number
    char type[MAXELEMENTNAME]; // The structure's type name
    void* heap;                // The structure's Heap address
};

struct LogStructList : LOGSTRUCTLIST {
    int listcount;        // Number of Structures logged
    int listsize;         // Size of List
    LogStruct* logstruct; // List of individual structures in dispatch order
};

struct CompoundField : COMPOUNDFIELD {
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
};

struct UserDefinedType : USERDEFINEDTYPE {
    int idamclass;                // IDAM user defined class
    char name[MAXELEMENTNAME];    // Name of the user defined Type (must be unique)
    char source[MAXELEMENTNAME];  // Source of the Structure Definition (header file or data file)
    int imagecount;               // Length of the image blob
    char* image;                  // An textual image of the c code used to define the structure type
    int ref_id;                   // Unique reference ID tag (Database Key etc.)
    int size;                     // The size of the type in local 32/64 bit architecture
    int fieldcount;               // the Number of data fields in the structure
    CompoundField* compoundfield; // Array of field details
};

struct UserDefinedTypeList : USERDEFINEDTYPELIST {
    int listCount;                    // Count of all User Structured Types
    UserDefinedType* userdefinedtype; // Array of User defined types
};

// ***** How many structures are in data (rank, shape, etc?)

struct NTree : NTREE {         // N-ary Tree linking all related user defined data structures: definitions and data
    int branches;              // Children (Branch) Count from this node
    char name[MAXELEMENTNAME]; // The Structure's and also the tree node's name
    UserDefinedType* userdefinedtype; // Definition of the data
    void* data;                       // the Data
    struct NTree* parent;             // Pointer to the parent node
    struct NTree** children;          // Pointer Array of sibling tree nodes (branches).
};

struct NTreeList : NTREELIST {
    int listCount;  // Count of all Individual Trees
    NTree* forrest; // Array of Tree list structures
};

struct GeneralBlock {                 // Generalised Data Structures: Client Side Only
    UserDefinedType* userdefinedtype;         // User defined type of the Data
    UserDefinedTypeList* userdefinedtypelist; // List of Known Structure Definitions
    LogMallocList* logmalloclist;             // List of Heap Mallocs
    unsigned int lastMallocIndex;             // Associate last search entry position found in the Malloc Log
};

#define UDA_MALLOC_SOURCE_NONE 0
#define UDA_MALLOC_SOURCE_SOAP 1
#define UDA_MALLOC_SOURCE_DOM 2
#define UDA_MALLOC_SOURCE_NETCDF 3

#define UDA_PACKAGE_XDRFILE 1
#define UDA_PACKAGE_STRUCTDATA 2
#define UDA_PACKAGE_XDROBJECT 3

int xdrAtomicData(LogMallocList* logmalloclist, XDR* xdrs, const char* type, int count, int size, char** data);
void print_user_defined_type_list_table(UserDefinedTypeList str);
void initSArray(SArray* str);
void initNTree(NTree* str);
void copy_user_defined_type_list(UserDefinedTypeList** anew, const UserDefinedTypeList* parseduserdefinedtypelist);
void get_initial_user_defined_type_list(UserDefinedTypeList** anew);

#endif // UDA_STRUCTURES_GENSTRUCTS_H
