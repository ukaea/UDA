//---------------------------------------------------------------------------------------------
// User Accessor functions to General Data Structures
//---------------------------------------------------------------------------------------------
// Locating Data: The whole sub-tree is in scope
//
// There are 6 node location methods:
// The NTREE data structure can be identified either by its name, its user defined type name, its user defined class id,
// or by the memory address of its data.
// It can also be identified by targeting the user defined type by a member name or a member type (user defined type
// only)
//
// Two types:
// A) Return the Tree Node whose data structure type contains a named element or member. This may be
//    either a structure or an atomic typed element.
//
// B) Return the Tree Node containing a named Data Structure (node will be a child of the node located using A).
//    The hierarchical name must consist only of structure types. Identifying data by type may fail as types are named
//    uniquely: same 'type' but different type name!
//
// Hierarchical names can have one of two forms: a.b.c or a/b/c. The latter can also begin with an optional '/'.
//
// Function naming conventions:
//
// Component means named data member. Not the name of the structure type.
//
// udaFindNTreeStructureComponent              data structure member name
// udaFindNTreeStructureComponentDefinition    data structure member's user defined type name
// udaFindNTreeStructure                       tree node name
// udaFindNTreeStructureMalloc                 memory address of the data
// udaFindNTreeStructureDefinition             user defined type name
//
// udaFindNTreeChildStructureComponent         data structure member name within the child nodes only
// udaFindNTreeChildStructure                  tree node name within the child nodes only
//
// udaFindNTreeStructureClass             user defined type class id
// udaMaxCountVlenStructureArray
// udaRegulariseVlenStructures
// udaRegulariseVlenData
//
// udaGetNodeStructureDataCount
// udaGetNodeStructureDataSize
// udaGetNodeStructureDataRank
// udaGetNodeStructureDataShape
// udaGetNodeStructureDataDataType
// udaGetNodeStructureData
//
// udaPrintImage
//
#include "accessors.h"

#include <clientserver/stringUtils.h>
#include <cstdlib>
#include <include/logging.h>

#include "struct.h"

/** Find (search type A) the first Tree Node with a data structure type containing a named element/member.
 *
 * This is a private function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node that defines the start (root) of the sub-tree. If nullptr the root node
 * is assumed.
 * @param target The name of the data structure's element/member (case sensitive)
 * This element may be either a data structure or an atomic typed element. The first occurance of the name is selected.
 * @return the Tree Node containing the named element.
 */

