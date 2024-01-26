#ifndef UDA_STRUCTURES_STRUCT_H
#define UDA_STRUCTURES_STRUCT_H

#include <rpc/rpc.h>
#include <stdbool.h>
#include <stdio.h>

#include "export.h"
#include "genStructs.h"
#include "udaTypes.h"

#ifdef SERVERBUILD
#  include <server/udaServer.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

LIBRARY_API NTREE* udaGetFullNTree();
LIBRARY_API void udaSetFullNTree(NTREE* ntree);

LIBRARY_API void udaSetLastMallocIndexValue(unsigned int* lastMallocIndexValue_in);

/** Initialise a SARRAY data structure.
 *
 * @param str A pointer to a SARRAY data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitSArray(SARRAY* str);

/** Print the Contents of a SARRAY data structure.
 *
 * @param fd A File Descriptor.
 * @param str A SARRAY data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintSarray(SARRAY str);

/** Add an NTREE List entry.
 *
 * @param node A NTREE node to add.
 * @return void.
 */
LIBRARY_API void udaAddNTreeList(LOGMALLOCLIST* logmalloclist, NTREE* node, NTREELIST* ntree_list);

/** Add an NTREE node to an array of child nodes.
 *
 * @param parent A NTREE node with a set of child nodes
 * @param child A NTREE node to add to the existing set of child nodes
 * @return void.
 */
LIBRARY_API void udaAddNTree(NTREE* parent, NTREE* child);

/** Free an NTREE node together with the array of child nodes.
 *
 * @param ntree A NTREE node with or without a set of child nodes
 * @return void.
 */
LIBRARY_API void udaFreeNTreeNode(NTREE* ntree);

/** Add a new image line to the existing image.
 *
 * @param image A block of bytes used to record structure definition image data.
 * @param imagecount The current count of bytes used to record the present image.
 * @param line A new image line to add to the existing image.
 * @return Both image and image count are updated on return.
 */
LIBRARY_API void udaAddImage(char** image, int* imagecount, const char* line);

/** Expand an image line that contains header defines and include the numerical value
 *
 * @param buffer An image line to be expanded
 * @param defnames An array of define names
 * @param defvalues An array of define values
 * @param defCount The number of define names and values
 * @param expand A pre-allocated array of char to be used to receive the expanded buffer string.
 * @return expand An expanded Image line.
 */
LIBRARY_API void udaExpandImage(char* buffer, char defnames[MAXELEMENTS][MAXELEMENTNAME], int* defvalues, int defCount,
                                char* expand);

/** Initialise a LOGMALLOCLIST data structure.
 *
 * @param str A pointer to a LOGMALLOCLIST data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitLogMallocList(LOGMALLOCLIST* str);

/** Initialise a LOGMALLOC data structure.
 *
 * @param str A pointer to a LOGMALLOC data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitLogMalloc(LOGMALLOC* str);

/** Initialise a LOGSTRUCTLIST data structure.
 *
 * @return void.
 */
LIBRARY_API void udaInitLogStructList(LOGSTRUCTLIST* logstructlist);

/** Initialise a LOGSTRUCT data structure.
 *
 * @param str A pointer to a LOGSTRUCT data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitLogStruct(LOGSTRUCT* str);

/** Initialise a COMPOUNDFIELD data structure.
 *
 * @param str A pointer to a COMPOUNDFIELD data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitCompoundField(COMPOUNDFIELD* str);

/** Initialise a USERDEFINEDTYPE data structure.
 *
 * @param str A pointer to a USERDEFINEDTYPE data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitUserDefinedType(USERDEFINEDTYPE* str);

/** Initialise a USERDEFINEDTYPELIST data structure.
 *
 * @param str A pointer to a USERDEFINEDTYPELIST data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitUserDefinedTypeList(USERDEFINEDTYPELIST* str);

/** Initialise a GENERAL_BLOCK data structure.
 *
 * @param str A pointer to a GENERAL_BLOCK data structure instance.
 * @return void.
 */
LIBRARY_API void udaInitGeneralBlock(GENERAL_BLOCK* str);

