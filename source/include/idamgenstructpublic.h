#ifndef IdamGenStructPublicInclude
#define IdamGenStructPublicInclude

// Public definitions of Data Structures used by the General Structure Passing system
//
// Change History
//
// 08Jan2010    D.G.Muir    Original Version
// 23Apr2010    D.G.Muir    Added compiler option REDUCEDCLIENT
// 05May2010    D.G.Muir    Added UVOIDTYPE
// 20Jun2011    dgm         Added ONESTRING type
// 21Sep2011    dgm         Change prototypes from static to extern when macro PLUGINTEST is defined
// 25Oct2012    dgm         Added findIdamNTreeStructureDefinition
// 06Noc2012    dgm         Added lastMallocIndex to GENERAL_BLOCK definition to capture the next search
//                          start index of the Malloc Log
// 20Nov2013    dgm         PLUGINTEST selections commented out
// 15Aug2014    dgm         findNTreeChildStructure, findNTreeChildStructureComponent added
//-------------------------------------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

//-------------------------------------------------------------------------------------------------------
// Macro definitions

#define MAXELEMENTS         256 // Max number of structure elements
#define MAXRANK             7   // Max rank of arrays within structure
#define MAXELEMENTNAME      256 // structure element name
#define MAXSOAPSTACKSTRING  256 // SOAP strings allocated on the stack must be less than this size

#define GROWMALLOCLIST      20  // Each time realloc is called, increase the heap by this number of array elements

#define POINTER_SIZE32      4   // Pointer size (bytes) on 32-bit architecture

#define MAXRECURSIVEDEPTH   30

#define SCALARDOUBLE    1
#define ARRAYDOUBLE     2
#define SCALARFLOAT     3
#define ARRAYFLOAT      4
#define SCALARLONG64    5
#define ARRAYLONG64     6
#define SCALARULONG64   7
#define ARRAYULONG64    8
#define SCALARINT       9
#define ARRAYINT        10
#define SCALARUINT      11
#define ARRAYUINT       12
#define SCALARSHORT     13
#define ARRAYSHORT      14
#define SCALARUSHORT    15
#define ARRAYUSHORT     16
#define SCALARCHAR      17
#define ARRAYCHAR       18
#define SCALARSTRING    19
#define ARRAYSTRING     20

#ifdef A64
typedef long long VOIDTYPE;   // Pointer arithmentic type for 64 bit architecture (8 byte pointer type)
typedef unsigned long long UVOIDTYPE;
#define ALIGNMENT           1           // Default Byte Boundary used for Structure Packing
#else
typedef int             VOIDTYPE;       // Pointer arithmentic type for 32 bit architecture (4 byte pointer type)
typedef unsigned int    UVOIDTYPE;
#define ALIGNMENT       1               // Default Byte Boundary used for Structure Packing
#endif

typedef char STRING;

//-------------------------------------------------------------------------------------------------------
// Structure types

struct TWOSTRINGS {
    char* s1;
    char* s2;
};
typedef struct TWOSTRINGS TWOSTRINGS;

struct TWOSTRINGS2 {
    STRING* s1;
    STRING* s2;
};
typedef struct TWOSTRINGS2 TWOSTRINGS2;

struct ONESTRING {
    STRING* string;
};
typedef struct ONESTRING ONESTRING;

struct SARRAY {                     // This structure must be parsed to create a structure definition.
                                    // Its function is to send or receive arrays of user defined structures and
                                    // atomic types. Single user defined structures can be passed directly.
    int count;                      // Number of data array elements
    int rank;                       // Rank of the data array
    int* shape;                     // Shape of the data array
    void* data;                     // Location of the Structure Array
    STRING type[MAXELEMENTNAME];    // The Structure Array Element's type name (Must be Unique)
};
typedef struct SARRAY SARRAY;

struct ENUMMEMBER {
    STRING name[MAXELEMENTNAME];    // The Enumeration member name
    long long value;                // The value of the member
};
typedef struct ENUMMEMBER ENUMMEMBER;

struct ENUMLIST {
    STRING name[MAXELEMENTNAME];    // The Enumeration name
    int type;                       // The integer base type
    int count;                      // The number of members of this enumeration class
    ENUMMEMBER* enummember;         // Array of enum members
    void* data;                     // Data with this enumerated type (properties are held by regular DATA_BLOCK structure)
};
typedef struct ENUMLIST ENUMLIST;

struct VLENTYPE {                   // Variable length (ragged) arrays
    unsigned int len;               // The array element count
    void* data;                     // Array data
};
typedef struct VLENTYPE VLENTYPE;