NTREE* udaFindNTreeStructureComponent1(NTREE* ntree, const char* target)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Single entity name expected - test

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        return nullptr;
    }

    // Is it the name of the current tree node?

    if (STR_EQUALS(ntree->name, target)) {
        return ntree;
    }

    // Search the components of this node for atomic types

    for (int i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (STR_EQUALS(ntree->userdefinedtype->compoundfield[i].name, target) &&
            ntree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            return ntree;
        }
    }

    // Recursively Search Child nodes for structured type data

    for (int i = 0; i < ntree->branches; i++) {
        NTREE* child = nullptr;
        if ((child = udaFindNTreeStructureComponent1(ntree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr; // Not found
}

/** Find (search type A) the first Tree Node with a data structure type containing a named element/member.
 * The name of the element is also returned.
 *
 * This is a private function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node that defines the start (root) of the sub-tree. If nullptr the root node
 * is assumed.
 * @param target The name of the data structure element/member (case sensitive) using the hierachical naming syntax
 * a.b.c or a/b/c. This element may be either a data structure or an atomic typed element. If a single named item is
 * specified without its hierarchical naming context, the tree node with the first occurance of the name is selected.
 * Using the hierarchy imposes more rigour to the search.
 * @param lastname Returns the name of the element, i.e., the name of the last item in the name hierarchy.
 * @return the Tree Node containing the named element.
 */
/*
If a hierarchical set of names is passed, all except the last must be a structured type
The last may be either a structured or an atomic type
Search all but the last on the child tree nodes.
The first name must be searched for down the tree from the root or starting node
All subsequent names must be within child nodes unless the last name
*/
NTREE* udaFindNTreeStructureComponent2(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target,
                                    const char** lastname)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        char** targetlist = udaParseTarget(target, &ntargets); // Deconstruct the Name and search for each hierarchy group

        *lastname = targetlist[ntargets - 1]; // Preserve the last element name

        // Search recursively for the first name

        NTREE* child = ntree;
        if ((child = udaFindNTreeStructureComponent1(child, targetlist[0])) == nullptr) {
            // Not found
            return nullptr;
        }

        // Search child nodes for all names but the last name

        for (int i = 1; i < ntargets - 1; i++) {
            NTREE* found = nullptr;
            for (int j = 0; j < child->branches; j++) {
                if (STR_EQUALS(child->children[j]->name, targetlist[i])) {
                    found = child->children[j];
                    break;
                }
            }
            if (found == nullptr) {
                // Not found
                return nullptr;
            }
            child = found;
        }

        udaAddMalloc(logmalloclist, (void*)targetlist[ntargets - 1], (int)strlen(targetlist[ntargets - 1]) + 1,
                  sizeof(char), "char");

        const char* last_target = targetlist[ntargets - 1];

        for (int i = 0; i < ntargets - 1; i++) {
            free(targetlist[i]); // Free all others
        }
        free(targetlist); // Free the list

        // Search the user defined type definition for the last name - return if an atomic type

        for (int i = 0; i < child->userdefinedtype->fieldcount; i++) {
            if (STR_EQUALS(child->userdefinedtype->compoundfield[i].name, last_target) &&
                child->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
                return child;
            } // Atomic type found
        }

        // Search child nodes for structured types

        for (int j = 0; j < child->branches; j++) {
            if (STR_EQUALS(child->children[j]->name, last_target)) {
                return child->children[j];
            }
        }

        return nullptr; // Not Found
    }

    // Recursively search using the single name passed

    *lastname = target;

    NTREE* child;
    if ((child = udaFindNTreeStructureComponent1(ntree, target)) != nullptr) {
        return child; // Found
    }

    return nullptr; // Not found
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 * The name of the structure is also returned.
 *
 * This is a private function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @param lastname Returns the name of the Structure, i.e., the name of the last node in the name hierarchy.
 * @return the Data Tree Node with the structure name.
 */
NTREE* udaFindNTreeStructure2(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target, const char** lastname)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        char** targetlist = nullptr;
        NTREE* child = ntree;

        targetlist = udaParseTarget(target, &ntargets); // Deconstruct Name and search for each hierarchy group

        for (int i = 0; i < ntargets; i++) { // Drill Down to requested named structure element
            if (i < ntargets - 1) {
                child = udaFindNTreeStructure2(logmalloclist, child, targetlist[i], lastname);
            } else {
                NTREE* test = nullptr;
                if ((test = udaFindNTreeStructure2(logmalloclist, child, targetlist[i], lastname)) ==
                    nullptr) { // Last element may not be a structure
                    if (udaFindNTreeStructureComponent2(logmalloclist, child, targetlist[i], lastname) == nullptr) {
                        child = nullptr;
                    }
                } else {
                    child = test;
                }
            }
            if (child == nullptr) {
                break;
            }
        }

        *lastname = targetlist[ntargets - 1]; // Preserve the last element name

        udaAddMalloc(logmalloclist, (void*)targetlist[ntargets - 1], (int)strlen(targetlist[ntargets - 1]) + 1,
                  sizeof(char), "char");
        for (int i = 0; i < ntargets - 1; i++) {
            // Free all others
            free(targetlist[i]);
        }
        free(targetlist); // Free the list

        return child; // Always the last node you look in !
    }

    // Search using a single hierarchical group name

    *lastname = target;

    if (STR_EQUALS(ntree->name, target)) {
        // Test the current Tree Node
        return ntree;
    }

    // Search Child nodes

    for (int i = 0; i < ntree->branches; i++) {
        if (STR_EQUALS(ntree->children[i]->name, target)) {
            return ntree->children[i];
        }
    }

    return nullptr;
}

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named element.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or
 * a/b/c. This element may be either a structure itself or an atomic typed element.
 * @return the Data Tree Node.
 */