/** Print the Contents of a COMPOUNDFIELD data structure.
 *
 * @param fd A File Descriptor.
 * @param str A COMPOUNDFIELD data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintCompoundField(COMPOUNDFIELD str);

/** Print the Tabulated Contents of a COMPOUNDFIELD data structure.
 *
 * @param fd A File Descriptor.
 * @param str A COMPOUNDFIELD data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintCompoundFieldTable(COMPOUNDFIELD str);

/** Print the Contents of a USERDEFINEDTYPE data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintUserDefinedType(USERDEFINEDTYPE str);

/** Print the Tabulated Contents of a USERDEFINEDTYPE data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintUserDefinedTypeTable(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE str);

/** Print the Tabulated Contents of a USERDEFINEDTYPE data structure with Zero Sized elements.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPE data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintZeroSizedUserDefinedTypeTable(USERDEFINEDTYPE str);

/** Print the Contents of a USERDEFINEDTYPELIST data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintUserDefinedTypeList(USERDEFINEDTYPELIST str);

/** Print the Tabulated Contents of a USERDEFINEDTYPELIST data structure.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintUserDefinedTypeListTable(USERDEFINEDTYPELIST str);

/** Print the Tabulated Contents of a USERDEFINEDTYPELIST data structure where the size is zero.
 *
 * @param fd A File Descriptor.
 * @param str A USERDEFINEDTYPELIST data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintZeroSizedUserDefinedTypeListTable(USERDEFINEDTYPELIST str);

/** Print the Contents of a LOGMALLOC data structure.
 *
 * @param fd A File Descriptor.
 * @param str A LOGMALLOC data structure instance.
 * @return void.
 */
LIBRARY_API void udaPrintMallocLog(LOGMALLOC str);

/** Print the Contents of the Global LOGMALLOCLIST data structure.
 *
 * @param fd A File Descriptor.
 * @return void.
 */
LIBRARY_API void udaPrintMallocLogList(const LOGMALLOCLIST* logmalloclist);

//==============================================================================================================
// Utility Functions

/** Add a stack memory location to the LOGMALLOCLIST data structure. These are not freed.
 *
 * @param stack The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
LIBRARY_API void udaAddNonMalloc(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type);

/** Add a stack memory location to the LOGMALLOCLIST data structure. These are not freed.
*
* @param stack The memory location.
* @param count The number of elements allocated.
* @param size The size of a single element.
* @param type The name of the type allocated.
* @param rank The rank of the allocated array.
* @param shape The shape of the allocated array. Only required when rank > 1.

* @return void.
*/
LIBRARY_API void udaAddNonMalloc2(LOGMALLOCLIST* logmalloclist, void* stack, int count, size_t size, const char* type,
                                  int rank, int* shape);

/** Add a heap memory location to the LOGMALLOCLIST data structure. These are freed.
 *
 * @param heap The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
LIBRARY_API void udaAddMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type);

/** Add a heap memory location to the LOGMALLOCLIST data structure. These are freed.
 *
 * @param heap The memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @param rank The rank of the allocated array.
 * @param shape The shape of the allocated array. Only required when rank > 1.
 * @return void.
 */
LIBRARY_API void udaAddMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int count, size_t size, const char* type,
                               int rank, int* shape);

/** Change the logged memory location to a new location (necessary with realloc).
 *
 * @param old The original logged memory location.
 * @param anew The new replacement memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
LIBRARY_API void udaChangeMalloc(LOGMALLOCLIST* logmalloclist, VOIDTYPE old, void* anew, int count, size_t size,
                                 const char* type);

/** Change the logged memory location to a new location (necessary with realloc).
 *
 * @param old The original logged memory location.
 * @param anew The new replacement memory location.
 * @param count The number of elements allocated.
 * @param size The size of a single element.
 * @param type The name of the type allocated.
 * @return void.
 */
LIBRARY_API void udaChangeNonMalloc(LOGMALLOCLIST* logmalloclist, void* old, void* anew, int count, size_t size,
                                    const char* type);