struct LOGMALLOC {
    int count;                      // Number of elements allocated
    int rank;                       // Dimensionality
    int size;                       // Size of each element allocated
    int freed;                      // Freed flag
    char type[MAXELEMENTNAME];      // The type name (Atomic or User Defined)
    void* heap;                     // Heap address of the allocated memory
    int* shape;                     // Dimensional lengths. Only required when rank > 1.
};
typedef struct LOGMALLOC LOGMALLOC;

struct LOGMALLOCLIST {
    int listcount;                  // Number of mallocs logged
    int listsize;                   // Size of List
    LOGMALLOC* logmalloc;           // List of individual mallocs in allocation order
};
typedef struct LOGMALLOCLIST LOGMALLOCLIST;

struct LOGSTRUCT {
    int id;                         // Structure Identity Number
    char type[MAXELEMENTNAME];      // The structure's type name
    void* heap;                     // The structure's Heap address
};
typedef struct LOGSTRUCT LOGSTRUCT;

struct LOGSTRUCTLIST {
    int listcount;                  // Number of Structures logged
    int listsize;                   // Size of List
    LOGSTRUCT* logstruct;           // List of individual structures in dispatch order
};
typedef struct LOGSTRUCTLIST LOGSTRUCTLIST;

struct COMPOUNDFIELD {
    int size;                       // The size of the field type: Atomic or User Defined
    int offset;                     // Data byte position within the structure (Architecture dependent)
    int offpad;                     // Structure Packing before this element's position
    int alignment;                  // Alignment Byte Boundary used for Structure Packing
    int atomictype;                 // Reference Type ID when atomic
    int pointer;                    // Flags this field is a pointer type
    int rank;                       // The rank of the field
    int count;                      // The total count of elements of the field
    int* shape;                     // The shape/organisation of the elements of the field
    char type[MAXELEMENTNAME];      // The Element's type name (Atomic or User Defined)
    char name[MAXELEMENTNAME];      // The Element's name (may be different to its type), i.e., variable name
    char desc[MAXELEMENTNAME];      // Description or Comment about the structure element
};
typedef struct COMPOUNDFIELD COMPOUNDFIELD;

struct USERDEFINEDTYPE {
    int idamclass;                  // IDAM user defined class
    char name[MAXELEMENTNAME];      // Name of the user defined Type (must be unique)
    char source[MAXELEMENTNAME];    // Source of the Structure Definition (header file or data file)
    int imagecount;                 // Length of the image blob
    char* image;                    // An textual image of the c code used to define the structure type
    int ref_id;                     // Unique reference ID tag (Database Key etc.)
    int size;                       // The size of the type in local 32/64 bit architecture
    int fieldcount;                 // the Number of data fields in the structure
    COMPOUNDFIELD* compoundfield;   // Array of field details
};
typedef struct USERDEFINEDTYPE USERDEFINEDTYPE;

struct USERDEFINEDTYPELIST {
    int listCount;                      // Count of all User Structured Types
    USERDEFINEDTYPE* userdefinedtype;   // Array of User defined types
};
typedef struct USERDEFINEDTYPELIST USERDEFINEDTYPELIST;

// ***** How many structures are in data (rank, shape, etc?)

struct NTREE {                          // N-ary Tree linking all related user defined data structures: definitions and data
    int branches;                       // Children (Branch) Count from this node
    char name[MAXELEMENTNAME];          // The Structure's and also the tree node's name
    USERDEFINEDTYPE* userdefinedtype;   // Definition of the data
    void* data;                         // the Data
    struct NTREE* parent;               // Pointer to the parent node
    struct NTREE** children;            // Pointer Array of sibling tree nodes (branches).
};
typedef struct NTREE NTREE;

struct NTREELIST {
    int listCount;              // Count of all Individual Trees
    NTREE* forrest;             // Array of Tree list structures
};
typedef struct NTREELIST NTREELIST;

struct GENERAL_BLOCK {                          // Generalised Data Structures: Client Side Only
    USERDEFINEDTYPE* userdefinedtype;           // User defined type of the Data
    USERDEFINEDTYPELIST* userdefinedtypelist;   // List of Known Structure Definitions
    LOGMALLOCLIST* logmalloclist;               // List of Heap Mallocs
    unsigned int lastMallocIndex;               // Associate last search entry position found in the Malloc Log
};
typedef struct GENERAL_BLOCK GENERAL_BLOCK;

#ifdef __cplusplus
}
#endif

#endif // IdamGenStructPublicInclude
