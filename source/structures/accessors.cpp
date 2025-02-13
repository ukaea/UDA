//---------------------------------------------------------------------------------------------
// User Accessor functions to General Data Structures
//---------------------------------------------------------------------------------------------
// Locating Data: The whole sub-tree is in scope
//
// There are 6 node location methods:
// The NTree data structure can be identified either by its name, its user defined type name, its user defined class id,
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

#include <uda/types.h>

#include "common/string_utils.h"
#include "logging/logging.h"
#include "struct.h"

using namespace uda::logging;
using namespace uda::structures;
using namespace uda::common;

/** Find (search type A) the first Tree Node with a data structure type containing a named element/member.
 *
 * This is a private function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node that defines the start (root) of the sub-tree. If nullptr the root node
 * is assumed.
 * @param target The name of the data structure's element/member (case sensitive)
 * This element may be either a data structure or an atomic typed element. The first occurance of the name is selected.
 * @return the Tree Node containing the named element.
 */

NTree* find_ntree_structure_component1(NTree* tree, const char* target)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Single entity name expected - test

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        return nullptr;
    }

    // Is it the name of the current tree node?

    if (STR_EQUALS(tree->name, target)) {
        return tree;
    }

    // Search the components of this node for atomic types

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (STR_EQUALS(tree->userdefinedtype->compoundfield[i].name, target) &&
            tree->userdefinedtype->compoundfield[i].atomictype != UDA_TYPE_UNKNOWN) {
            return tree;
        }
    }

    // Recursively Search Child nodes for structured type data

    for (int i = 0; i < tree->branches; i++) {
        NTree* child = nullptr;
        if ((child = find_ntree_structure_component1(tree->children[i], target)) != nullptr) {
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
 * @param tree A pointer to a parent tree node that defines the start (root) of the sub-tree. If nullptr the root node
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
NTree* uda::structures::find_ntree_structure_component2(LogMallocList* log_malloc_list, NTree* tree,
    const char* target, const char** lastname)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        // Deconstruct the Name and search for each hierarchy group
        char** targetlist = parse_target(target, &ntargets);

        *lastname = targetlist[ntargets - 1]; // Preserve the last element name

        // Search recursively for the first name

        NTree* child = tree;
        if ((child = find_ntree_structure_component1(child, targetlist[0])) == nullptr) {
            // Not found
            return nullptr;
        }

        // Search child nodes for all names but the last name

        for (int i = 1; i < ntargets - 1; i++) {
            NTree* found = nullptr;
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

        add_malloc(log_malloc_list, targetlist[ntargets - 1], (int)strlen(targetlist[ntargets - 1]) + 1, sizeof(char), "char");

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

    NTree* child;
    if ((child = find_ntree_structure_component1(tree, target)) != nullptr) {
        return child; // Found
    }

    return nullptr; // Not found
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 * The name of the structure is also returned.
 *
 * This is a private function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @param lastname Returns the name of the Structure, i.e., the name of the last node in the name hierarchy.
 * @return the Data Tree Node with the structure name.
 */
NTree* find_ntree_structure2(LogMallocList* log_malloc_list, NTree* tree, const char* target, const char** lastname)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        char** targetlist = nullptr;
        NTree* child = tree;

        targetlist = parse_target(target, &ntargets); // Deconstruct Name and search for each hierarchy group

        for (int i = 0; i < ntargets; i++) { // Drill Down to requested named structure element
            if (i < ntargets - 1) {
                child = find_ntree_structure2(log_malloc_list, child, targetlist[i], lastname);
            } else {
                NTree* test = nullptr;
                if ((test = find_ntree_structure2(log_malloc_list, child, targetlist[i], lastname)) ==
                    nullptr) { // Last element may not be a structure
                    if (find_ntree_structure_component2(log_malloc_list, child, targetlist[i], lastname) == nullptr) {
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

        add_malloc(log_malloc_list, targetlist[ntargets - 1], (int)strlen(targetlist[ntargets - 1]) + 1,
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

    if (STR_EQUALS(tree->name, target)) {
        // Test the current Tree Node
        return tree;
    }

    // Search Child nodes

    for (int i = 0; i < tree->branches; i++) {
        if (STR_EQUALS(tree->children[i]->name, target)) {
            return tree->children[i];
        }
    }

    return nullptr;
}

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named element.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or
 * a/b/c. This element may be either a structure itself or an atomic typed element.
 * @return the Data Tree Node.
 */
NTree* uda::structures::find_ntree_structure_component(LogMallocList* log_malloc_list, NTree* tree, const char* target)
{
    const char* lastname = nullptr;
    return find_ntree_structure_component2(log_malloc_list, tree, target, &lastname);
}

/** Find (search type A) and return a Pointer to the Child Data Tree Node with a data structure that contains a named
 * element.
 *
 * This is a public function with the child sub-trees in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or
 * a/b/c. This element may be either a structure itself or an atomic typed element.
 * @return the Data Tree Node.
 */
NTree* find_ntree_child_structure_component(LogMallocList* log_malloc_list, NTree* tree, const char* target)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Search each child branch

    for (int i = 0; i < tree->branches; i++) {
        NTree* child = nullptr;
        if ((child = find_ntree_structure_component(log_malloc_list, tree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr;
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @return the Data Tree Node.
 */
NTree* uda::structures::find_ntree_structure(LogMallocList* log_malloc_list, NTree* tree, const char* target)
{
    const char* lastname = nullptr;
    return find_ntree_structure2(log_malloc_list, tree, target, &lastname);
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
 *
 * This is a public function with child sub-trees in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
 * @return the child Data Tree Node.
 */
NTree* find_ntree_child_structure(LogMallocList* log_malloc_list, NTree* tree, const char* target)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Search each child branch

    for (int i = 0; i < tree->branches; i++) {
        NTree* child = nullptr;
        if ((child = find_ntree_structure(log_malloc_list, tree->children[i], target)) != nullptr) {
            return child;
        }
    }

    return nullptr;
}

/** Find and return a Pointer to a Data Tree Node with a data structure located at a specific memory location.
 *
 * This is a public function with the whole sub-tree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param data The heap address of the data.
 * @return the Data Tree Node.
 */
NTree* find_ntree_structure_malloc(NTree* tree, void* data)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    if (data == tree->data) {
        return tree;
    }
    for (int i = 0; i < tree->branches; i++) {
        NTree* next;
        if ((next = find_ntree_structure_malloc(tree->children[i], data)) != nullptr) {
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
NTree* uda::structures::find_ntree_structure_definition(NTree* tree, const char* target)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if ((strchr(target, '.') != nullptr) || strchr(target, '/') != nullptr) {
        int ntargets;
        char** targetlist = nullptr;
        NTree* child = tree;

        targetlist = parse_target(target, &ntargets); // Deconstruct the Name and search for each hierarchy group

        for (int i = 0; i < ntargets; i++) { // Drill Down to requested named structure type
            if ((child = find_ntree_structure_definition(child, targetlist[i])) == nullptr) {
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

    if (STR_EQUALS(tree->userdefinedtype->name, target)) {
        return tree;
    }

    for (int i = 0; i < tree->branches; i++) {
        NTree* child = nullptr;
        if ((child = find_ntree_structure_definition(tree->children[i], target)) != nullptr) {
            return child;
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
NTree* find_ntree_structure_component_definition(NTree* tree, const char* target)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    for (int i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == UDA_TYPE_UNKNOWN &&
            STR_EQUALS(tree->userdefinedtype->compoundfield[i].type, target)) {
            return tree;
        }
    }

    for (int i = 0; i < tree->branches; i++) {
        NTree* next;
        if ((next = find_ntree_structure_component_definition(tree->children[i], target)) != nullptr) {
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
NTree* find_ntree_structure_class(NTree* tree, int cls)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    if (tree->userdefinedtype->idamclass == cls) {
        return tree;
    }

    for (int i = 0; i < tree->branches; i++) {
        NTree* next;
        if ((next = find_ntree_structure_class(tree->children[i], cls)) != nullptr) {
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
int max_count_vlen_structure_array(NTree* tree, const char* target, int reset)
{
    static unsigned int count = 0;
    if (reset) {
        count = 0;
    }

    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    if (tree->userdefinedtype->idamclass == UDA_TYPE_VLEN && STR_EQUALS(tree->userdefinedtype->name, target)) {
        auto vlen = (VLenType*)tree->data;
        if (vlen->len > count) {
            count = vlen->len;
        }
    }

    for (int i = 0; i < tree->branches; i++) {
        count = (unsigned int)max_count_vlen_structure_array(tree->children[i], target, 0);
    }

    return count;
}

/** Regularise a specific VLEN structure.
 *
 * This is a public function with the whole subtree in scope.
 *
 * @param tree A pointer to a parent tree node. If nullptr the root node is assumed.
 * @param target The name of the VLEN Structure Definition.
 * @param count The maximum count size for the VLEN data arrays.
 * @return An integer returning an error code: 0 => OK.
 */
int uda::structures::regularise_vlen_structures(UserDefinedTypeList* user_defined_type_list,
                                                LogMallocList* log_malloc_list, NTree* tree,
                                                const char* target, unsigned int count)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    int resetBranches = 0;
    int size = 0;
    void* newnew = nullptr;

    if (tree->userdefinedtype->idamclass == UDA_TYPE_VLEN && STR_EQUALS(tree->userdefinedtype->name, target)) {
        auto vlen = (VLenType*)tree->data;

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
        UserDefinedType* child = static_cast<UserDefinedType*>(
            find_user_defined_type(user_defined_type_list, tree->userdefinedtype->compoundfield[1].type, 0));
        vlen->data = realloc(vlen->data, count * child->size); // Expand Heap to regularise
        newnew = vlen->data;
        size = child->size;
        change_malloc(log_malloc_list, old, vlen->data, count, child->size, child->name);
        tree->data = (void*)vlen;

        // Write new data array to Original Tree Nodes

        for (unsigned int i = 0; i < vlen->len; i++) {
            tree->children[i]->data = (char*)newnew + i * size;
        }

        resetBranches = vlen->len; // Flag requirement to add extra tree nodes
    }

    for (int i = 0; i < tree->branches; i++) {
        int rc;
        if ((rc = regularise_vlen_structures(user_defined_type_list, log_malloc_list, tree->children[i], target, count)) != 0) {
            return rc;
        }
    }

    // Update branch count and add new Child nodes with New data array

    if (resetBranches > 0) {
        tree->branches = count; // Only update once all True children have been regularised
        auto old = (VOIDTYPE)tree->children;
        tree->children = (NTree**)realloc(tree->children, count * sizeof(void*));

        unsigned int ui;
        for (ui = (unsigned int)resetBranches; ui < count; ui++) {
            tree->children[ui] = (NTree*)malloc(sizeof(NTree));
            add_malloc(log_malloc_list, tree->children[ui], 1, sizeof(NTree), "NTree");
            memcpy(tree->children[ui], tree->children[0], sizeof(NTree));
        }
        change_malloc(log_malloc_list, old, tree->children, count, sizeof(NTree), "NTree");

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
int uda_regularise_vlen_data(UserDefinedTypeList* user_defined_type_list, LogMallocList* log_malloc_list, NTree* tree)
{
    int rc = 0, count = 0;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    NTree* nt = nullptr;
    do {
        if ((nt = find_ntree_structure_class(tree, UDA_TYPE_VLEN)) != nullptr) {
            count = max_count_vlen_structure_array(tree, nt->userdefinedtype->name, 1);
            if (count > 0) {
                rc = regularise_vlen_structures(user_defined_type_list, log_malloc_list, tree, nt->userdefinedtype->name, count);
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
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Count of structured data array elements.
 */
int uda::structures::get_node_structure_data_count(LogMallocList* log_malloc_list, NTree* tree)
{
    int count, size;
    const char* type;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }
    find_malloc(log_malloc_list, (void*)&tree->data, &count, &size, &type);
    return count;
}

/** Return the Size (bytes) of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the Size (bytes) of the structured data array.
 */
int get_node_data_structure_data_size(LogMallocList* log_malloc_list, NTree* tree)
{
    int count, size;
    const char* type;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }
    find_malloc(log_malloc_list, (void*)&tree->data, &count, &size, &type);
    return size;
}

/** Return the rank of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return The rank of the structured data array.
 */
int get_node_structure_data_rank(LogMallocList* log_malloc_list, NTree* tree)
{
    int count, size, rank;
    int* shape;
    const char* type;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }
    find_malloc2(log_malloc_list, (void*)&tree->data, &count, &size, &type, &rank, &shape);
    return rank;
}

/** Return the shape of the structured data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return A pointer to the integer shape array of the structured data array.
 */
int* get_node_structure_data_shape(LogMallocList* log_malloc_list, NTree* tree)
{
    int count, size, rank;
    int* shape;
    const char* type;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }

    if (tree->parent != nullptr) {
        int branches = tree->parent->branches;
        if (branches > 1) {
            for (int i = 0; i < branches; i++) {
                if (tree->parent->children[i] == tree) {
                    break;
                }
            }
        }
    }

    find_malloc2(log_malloc_list, (void*)&tree->data, &count, &size, &type, &rank, &shape);
    return shape;
}

/** Return a pointer to the structured data type name of the data array attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return the data type name of the structured data array.
 */
const char* get_node_structure_data_data_type(LogMallocList* log_malloc_list, NTree* tree)
{
    int count, size;
    const char* type = nullptr;
    if (tree == nullptr) {
        tree = get_full_ntree();
    }
    find_malloc(log_malloc_list, (void*)&tree->data, &count, &size, &type);
    return type;
}

/** Return a pointer to the data attached to this tree node.
 *
 * This is a public function with the current tree node only in scope.
 *
 * @param tree A pointer to a tree node. If nullptr the root node is assumed.
 * @return A void pointer to the data .
 */
void* get_node_structure_data(NTree* tree)
{
    if (tree == nullptr) {
        tree = get_full_ntree();
    }
    return tree->data;
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
void uda::structures::print_image(const char* image, int imagecount)
{
    int next = 0;
    if (image == nullptr || imagecount == '\0') {
        return;
    }
    while (next < imagecount) {
        UDA_LOG(UDA_LOG_DEBUG, "{}", &image[next]);
        next = next + (int)strlen(&image[next]) + 1;
    }
}

template <typename T> struct TypeNamer {
    static constexpr const char* Name = "";
};

template <> struct TypeNamer<short> {
    static constexpr const char* Name = "short";
};

template <> struct TypeNamer<int> {
    static constexpr const char* Name = "int";
};

template <> struct TypeNamer<long long> {
    static constexpr const char* Name = "long long";
};

template <> struct TypeNamer<unsigned short> {
    static constexpr const char* Name = "unsigned short";
};

template <> struct TypeNamer<unsigned int> {
    static constexpr const char* Name = "unsigned int";
};

template <> struct TypeNamer<unsigned long long> {
    static constexpr const char* Name = "unsigned long long";
};

template <> struct TypeNamer<float> {
    static constexpr const char* Name = "float";
};

template <> struct TypeNamer<double> {
    static constexpr const char* Name = "double";
};

template <> struct TypeNamer<char> {
    static constexpr const char* Name = "char";
};

template <> struct TypeNamer<unsigned char> {
    static constexpr const char* Name = "unsigned char";
};

template <> struct TypeNamer<char*> {
    static constexpr const char* Name = "STRING";
};

template <> struct TypeNamer<void> {
    static constexpr const char* Name = "void";
};

template <typename T>
void defineField(CompoundField* field, const char* name, const char* desc, int rank, int* shape, bool is_pointer, bool is_scalar)
{
    copy_string(TypeNamer<T>::Name, field->type, MAXELEMENTNAME);
    if (is_pointer) {
        if (!is_scalar) {
            strcat(field->type, " *");
        }
        field->count = 1;
        field->rank = 0;
        field->shape = nullptr;
        field->pointer = 1;
        field->size = sizeof(T*);
    } else {
        if constexpr (!std::is_same_v<T, void>) {
            int count = 1;
            for (int i = 0; i < rank; ++i) {
                count *= shape[i];
            }

            field->pointer = 0;
            field->count = count;
            field->size = count * sizeof(T);
            field->rank = rank;
            field->shape = (int*) malloc(rank * sizeof(int));
            for (int i = 0; i < rank; ++i) {
                field->shape[i] = shape[i];
            }
        }
    }
    snprintf(field->desc, MAXELEMENTNAME, "[%s %s] %s", field->type, name, desc);
}

//----------------------------------------------------------------------------------------------------------
/**User defined structure field definition for common types
 *
 * This is a public function
 *
 * @param field The user defined structure's field to be populated
 * @param name Name of the structure's field
 * @param desc Description of the fields contents
 * @param offset Current field byte offset from the start of the structure. Ppdated on return.
 * @param type_id Enumerated key indicating the type of data field, e.g. float array
 * @return Void
 */
void uda::structures::defineField(CompoundField* field, const char* name, const char* desc, int* offset,
                                  unsigned short type_id, int rank, int* shape, bool is_pointer, bool is_scalar)
{
    init_compound_field(field);
    copy_string(name, field->name, MAXELEMENTNAME);
    field->atomictype = type_id;

    switch (type_id) {
        case UDA_TYPE_DOUBLE:
            ::defineField<double>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_FLOAT:
            ::defineField<float>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_CHAR:
            ::defineField<char>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_UNSIGNED_CHAR:
            ::defineField<unsigned char>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_SHORT:
            ::defineField<short>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_UNSIGNED_SHORT:
            ::defineField<short>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_INT:
            ::defineField<int>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_UNSIGNED_INT:
            ::defineField<int>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_LONG64:
            ::defineField<long long>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_UNSIGNED_LONG64:
            ::defineField<long long>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_STRING:
            ::defineField<char*>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
        case UDA_TYPE_VOID:
            ::defineField<void>(field, name, desc, rank, shape, is_pointer, is_scalar);
            break;
    }

    if (type_id != UDA_TYPE_STRING) {
        field->offset = (int)new_offset(*offset, field->type);
        field->offpad = (int)padding(*offset, field->type);
        field->alignment = get_alignment_of(field->type);
    } else {
        field->offset = (int)new_offset((size_t)*offset, "char *");
        field->offpad = (int)padding((size_t)*offset, "char *");
        field->alignment = get_alignment_of("char *");
    }

    *offset = field->offset + field->size; // Next Offset
}

void uda::structures::defineUserTypeField(CompoundField* field, const char* name, const char* desc, int* offset,
                                          int rank, int* shape, UserDefinedType* user_type, bool is_pointer)
{
    init_compound_field(field);
    strcpy(field->name, name);

    field->atomictype = UDA_TYPE_UNKNOWN;

    strcpy(field->type, user_type->name);
    strcpy(field->desc, desc);

    field->pointer = (int)is_pointer;

    if (is_pointer || rank == 0) {
        field->rank = 0;
        field->shape = nullptr;
        field->count = 1;
    } else {
        field->rank = rank;
        field->shape = (int*)malloc(rank * sizeof(int));
        int count = 1;
        for (int i = 0; i < rank; ++i) {
            field->shape[i] = shape[i];
            count *= shape[i];
        }
        field->count = count;
    }

    field->size = field->count * user_type->size;
    field->offset = (int)new_offset(*offset, field->type);
    field->offpad = (int)padding(*offset, field->type);
    field->alignment = get_alignment_of(field->type);

    *offset = field->offset + field->size; // Next Offset
}

/** Return a Pointer to a string array containing the hierarchical names of structure components.
 *
 * @param target The name of a User Defined Structure element (case sensitive).
 * @param ntargets A returned count of the number of names in the returned list.
 * @return the list of structure component names.
 */
char** uda::structures::parse_target(const char* target, int* ntargets)
{
    char** targetlist = nullptr;
    char *buffer = nullptr, *work, *p;
    buffer = (char*)malloc((strlen(target) + 1) * sizeof(char));
    strcpy(buffer, target);
    work = buffer;
    *ntargets = 0;
    if (((p = strchr(work, '.')) != nullptr) || (p = strchr(work, '/')) != nullptr) {
        p[0] = '\0';
        targetlist = (char**)realloc((void*)targetlist, (*ntargets + 1) * sizeof(char*));
        targetlist[0] = (char*)malloc((strlen(work) + 1) * sizeof(char));
        strcpy(targetlist[0], work);
        work = &p[1];
        *ntargets = 1;
        do {
            targetlist = (char**)realloc((void*)targetlist, (*ntargets + 1) * sizeof(char*));
            if (((p = strchr(work, '.')) != nullptr) || (p = strchr(work, '/')) != nullptr) {
                p[0] = '\0';
                targetlist[*ntargets] = (char*)malloc((strlen(work) + 1) * sizeof(char));
                strcpy(targetlist[*ntargets], work);
                work = &p[1];
            } else {
                targetlist[*ntargets] = (char*)malloc((strlen(work) + 1) * sizeof(char));
                strcpy(targetlist[*ntargets], work);
            }
            *ntargets = *ntargets + 1;
        } while (p != nullptr);
    }
    if (*ntargets == 0) { // Return the target even if not hierarchical
        *ntargets = 1;
        targetlist = (char**)malloc(sizeof(char*));
        targetlist[0] = buffer;
    }
    if (*ntargets > 1 && buffer != nullptr) {
        free(buffer);
    }
    return targetlist;
}