LIBRARY_API int udaDupCountMallocLog(LOGMALLOCLIST* str);

/** Free allocated heap memory but preserve the addresses. There are no arguments.
 *
 * @return void.
 */
LIBRARY_API void udaFreeMallocLog(LOGMALLOCLIST* str);

/** Free allocated heap memory and reinitialise a new logmalloclist-> There are no arguments.
 *
 * @return void.
 */
LIBRARY_API void udaFreeMallocLogList(LOGMALLOCLIST* str);

/** Find the meta data associated with a specific memory location.
 *
 * @param heap The target memory location.
 * @param count The returned allocation count.
 * @param size The returned allocation size.
 * @param type The returned allocation type.
 * @return void.
 */
LIBRARY_API void udaFindMalloc(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type);

/** Find the meta data associated with a specific memory location.
*
* @param heap The target memory location.
* @param count The returned allocation count.
* @param size The returned allocation size.
* @param type The returned allocation type.
* @param rank The returned rank of the allocated array.
* @param shape The returned shape of the allocated array. Only given when rank > 1.

* @return void.
*/
LIBRARY_API void udaFindMalloc2(LOGMALLOCLIST* logmalloclist, void* heap, int* count, int* size, const char** type,
                                int* rank, int** shape);

/** Add a heap memory location to the LOGSTRUCTLIST data structure. These are freed.
 *
 * @param heap The memory location.
 * @param type The name of the type allocated.
 * @return void.
 */
LIBRARY_API void udaAddStruct(void* heap, const char* type, LOGSTRUCTLIST* log_struct_list);

/** Free allocated heap memory and reinitialise a new LOGSTRUCTLIST. There are no arguments.
 *
 * @return void.
 */
LIBRARY_API void udaFreeLogStructList(LOGSTRUCTLIST* log_struct_list);

/** Find the meta data associated with a specific Structure.
 *
 * @param heap The target memory location.
 * @param type The returned structure type.
 * @return The structure id.
 */
LIBRARY_API int udaFindStructId(void* heap, char** type, LOGSTRUCTLIST* log_struct_list);

/** Find the Heap address and Data Type of a specific Structure.
 *
 * @param id The structure id.
 * @param type The returned structure type.
 * @return The heap memory location
 */
LIBRARY_API void* udaFindStructHeap(int id, char** type, LOGSTRUCTLIST* log_struct_list);

/** Copy a User Defined Structure Definition.
 *
 * @param old The type definition to be copied.
 * @param anew The copy of the type definition.
 * @return void.
 */
LIBRARY_API void udaCopyUserDefinedType(USERDEFINEDTYPE* old, USERDEFINEDTYPE* anew);

/** Copy the Master User Defined Structure Definition List.
 *
 * @param anew The copy of the type definition list.
 * @return void.
 */
LIBRARY_API void udaCopyUserDefinedTypeList(USERDEFINEDTYPELIST** anew,
                                            const USERDEFINEDTYPELIST* parseduserdefinedtypelist);

/** Create the Initial User Defined Structure Definition List.
 *
 * @param anew The initial type definition list.
 * @return void.
 */
LIBRARY_API void udaGetInitialUserDefinedTypeList(USERDEFINEDTYPELIST** anew);

/** Add a Compound Field type to a structure definition.
 *
 * @param str The structure definition.
 * @param field The Compound field type.
 * @return void.
 */
LIBRARY_API void udaAddCompoundField(USERDEFINEDTYPE* str, COMPOUNDFIELD field);

/** Add a structure definition to the List of structure types
 *
 * @param str The list of structure definitions.
 * @param type The new definition to add to the list.
 * @return void.
 */
LIBRARY_API void udaAddUserDefinedType(USERDEFINEDTYPELIST* str, USERDEFINEDTYPE type);

/** Replace/Update the structure definition list with an different structure type.
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be replaced/updated
 * @param type The definition to add into the list.
 * @return void.
 */
LIBRARY_API void udaUpdateUserDefinedType(USERDEFINEDTYPELIST* str, int typeId, USERDEFINEDTYPE type);