NTREE* udaFindNTreeStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname = nullptr;
    return udaFindNTreeStructureComponent2(logmalloclist, ntree, target, &lastname);
}

/** Find (search type A) and return a Pointer to the Child Data Tree Node with a data structure that contains a named
 * element.
 *
 * This is a public function with the child sub-trees in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or
 * a/b/c. This element may be either a structure itself or an atomic typed element.
 * @return the Data Tree Node.
 */
NTREE* udaFindNTreeChildStructureComponent(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Search each child branch

    for (int i = 0; i < ntree->branches; i++) {
        NTREE* child = nullptr;
        if ((child = udaFindNTreeStructureComponent(logmalloclist, ntree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr;
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @return the Data Tree Node.
 */
NTREE* udaFindNTreeStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    const char* lastname = nullptr;
    return udaFindNTreeStructure2(logmalloclist, ntree, target, &lastname);
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 *
 * This is a public function with child sub-trees in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @return the child Data Tree Node.
 */
NTREE* udaFindNTreeChildStructure(LOGMALLOCLIST* logmalloclist, NTREE* ntree, const char* target)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Search each child branch

    for (int i = 0; i < ntree->branches; i++) {
        NTREE* child = nullptr;
        if ((child = udaFindNTreeStructure(logmalloclist, ntree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr;
}

/** Find and return a Pointer to a Data Tree Node with a data structure located at a specific memory location.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param data The heap address of the data.
 * @return the Data Tree Node.
 */
NTREE* udaFindNTreeStructureMalloc(NTREE* ntree, void* data)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    if (data == ntree->data) {
        return ntree;
    }
    for (int i = 0; i < ntree->branches; i++) {
        NTREE* next;
        if ((next = udaFindNTreeStructureMalloc(ntree->children[i], data)) != nullptr) {
            return next;
        }
    }
    return nullptr;
}

/** Locate a tree node with structured data having the specified Structure Definition name.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param ntree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure Definition.
 * @return A pointer to the First tree node found with the targeted structure definition.
 */
NTREE* udaFindNTreeStructureDefinition(NTREE* ntree, const char* target)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        char** targetlist = nullptr;
        NTREE* child = ntree;

        targetlist = udaParseTarget(target, &ntargets); // Deconstruct the Name and search for each hierarchy group

        for (int i = 0; i < ntargets; i++) { // Drill Down to requested named structure type
            if ((child = udaFindNTreeStructureDefinition(child, targetlist[i])) == nullptr) {
                break;
            }
        }

        // Free all entries
        for (int i = 0; i < ntargets; i++) {
            free(targetlist[i]);
        }

        free(targetlist); // Free the list

        return child;
    }

    if (STR_EQUALS(ntree->userdefinedtype->name, target)) {
        return ntree;
    }

    for (int i = 0; i < ntree->branches; i++) {
        NTREE* child = nullptr;
        if ((child = udaFindNTreeStructureDefinition(ntree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr;
}

NTREE* xudaFindNTreeStructureDefinition(NTREE* tree, const char* target)
{
    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    if (STR_EQUALS(tree->userdefinedtype->name, target)) {
        return tree;
    }

    for (int i = 0; i < tree->branches; i++) {
        NTREE* next;
        if ((next = udaFindNTreeStructureDefinition(tree->children[i], target)) != nullptr) {
            return next;
        }
    }

    return nullptr;
}

/** Locate a tree node with structured data having the specified Structure Definition name.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure Definition.
 * @return A pointer to the First tree node found with the targeted structure definition.
 */
NTREE* udaFindNTreeStructureComponentDefinition(NTREE* tree, const char* target)
{
    NTREE* next;

    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN &&
            STR_EQUALS(tree->userdefinedtype->compoundfield[i].type, target)) {
            return tree;
        }
    }

    for (int i = 0; i < tree->branches; i++) {
        if ((next = udaFindNTreeStructureComponentDefinition(tree->children[i], target)) != nullptr) {
            return next;
        }
    }

    return nullptr;
}

/** Locate a tree node with structured data having a Specific Structure Class.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param class The Structure Class, e.g., UDA_TYPE_VLEN.
 * @return A pointer to the First tree node found with the targeted structure class.
 */
NTREE* udaFindNTreeStructureClass(NTREE* tree, int cls)
{
    NTREE* next;

    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    if (tree->userdefinedtype->idamclass == cls) {
        return tree;
    }

    for (int i = 0; i < tree->branches; i++) {
        if ((next = udaFindNTreeStructureClass(tree->children[i], cls)) != nullptr) {
            return next;
        }
    }

    return nullptr;
}

/** Identify the largest count of a Variable Length Array with a given structure type.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the VLEN Structure Definition.
 * @param reset Reset the counbter to zero.
 * @return An integer returning the maximum count value.
 */
int udaMaxCountVlenStructureArray(NTREE* tree, const char* target, int reset)
{
    static unsigned int count = 0;
    if (reset) {
        count = 0;
    }

    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    if (tree->userdefinedtype->idamclass == UDA_TYPE_VLEN && STR_EQUALS(tree->userdefinedtype->name, target)) {
        auto vlen = (VLENTYPE*)tree->data;
        if (vlen->len > count) {
            count = vlen->len;
        }
    }

    for (int i = 0; i < tree->branches; i++) {
        count = (unsigned int)udaMaxCountVlenStructureArray(tree->children[i], target, 0);
    }

    return count;
}

/** Regularise a specific VLEN structure.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the VLEN Structure Definition.
 * @param count The maximum count size for the VLEN data arrays.
 * @return An integer returning an error code: 0 => OK.
 */
int udaRegulariseVlenStructures(LOGMALLOCLIST* logmalloclist, NTREE* tree, USERDEFINEDTYPELIST* userdefinedtypelist,
                                  const char* target, unsigned int count)
{
    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    int resetBranches = 0;
    int size = 0;
    void* newnew = nullptr;

    if (tree->userdefinedtype->idamclass == UDA_TYPE_VLEN && STR_EQUALS(tree->userdefinedtype->name, target)) {
        auto vlen = (VLENTYPE*)tree->data;

        // VLEN stuctures have only two fields: len and data
        // Need the size of the data component

        if (vlen->len == 0) {
            return 1; // No data!
        }
        if (vlen->len > count) {
            return 1; // Incorrect count value
        }

        // VLEN Memory is contiguous so re-allocate: regularise by expanding to a consistent array size (No longer a
        // VLEN!)

        auto old = (VOIDTYPE)vlen->data;
        USERDEFINEDTYPE* child =
            findUserDefinedType(userdefinedtypelist, tree->userdefinedtype->compoundfield[1].type, 0);
        vlen->data = realloc(vlen->data, count * child->size); // Expand Heap to regularise
        newnew = vlen->data;
        size = child->size;
        udaChangeMalloc(logmalloclist, old, vlen->data, count, child->size, child->name);
        tree->data = (void*)vlen;

        // Write new data array to Original Tree Nodes

        for (unsigned int i = 0; i < vlen->len; i++) {
            tree->children[i]->data = (char*)newnew + i * size;
        }

        resetBranches = vlen->len; // Flag requirement to add extra tree nodes
    }

    for (int i = 0; i < tree->branches; i++) {
        int rc;
        if ((rc = udaRegulariseVlenStructures(logmalloclist, tree->children[i], userdefinedtypelist, target,
                                                count)) != 0) {
            return rc;
        }
    }

    // Update branch count and add new Child nodes with New data array

    if (resetBranches > 0) {
        tree->branches = count; // Only update once all True children have been regularised
        auto old = (VOIDTYPE)tree->children;
        tree->children = (NTREE**)realloc((void*)tree->children, count * sizeof(void*));

        unsigned int ui;
        for (ui = (unsigned int)resetBranches; ui < count; ui++) {
            tree->children[ui] = (NTREE*)malloc(sizeof(NTREE));
            udaAddMalloc(logmalloclist, (void*)tree->children[ui], 1, sizeof(NTREE), "NTREE");
            memcpy(tree->children[ui], tree->children[0], sizeof(NTREE));
        }
        udaChangeMalloc(logmalloclist, old, (void*)tree->children, count, sizeof(NTREE), "NTREE");

        // Update All new Child Nodes with array element addresses

        for (ui = (unsigned int)resetBranches; ui < count; ui++) {
            memcpy((char*)newnew + ui * size, newnew, size); // write extra array items: use the first array element
        }

        for (ui = (unsigned int)resetBranches; ui < count; ui++) {
            tree->children[ui]->data = (char*)newnew + ui * size;
        }
    }

    return 0;
}

/** Regularise the Shape of All VLEN structured data arrays in the data tree: necessary for accessing in some languages,
 * e.g. IDL.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @return An integer returning an error code: 0 => OK.
 */
int udaRegulariseVlenData(LOGMALLOCLIST* logmalloclist, NTREE* tree, USERDEFINEDTYPELIST* userdefinedtypelist)
{
    int rc = 0, count = 0;
    NTREE* nt = nullptr;
    if (tree == nullptr) {
        tree = udaGetFullNTree();
    }

    do {
        if ((nt = udaFindNTreeStructureClass(tree, UDA_TYPE_VLEN)) != nullptr) {
            count = udaMaxCountVlenStructureArray(tree, nt->userdefinedtype->name, 1);
            if (count > 0) {
                rc = udaRegulariseVlenStructures(logmalloclist, tree, userdefinedtypelist, nt->userdefinedtype->name,
                                                   count);
            }
            if (rc != 0) {
                return rc;
            }
            nt->userdefinedtype->idamclass = UDA_TYPE_COMPOUND; // Change the class to 'regular compound structure'
        }
    } while (nt != nullptr);

    return 0;
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// User Accessor functions to Node Data (only the current node is in scope)

/** Return the Count of data array elements attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of structured data array elements.
 */
int udaGetNodeStructureDataCount(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, size;
    const char* type;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    udaFindMalloc(logmalloclist, (void*)&ntree->data, &count, &size, &type);
    return count;
}

/** Return the Size (bytes) of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Size (bytes) of the structured data array.
 */
int udaGetNodeStructureDataSize(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, size;
    const char* type;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    udaFindMalloc(logmalloclist, (void*)&ntree->data, &count, &size, &type);
    return size;
}

/** Return the rank of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return The rank of the structured data array.
 */
int udaGetNodeStructureDataRank(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, size, rank;
    int* shape;
    const char* type;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    udaFindMalloc2(logmalloclist, (void*)&ntree->data, &count, &size, &type, &rank, &shape);
    return rank;
}

/** Return the shape of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return A pointer to the integer shape array of the structured data array.
 */
int* udaGetNodeStructureDataShape(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, size, rank;
    int* shape;
    const char* type;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }

    if (ntree->parent != nullptr) {
        int branches = ntree->parent->branches;
        fprintf(stdout, "\n%p Parent Name %s\n", ntree, ntree->parent->name);
        fprintf(stdout, "%p Parent Type %s\n", ntree, ntree->parent->userdefinedtype->name);
        fprintf(stdout, "%p Siblings %d\n", ntree, branches);
        if (branches > 1) {
            int id = 0;
            for (int i = 0; i < branches; i++) {
                if (ntree->parent->children[i] == ntree) {
                    id = i;
                    break;
                }
            }
            fprintf(stdout, "%p Child ID %d\n", ntree, id);
        }
        fflush(stdout);
    }

    udaFindMalloc2(logmalloclist, (void*)&ntree->data, &count, &size, &type, &rank, &shape);
    return shape;
}

/** Return a pointer to the structured data type name of the data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the data type name of the structured data array.
 */
const char* udaGetNodeStructureDataDataType(LOGMALLOCLIST* logmalloclist, NTREE* ntree)
{
    int count, size;
    const char* type = nullptr;
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    udaFindMalloc(logmalloclist, (void*)&ntree->data, &count, &size, &type);
    return type;
}

/** Return a pointer to the data attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param ntree A pointer to a tree node. If nullptr the root node is assumed.
 * @return A void pointer to the data .
 */
void* udaGetNodeStructureData(NTREE* ntree)
{
    if (ntree == nullptr) {
        ntree = udaGetFullNTree();
    }
    return ntree->data;
}

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
void udaPrintImage(const char* image, int imagecount)
{
    int next = 0;
    if (image == nullptr || imagecount == '\0') {
        return;
    }
    while (next < imagecount) {
        UDA_LOG(UDA_LOG_DEBUG, "%s", &image[next]);
        next = next + (int)strlen(&image[next]) + 1;
    }
}

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
void udaDefineField(COMPOUNDFIELD* field, const char* name, const char* desc, int* offset, unsigned short type_id)
{
    udaInitCompoundField(field);
    strcpy(field->name, name);

    field->pointer = 0; // default for scalar values
    field->count = 1;

    switch (type_id) {
        case SCALARDOUBLE:
            field->atomictype = UDA_TYPE_DOUBLE;
            strcpy(field->type, "double");
            snprintf(field->desc, MAXELEMENTNAME, "[double %s] %s", name, desc);
            field->size = field->count * sizeof(double);
            break;
        case ARRAYDOUBLE:
            field->atomictype = UDA_TYPE_DOUBLE;
            strcpy(field->type, "double *");
            snprintf(field->desc, MAXELEMENTNAME, "[double *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(double*);
            break;
        case SCALARFLOAT:
            field->atomictype = UDA_TYPE_FLOAT;
            strcpy(field->type, "float");
            snprintf(field->desc, MAXELEMENTNAME, "[float %s] %s", name, desc);
            field->size = field->count * sizeof(float);
            break;
        case ARRAYFLOAT:
            field->atomictype = UDA_TYPE_FLOAT;
            strcpy(field->type, "float *");
            snprintf(field->desc, MAXELEMENTNAME, "[float *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(float*);
            break;
        case SCALARLONG64:
            field->atomictype = UDA_TYPE_LONG64;
            strcpy(field->type, "long long");
            snprintf(field->desc, MAXELEMENTNAME, "[long long %s] %s", name, desc);
            field->size = field->count * sizeof(long long);
            break;
        case ARRAYLONG64:
            field->atomictype = UDA_TYPE_LONG64;
            strcpy(field->type, "long long *");
            snprintf(field->desc, MAXELEMENTNAME, "[long long *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(long long*);
            break;
        case SCALARULONG64:
            field->atomictype = UDA_TYPE_UNSIGNED_LONG64;
            strcpy(field->type, "unsigned long long");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned long long %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned long long);
            break;
        case ARRAYULONG64:
            field->atomictype = UDA_TYPE_UNSIGNED_LONG64;
            strcpy(field->type, "unsigned long long *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned long long *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned long long*);
            break;
        case SCALARINT:
            field->atomictype = UDA_TYPE_INT;
            strcpy(field->type, "int");
            snprintf(field->desc, MAXELEMENTNAME, "[int %s] %s", name, desc);
            field->size = field->count * sizeof(int);
            break;
        case ARRAYINT:
            field->atomictype = UDA_TYPE_INT;
            strcpy(field->type, "int *");
            snprintf(field->desc, MAXELEMENTNAME, "[int *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(int*);
            break;
        case SCALARUINT:
            field->atomictype = UDA_TYPE_UNSIGNED_INT;
            strcpy(field->type, "unsigned int");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned int %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned int);
            break;
        case ARRAYUINT:
            field->atomictype = UDA_TYPE_UNSIGNED_INT;
            strcpy(field->type, "unsigned int *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned int *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned int*);
            break;
        case SCALARSHORT:
            field->atomictype = UDA_TYPE_SHORT;
            strcpy(field->type, "short");
            snprintf(field->desc, MAXELEMENTNAME, "[short %s] %s", name, desc);
            field->size = field->count * sizeof(short);
            break;
        case ARRAYSHORT:
            field->atomictype = UDA_TYPE_SHORT;
            strcpy(field->type, "short *");
            snprintf(field->desc, MAXELEMENTNAME, "[short *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(short*);
            break;
        case SCALARUSHORT:
            field->atomictype = UDA_TYPE_UNSIGNED_SHORT;
            strcpy(field->type, "unsigned short");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned short %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned short);
            break;
        case ARRAYUSHORT:
            field->atomictype = UDA_TYPE_UNSIGNED_SHORT;
            strcpy(field->type, "unsigned short *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned short *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned short*);
            break;
        case SCALARCHAR:
            field->atomictype = UDA_TYPE_CHAR;
            strcpy(field->type, "char");
            snprintf(field->desc, MAXELEMENTNAME, "[char %s] %s", name, desc);
            field->size = field->count * sizeof(char);
            break;
        case ARRAYCHAR:
            field->atomictype = UDA_TYPE_CHAR;
            strcpy(field->type, "char *");
            snprintf(field->desc, MAXELEMENTNAME, "[char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char*);
            break;
        case SCALARSTRING:
            field->atomictype = UDA_TYPE_STRING;
            strcpy(field->type, "STRING");
            snprintf(field->desc, MAXELEMENTNAME, "[char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char*);
            field->offset =
                (int)newoffset((size_t)*offset, "char *"); // must be an explicit char pointer (STRING Convention!)
            field->offpad = (int)padding((size_t)*offset, "char *");
            field->alignment = udaGetalignmentof("char *");
            break;
        case ARRAYSTRING:
            // Bug Fix dgm 07Jul2014: atomictype was missing!
            field->atomictype = UDA_TYPE_STRING;
            strcpy(field->type, "STRING *");
            snprintf(field->desc, MAXELEMENTNAME, "[char **%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(char**);
            break;
        case ARRAYVOID:
            field->atomictype = UDA_TYPE_VOID;
            strcpy(field->type, "void *");
            snprintf(field->desc, MAXELEMENTNAME, "[void *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(void*);
            break;
        case SCALARUCHAR:
            field->atomictype = UDA_TYPE_UNSIGNED_CHAR;
            strcpy(field->type, "unsigned char");
            snprintf(field->desc, MAXELEMENTNAME, "[char %s] %s", name, desc);
            field->size = field->count * sizeof(unsigned char);
            break;
        case ARRAYUCHAR:
            field->atomictype = UDA_TYPE_UNSIGNED_CHAR;
            strcpy(field->type, "unsigned char *");
            snprintf(field->desc, MAXELEMENTNAME, "[unsigned char *%s] %s", name, desc);
            field->pointer = 1;
            field->size = field->count * sizeof(unsigned char*);
            break;
    }

    field->rank = 0;
    field->shape = nullptr;

    if (type_id != SCALARSTRING) {
        field->offset = (int)newoffset(*offset, field->type);
        field->offpad = (int)padding(*offset, field->type);
        field->alignment = udaGetalignmentof(field->type);
    }

    *offset = field->offset + field->size; // Next Offset
}

void udaDefineCompoundField(COMPOUNDFIELD* field, const char* type, const char* name, char* desc, int offset, int size)
{
    udaInitCompoundField(field);

    strcpy(field->name, name);
    field->atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field->type, type);
    strcpy(field->desc, desc);

    field->pointer = 1;
    field->count = 1;
    field->rank = 0;
    field->shape = nullptr;

    field->size = field->count * size;
    field->offset = offset;
    field->offpad = padding(offset, field->type);
    field->alignment = ALIGNMENT;
}
