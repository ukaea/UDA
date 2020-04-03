#ifndef IDAM_STRUCTURES_ACCESSORS_H
#define IDAM_STRUCTURES_ACCESSORS_H

#include <structures/genStructs.h>

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Find (search type A) the first Tree Node with a data structure type containing a named element/member.
* The name of the element is also returned.  
*
* This is a private function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node that defines the start (root) of the sub-tree. If NULL the root node is assumed. 
* @param target The name of the data structure element/member (case sensitive) using the hierachical naming syntax a.b.c or a/b/c.
* This element may be either a data structure or an atomic typed element. If a single named item is specified without 
* its hierarchical naming context, the tree node with the first occurance of the name is selected. Using the hierarchy imposes
* more rigour to the search.  
* @param lastname Returns the name of the element, i.e., the name of the last item in the name hierarchy.
* @return the Tree Node containing the named element.
*/ 
LIBRARY_API NTREE *findNTreeStructureComponent2(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target, const char **lastname);

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
* The name of the structure is also returned.  
*
* This is a private function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @param lastname Returns the name of the Structure, i.e., the name of the last node in the name hierarchy.
* @return the Data Tree Node with the structure name.
*/
LIBRARY_API NTREE *findNTreeStructure2(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target, const char **lastname);

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named element. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @return the Data Tree Node.
*/ 
LIBRARY_API NTREE *findNTreeStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target);

/** Find (search type A) and return a Pointer to the Child Data Tree Node with a data structure that contains a named element. 
*
* This is a public function with the child sub-trees in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @return the Data Tree Node.
*/ 
LIBRARY_API NTREE *findNTreeChildStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target);

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @return the Data Tree Node.
*/ 
LIBRARY_API NTREE *findNTreeStructure(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target);

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
*
* This is a public function with child sub-trees in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @return the child Data Tree Node.
*/
LIBRARY_API NTREE *findNTreeChildStructure(LOGMALLOCLIST* logmalloclist, NTREE *ntree, const char * target);

/** Find and return a Pointer to a Data Tree Node with a data structure located at a specific memory location.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param data The heap address of the data.
* @return the Data Tree Node.
*/
LIBRARY_API NTREE* findNTreeStructureMalloc(NTREE* ntree, void* data);

/** Locate a tree node with structured data having the specified Structure Definition name. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/
LIBRARY_API NTREE* findNTreeStructureDefinition(NTREE* ntree, const char* target);

/** Locate a tree node with structured data having the specified Structure Definition name. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/
LIBRARY_API NTREE* findNTreeStructureComponentDefinition(NTREE* tree, const char* target);

/** Locate a tree node with structured data having a Specific Structure Class. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param class The Structure Class, e.g., UDA_TYPE_VLEN.
* @return A pointer to the First tree node found with the targeted structure class.
*/
LIBRARY_API NTREE* idam_findNTreeStructureClass(NTREE* tree, int class_);

/** Identify the largest count of a Variable Length Array with a given structure type. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the VLEN Structure Definition.
* @param reset Reset the counbter to zero.
* @return An integer returning the maximum count value.
*/
LIBRARY_API int idam_maxCountVlenStructureArray(NTREE* tree, const char* target, int reset);

/** Regularise a specific VLEN structure. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @param target The name of the VLEN Structure Definition.
* @param count The maximum count size for the VLEN data arrays.
* @return An integer returning an error code: 0 => OK.
*/
LIBRARY_API int idam_regulariseVlenStructures(LOGMALLOCLIST* logmalloclist, NTREE *tree, USERDEFINEDTYPELIST* userdefinedtypelist, const char * target, unsigned int count);

/** Regularise the Shape of All VLEN structured data arrays in the data tree: necessary for accessing in some languages, e.g. IDL. 
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.  
* @return An integer returning an error code: 0 => OK.
*/
LIBRARY_API int idam_regulariseVlenData(LOGMALLOCLIST* logmalloclist, NTREE *tree, USERDEFINEDTYPELIST* userdefinedtypelist);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// User Accessor functions to Node Data (only the current node is in scope)   

/** Return the Count of data array elements attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the Count of structured data array elements.
*/ 
LIBRARY_API int getNodeStructureDataCount(LOGMALLOCLIST* logmalloclist, NTREE *ntree);

/** Return the Size (bytes) of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the Size (bytes) of the structured data array.
*/ 
LIBRARY_API int getNodeStructureDataSize(LOGMALLOCLIST* logmalloclist, NTREE *ntree);

/** Return the rank of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return The rank of the structured data array.
*/
LIBRARY_API int getNodeStructureDataRank(LOGMALLOCLIST* logmalloclist, NTREE *ntree);

/** Return the shape of the structured data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return A pointer to the integer shape array of the structured data array.
*/ 
LIBRARY_API int *getNodeStructureDataShape(LOGMALLOCLIST* logmalloclist, NTREE *ntree);

/** Return a pointer to the structured data type name of the data array attached to this tree node. 
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return the data type name of the structured data array.
*/
LIBRARY_API const char *getNodeStructureDataDataType(LOGMALLOCLIST* logmalloclist, NTREE *ntree);
 
/** Return a pointer to the data attached to this tree node.  
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed. 
* @return A void pointer to the data .
*/
LIBRARY_API void* getNodeStructureData(NTREE* ntree);

//----------------------------------------------------------------------------------------------------------
// Sundry utility functions

/** Print the Contents of a tree node to a specified File Descriptor. The arguments for this function are 
* available from the Structure Definition structure. 
*
* This is a public function with the current tree node only in scope.
*
* @param fd The File Descriptor, e.g., stdout
* @param image A text block containing null terminated strings that forms an text image of the structure definition.
* @param imagecount The number of bytes in the image text block.
* @return Void
*/
LIBRARY_API void printImage(const char* image, int imagecount);

//---------------------------------------------------------------------------------------------------------- 
/**User defined structure field definition for common types  
*
* This is a public function  
*
* @param field The user defined structure's field to be populated
* @param name Name of the structure's field.
* @param desc Description of the fields contents
* @param offset Current field byte offset from the start of the structure. Ppdated on return.
* @param type_id Enumerated key indicating the type of data field, e.g. float array
* @return Void
*/
LIBRARY_API void defineField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset, unsigned short type_id);

LIBRARY_API void defineCompoundField(COMPOUNDFIELD* field, const char* type, const char* name, char* desc, int offset, int size);

#ifdef __cplusplus
}
#endif

#endif // IDAM_STRUCTURES_ACCESSORS_H