/** Change a structure element's property in the structure definition
 *
 * @param str The list of structure definitions.
 * @param typeId The definition list entry to be modified
 * @param element The structure element to be modified
 * @param property The structure element's definition property to be modified
 * @param value The new property value
 * @return void.
 */
LIBRARY_API void udaChangeUserDefinedTypeElementProperty(USERDEFINEDTYPELIST* str, int typeId, char* element,
                                                         char* property, void* value);

/** The number of Structure Definitions or User Defined Types in the structure list
 *
 * @param str The list of structure definitions.
 * @return The count of structured types.
 */
LIBRARY_API int udaCountUserDefinedType(USERDEFINEDTYPELIST* str);

/** Free heap from a Compound Field.
 *
 * @param str The Compound Field.
 * @return void.
 */
LIBRARY_API void udaFreeCompoundField(COMPOUNDFIELD* str);

/** Free heap from a User Defined Type.
 *
 * @param type The User Defined Type.
 * @return void.
 */
LIBRARY_API void udaFreeUserDefinedType(USERDEFINEDTYPE* type);

/** Free heap from a User Defined Type List.
 *
 * @param userdefinedtypelist The User Defined Type List.
 * @return void.
 */
LIBRARY_API void udaFreeUserDefinedTypeList(USERDEFINEDTYPELIST* userdefinedtypelist);

/** The size or byte count of an atomic or structured type
 *
 * @param type The name of the type
 * @return The size in bytes.
 */
LIBRARY_API size_t udaGetsizeof(USERDEFINEDTYPELIST* userdefinedtypelist, const char* type);

/** The value of the IDAM enumeration type for a named regular atomic type
 *
 * @param type The name of the atomic type
 * @return The integer value of the corresponding IDAM enumeration.
 */
LIBRARY_API int udaGettypeof(const char* type);

/** Return structure element alignment byte boundary
 *
 * @param type The name of the structure atomic type.
 * @return number of bytes to align with.
 *
 * Alignment rules are Architecture Dependent:
 * Single byte numbers are aligned at a single byte boundary
 * Two byte numbers are aligned with two byte boundaries
 * Four byte numbers are aligned on four byte boundaries
 * Eight byte numbers are aligned on four byte boundaries if 32 bit architecture and Linux
 * or 8 byte boundary if 64 bit or 32 bit and Windows.
 * Structures are aligned depending on whichever element has the largest alignment boundary.
 *
 * Structures between 1 and 4 bytes of data should be padded so that the total structure is 4 bytes.
 * Structures between 5 and 8 bytes of data should be padded so that the total structure is 8 bytes.
 * Structures between 9 and 16 bytes of data should be padded so that the total structure is 16 bytes.
 * Structures greater than 16 bytes should be padded to 16 byte boundary.
 *
 * Indexing into arrays can be speeded up by making the structure size a power of 2. The compiler can then replace the
 * multiply (entry = base address + index*size of structure) with a simple shift operation.
 *
 *
 */
LIBRARY_API int udaGetalignmentof(const char* type);

LIBRARY_API size_t udaNewoffset(size_t offset, const char* type);

LIBRARY_API size_t udaPadding(size_t offset, const char* type);

/** The name of an atomic type corresponding to a value of the IDAM enumeration type.
 *
 * @param type The integer value of the type enumeration.
 * @return The name of the atomic type.
 */
LIBRARY_API const char* udaNameType(UDA_TYPE type);

/** The size or byte count of a user defined structured type.
 *
 * @param str The user defined structure definition.
 * @return The size in bytes.
 */
LIBRARY_API size_t udaGetStructureSize(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* str);

/** Print an error message.
 *
 * @param fd The output file descriptor pointer.
 * @param warning Print a warning message rather than an error message.
 * @param line The line number where the error occured.
 * @param file The file name where the error occured.
 * @param msg The message to print.
 * @return The size in bytes.
 */
LIBRARY_API void udaPrintError(int warning, int line, char* file, char* msg);

//==============================================================================================================
// Functions to Send or Receive Data contained in User Defined Structures

// Send or Receive the Data Structure

// Recursive Send/Receive Individual User Defined Structure Elements
// Structure elements may be primitive (atomic) types or other User defined types
// The structure Size has two possible sizes if it contains pointers.
// Pointer size changes from 4 to 8 bytes if hardware architecture changes from 32 to 64 bits.
// Hardware architecture also affects offset values.
// All atomic types have standard byte lengths.
// Assume 32 bit architecture as the default and pass additional values for 64 bit systems
// The type, length and size of pointer types is passed prior to sending.
// Pointer type, length and size are recorded in a malloc log.
// The count of data structures to be received is passed ...
//

LIBRARY_API int udaXDRAtomicData(LOGMALLOCLIST* logmalloclist, XDR* xdrs, const char* type, int count, int size,
                                 char** data);

// Send/Receive Array of Structures

LIBRARY_API int udaXDRUserDefinedTypeData(XDR* xdrs, LOGMALLOCLIST* logmalloclist,
                                          USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* userdefinedtype,
                                          void** data, int protocolVersion, bool xdr_stdio_flag,
                                          LOGSTRUCTLIST* log_struct_list, int malloc_source);

LIBRARY_API int udaFindUserDefinedTypeId(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name);

LIBRARY_API USERDEFINEDTYPE* udaFindUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, const char* name,
                                                    int ref_id);

LIBRARY_API int udaTestUserDefinedType(USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* udt);

//==============================================================================================================
// Functions to Send or Receive Definitions of User Defined Structure

LIBRARY_API bool_t udaXDRCompoundfield(XDR* xdrs, COMPOUNDFIELD* str);

LIBRARY_API bool_t udaXDRUserdefinedtype(XDR* xdrs, USERDEFINEDTYPELIST* userdefinedtypelist, USERDEFINEDTYPE* str);

LIBRARY_API bool_t udaXDRUserdefinedtypelist(XDR* xdrs, USERDEFINEDTYPELIST* str, bool xdr_stdio_flag);

//----------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------
// Client Utility functions operating on Linked List N-Tree

/** Print the data from an array of Atomic Type to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param data A pointer to the data.
 * @param atomictype The name of a Atomic type.
 * @param count The array element count.
 * @param label A label to print before the value.
 * @return void
 */
LIBRARY_API void udaPrintAtomicData(void* data, int atomictype, int count, const char* label);

/** Print the data from a named array of Atomic Type from a given tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node.
 * @param target The name of a User Defined Structure type.
 * @return void
 *
 * \todo {When the structure is an array, either print data from a single array element or print data from
 * all structure elements}
 */
LIBRARY_API void udaPrintAtomicType(LOGMALLOCLIST* logmalloclist, NTREE* tree, const char* target);

/** Print the Count of elements of a named data array from a given tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param ntree A pointer to a tree node.
 * @param target The name of a Structure element.
 * @return void
 */
LIBRARY_API void udaPrintTypeCount(NTREE* ntree, const char* target);

// The compound field element of the structure definition contains the count, rank and shape details
// of non-pointer data.
// Pointer data have a count from the malloc log.

//---------------------------------------------------------------------------------------------
// Node Data Structure Component

/** Return a pointer to a User Defined Structure Component Structure.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a Structure Element.
 * @return the Structure Element Definition Structure.
 */
COMPOUNDFIELD* getNodeStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

//---------------------------------------------------------------------------------------------
// Tree Node Family: Single tree node is in scope

/** Return a Pointer to a string array containing the hierarchical names of structure components.
 *
 * @param target The name of a User Defined Structure element (case sensitive).
 * @param ntargets A returned count of the number of names in the returned list.
 * @return the list of structure component names.
 */
LIBRARY_API char** udaParseTarget(const char* target, int* ntargets);

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */
LIBRARY_API void udaPrintNode(NTREE* tree);

/** Print the Contents of a tree node with the specified User Defined Structure name to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param target The name of a User Defined Structure type. If an null string is passed, the structure
 *        of the root node is used.
 * @return void
 */
LIBRARY_API void udaPrintNodeStructureDefinition(const char* target);

/** Print an Image of the Named Structure Definition to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param target The name of a User Defined Structure type.
 * @return void
 */
LIBRARY_API void udaPrintNodeStructureImage(const char* target);

/** Return a Pointer to the User Defined Type Structure of the data attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the User Defined Type Structure Definition.
 */
LIBRARY_API USERDEFINEDTYPE* udaGetNodeUserDefinedType(NTREE* ntree);

/** Return the name of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
LIBRARY_API char* udaGetNodeStructureName(NTREE* ntree);

/** Return the Type of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Type of the User Defined Type Structure.
 */
LIBRARY_API char* udaGetNodeStructureType(NTREE* ntree);

/** Return the Size of the User Defined Type Structure.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Size (Bytes) of the User Defined Type Structure.
 */
LIBRARY_API int udaGetNodeStructureSize(NTREE* ntree);

/** Return a pointer to a Tree Nodes's Data Structure Array element.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param index The array index
 * @return a Pointer to a Structure Array element.
 */
LIBRARY_API void* udaGetNodeStructureArrayData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, int index);

/** Return a pointer to a Component Data Structure Array element.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of the Structure Array element.
 * @param structureindex The Array index
 * @param componentindex The structure element index
 * @return a Pointer to a Component Structure Array element.
 */
LIBRARY_API void* udaGetNodeStructureComponentArrayData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
                                                        int structureindex, int componentindex);

/** Return the count of child User Defined Type Structures (elements of this structure).
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the name of the User Defined Type Structure.
 */
LIBRARY_API int udaGetNodeChildrenCount(NTREE* ntree);

/** Return a Child Node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param child A integer index identifying which child from the child array to return
 * @return the Child Node.
 */
LIBRARY_API NTREE* udaGetNodeChild(NTREE* ntree, int child);

/** Return a Child Node'd ID (Branch index value).
 *
 * @param ntree A pointer to a Parent tree node. If NULL the root node is assumed.
 * @param child A ipointer to a Child tree node.
 * @return the Child Node's ID.
 */
LIBRARY_API int udaGetNodeChildId(NTREE* ntree, NTREE* child);

/** Return a Pointer to the children of this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Array of children.
 */
LIBRARY_API NTREE** udaGetNodeChildren(NTREE* ntree);

/** Return the parent Node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Parent Node.
 */
LIBRARY_API NTREE* udaGetNodeParent(NTREE* ntree);

/** Return the Data pointer.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return a Pointer to the Data.
 */
LIBRARY_API void* udaGetNodeData(NTREE* ntree);

/** Return a Count of Structured Component Types attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Count of Structured types.
 */

LIBRARY_API int udaGetNodeStructureCount(NTREE* ntree);

/** Return a Count of Atomic Component Types attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Count of Atomic types.
 */

LIBRARY_API int udaGetNodeAtomicCount(NTREE* ntree);

/** Return a List of Structure component Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Structure names.
 */

LIBRARY_API char** udaGetNodeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Atomic component Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Atomic element names.
 */
LIBRARY_API char** udaGetNodeAtomicNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Structure Component Type Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Structure Type names.
 */
LIBRARY_API char** udaGetNodeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Atomic Component Type Names attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Atomic Type names.
 */
LIBRARY_API char** udaGetNodeAtomicTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Structure Component Pointer property attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Structure Pointer Properties.
 */
LIBRARY_API int* udaGetNodeStructurePointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Atomic Component Pointer property attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Atomic Pointer Properties.
 */
LIBRARY_API int* udaGetNodeAtomicPointers(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Rank values of the Structure Components attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Structure Ranks.
 */
LIBRARY_API int* udaGetNodeStructureRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Rank values of the Atomic Components attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Atomic Ranks.
 */
LIBRARY_API int* udaGetNodeAtomicRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Shape Arrays of the Structure Components attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Structure Shape Arrays.
 */
LIBRARY_API int** udaGetNodeStructureShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of Shape Arrays of the Atomic Components attached to a tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of Atomic Shape Arrays.
 */

LIBRARY_API int** udaGetNodeAtomicShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Print the Names and Types of all Node Data Elements to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */

LIBRARY_API void udaPrintNodeNames(LOGMALLOCLIST* logmalloclist, NTREE* tree);

/** Print the Atomic Data from a data node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */

LIBRARY_API void udaPrintNodeAtomic(LOGMALLOCLIST* logmalloclist, NTREE* tree);

/** Return the number of User Defined Type Structure Definition Components attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
LIBRARY_API int udaGetNodeStructureComponentCount(NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
LIBRARY_API char** udaGetNodeStructureComponentNames(NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
LIBRARY_API char** udaGetNodeStructureComponentTypes(NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
LIBRARY_API char** udaGetNodeStructureComponentDescriptions(NTREE* ntree);

/** Return the Count of User Defined Structure Component Data array elements attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Count of User Defined Structure Component Data Array elements.
 */

LIBRARY_API int udaGetNodeStructureComponentDataCount(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

/** Return the Rank of User Defined Structure Component Data array attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Rank of User Defined Structure Component Data array.
 */

LIBRARY_API int udaGetNodeStructureComponentDataRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

/** Return the Shape array of the User Defined Structure Component Data array attached to this tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the Shape array of length Rank of the User Defined Structure Component Data array.
 */

LIBRARY_API int* udaGetNodeStructureComponentDataShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

/** Return True (1) if the User Defined Structure Component Data array, attached to this tree node,
 * is a pointer type. Returns False (0) otherwise.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure definition.
 * @return the value 1 if the User Defined Structure Component Data array is a pointer type.
 */

LIBRARY_API int udaGetNodeStructureComponentDataIsPointer(LOGMALLOCLIST* logmalloclist, NTREE* ntree,
                                                          const char* target);

/** Return the Size of a User Defined Structure Component.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Size of the User Defined Structure Component.
 */

LIBRARY_API int udaGetNodeStructureComponentDataSize(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

/** Return the Type Name of a User Defined Structure Component.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the Type Name of the User Defined Structure Component.
 */

LIBRARY_API const char* udaGetNodeStructureComponentDataDataType(LOGMALLOCLIST* logmalloclist, NTREE* ntree,
                                                                 const char* target);

/** Return a pointer to a User Defined Structure Component's data.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data.
 */

LIBRARY_API void* udaGetNodeStructureComponentData(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target);

/** Print a User Defined Structure Component's data.
 *
 * @param fd File Descriptor
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return void.
 */

LIBRARY_API void udaPrintNodeStructureComponentData(NTREE* ntree, LOGMALLOCLIST* logmalloclist,
                                                    USERDEFINEDTYPELIST* userdefinedtypelist, const char* target);

/** Print a Data Structure's Contents.
 *
 * @param fd File Descriptor
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return void.
 */

LIBRARY_API void udaPrintNodeStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a pointer to a User Defined Structure Component's data cast to FLOAT.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */

LIBRARY_API float* udaCastNodeStructureComponentDatatoFloat(LOGMALLOCLIST* logmalloclist, NTREE* ntree,
                                                            const char* target);

/** Return a pointer to a User Defined Structure Component's data cast to DOUBLE.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @param target The name of a User Defined Structure Component.
 * @return the User Defined Structure Component's data cast to float.
 */

LIBRARY_API double* udaCastNodeStructureComponentDatatoDouble(LOGMALLOCLIST* logmalloclist, NTREE* ntree,
                                                              const char* target);

//---------------------------------------------------------------------------------------------
// Tree Branch Family: Whole tree is in scope

/** Initialise a NTREE data structure.
 *
 * @param str A pointer to a NTREE data structure instance.
 * @return void.
 */

LIBRARY_API void udaInitNTree(NTREE* str);

/** Initialise the Global NTREE list structure.
 *
 * @return void.
 */

LIBRARY_API void udaInitNTreeList(NTREELIST* ntree_list);

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */

LIBRARY_API void udaPrintNTree2(NTREE* tree);

/** Print the Contents of a tree node to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */

LIBRARY_API void udaPrintNTree(NTREE* tree, USERDEFINEDTYPELIST* userdefinedtypelist);

/** Print Details of the tree node List to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */

LIBRARY_API void udaPrintNTreeList(NTREE* tree);

/** Return a Count of User Defined Type Tree Nodes from and including the passed tree node.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the Count of Tree Nodes.
 */

LIBRARY_API int udaGetNTreeStructureCount(NTREE* ntree);

/** Return a List of User Defined Type Structure Names attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure names.
 */

LIBRARY_API char** udaGetNTreeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of User Defined Type Structure Type Names attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Type names.
 */
LIBRARY_API char** udaGetNTreeStructureTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Print the Names and Types of all Data Structures to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */
LIBRARY_API void udaPrintNTreeStructureNames(LOGMALLOCLIST* logmalloclist, NTREE* tree);

/** Return the total number of User Defined Type Structure Definition Components attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the number of User Defined Type Structure Definition Components.
 */
LIBRARY_API int udaGetNTreeStructureComponentCount(NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Names attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component names.
 */
LIBRARY_API char** udaGetNTreeStructureComponentNames(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Types attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Types.
 */
LIBRARY_API char** udaGetNTreeStructureComponentTypes(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Return a List of User Defined Type Structure Definition Components Descriptions attached to this tree branch.
 *
 * @param ntree A pointer to a tree node. If NULL the root node is assumed.
 * @return the List of User Defined Type Structure Definition Component Descriptions.
 */
LIBRARY_API char** udaGetNTreeStructureComponentDescriptions(LOGMALLOCLIST* logmalloclist, NTREE* ntree);

/** Print the Names and Types of all Data Elements to a specified File Descriptor.
 *
 * @param fd The File Descriptor, e.g., stdout
 * @param tree A pointer to a tree node. If NULL the root node is assumed.
 * @return void
 */
LIBRARY_API void udaPrintNTreeStructureComponentNames(LOGMALLOCLIST* logmalloclist, NTREE* tree);

//=======================================================================================================
// Print utility functions: explicit output to stdout

LIBRARY_API void udaPrintNode_stdout(NTREE* tree);

LIBRARY_API void udaPrintNodeNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree);

LIBRARY_API void udaPrintNodeAtomic_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree);

LIBRARY_API void udaPrintNTreeStructureNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree);

LIBRARY_API void udaPrintNTreeStructureComponentNames_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree);

LIBRARY_API void udaPrintAtomicType_stdout(LOGMALLOCLIST* logmalloclist, NTREE* tree, const char* target);

LIBRARY_API void udaGetNodeStructureComponentDataShape_f(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
                                                         int* shape_f);

LIBRARY_API void udaGetNodeStructureComponentShortData_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target,
                                                         short* data_f);

LIBRARY_API void udaGetNodeStructureComponentFloatData_f(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* target,
                                                         float* data_f);

LIBRARY_API void udaDereferenceShortData(short* data_c, int count, short* data_f);

LIBRARY_API void udaDereferenceFloatData(float* data_c, int count, float* data_f);

LIBRARY_API short* udaCastNodeStructureComponentDatatoShort(LOGMALLOCLIST* logmalloclist, NTREE* ntree,
                                                            const char* target);

LIBRARY_API void udaCastNodeStructureComponentDatatoShort_f(LOGMALLOCLIST* logmalloclist, NTREE* node,
                                                            const char* target, short* data_f);

LIBRARY_API void udaCastNodeStructureComponentDatatoFloat_f(LOGMALLOCLIST* logmalloclist, NTREE* node,
                                                            const char* target, float* data_f);

LIBRARY_API void udaAddStructureField(USERDEFINEDTYPE* user_type, const char* name, const char* desc,
                                      UDA_TYPE data_type, bool is_pointer, int rank, int* shape, size_t offset);

#ifdef __cplusplus
}
#endif

#endif // UDA_STRUCTURES_STRUCT_H
