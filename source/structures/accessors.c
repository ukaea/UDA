//---------------------------------------------------------------------------------------------
// User Accessor functions to General Data Structures
//---------------------------------------------------------------------------------------------
// Locating Data: The whole sub-tree is in scope
//
// There are 6 node location methods:
// The NTREE data structure can be identified either by its name, its user defined type name, its user defined class id,
// or by the memory address of its data.
// It can also be identified by targeting the user defined type by a member name or a member type (user defined type only)
//
// Two types:
// A) Return the Tree Node whose data structure type contains a named element or member. This may be
//    either a structure or an atomic typed element.
//
// B) Return the Tree Node containing a named Data Structure (node will be a child of the node located using A).
//    The hierarchical name must consist only of structure types. Identifying data by type may fail as types are named uniquely: same 'type' but different type name!
//
// Hierarchical names can have one of two forms: a.b.c or a/b/c. The latter can also begin with an optional '/'.
//
// Function naming conventions:
//
// Component means named data member. Not the name of the structure type.
//
// findNTreeStructureComponent              data structure member name
// findNTreeStructureComponentDefinition    data structure member's user defined type name
// findNTreeStructure                       tree node name
// findNTreeStructureMalloc                 memory address of the data
// findNTreeStructureDefinition             user defined type name
//
// findNTreeChildStructureComponent         data structure member name within the child nodes only
// findNTreeChildStructure                  tree node name within the child nodes only
//
// idam_findNTreeStructureClass             user defined type class id
// idam_maxCountVlenStructureArray
// idam_regulariseVlenStructures
// idam_regulariseVlenData
//
// getNodeStructureDataCount
// getNodeStructureDataSize
// getNodeStructureDataRank
// getNodeStructureDataShape
// getNodeStructureDataDataType
// getNodeStructureData
//
// printImage
//
#include "accessors.h"

#include <idamLog.h>
#include <stdlib.h>

#include "struct.h"

/** Find (search type A) the first Tree Node with a data structure type containing a named element/member.
*
* This is a private function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node that defines the start (root) of the sub-tree. If NULL the root node is assumed. 
* @param target The name of the data structure's element/member (case sensitive) 
* This element may be either a data structure or an atomic typed element. The first occurance of the name is selected. 
* @return the Tree Node containing the named element.
*/

NTREE * findNTreeStructureComponent1(NTREE * ntree, const char * target)
{
    int i;
    NTREE * child = NULL;
    char * p;

    if (ntree == NULL) ntree = fullNTree;

// Single entity name expected - test

    if (((p = strchr(target, '.')) != NULL) || (p = strchr(target, '/')) != NULL) return NULL;

// Is it the name of the current tree node?

    if (!strcmp(ntree->name, target)) return (ntree);

// Recursively Search Child nodes for structured type data

    for (i = 0; i < ntree->branches; i++) {
        if ((child = findNTreeStructureComponent1(ntree->children[i], target)) != NULL) return (child);
    }

// Search the components of this node for atomic types

    for (i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (!strcmp(ntree->userdefinedtype->compoundfield[i].name, target) &&
            ntree->userdefinedtype->compoundfield[i].atomictype != TYPE_UNKNOWN)
            return (ntree);
    }

    return NULL;    // Not found
}

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
/* 
If a hierarchical set of names is passed, all except the last must be a structured type
The last may be either a structured or an atomic type
Search all but the last on the child tree nodes.
The first name must be searched for down the tree from the root or starting node
All subsequent names must be within child nodes unless the last name
*/
NTREE * findNTreeStructureComponent2(NTREE * ntree, const char * target, const char ** lastname)
{
    int i, j;
    NTREE * child = NULL, * found = NULL;
    char * p;

    if (ntree == NULL) ntree = fullNTree;

// Is the hierarchical name of the form: a.b.c or a/b/c

    if (((p = strchr(target, '.')) != NULL) || (p = strchr(target, '/')) != NULL) {
        int ntargets;
        char ** targetlist = NULL;
        child = ntree;

        targetlist = parseTarget(target, &ntargets);    // Deconstruct the Name and search for each hierarchy group

        *lastname = targetlist[ntargets - 1];        // Preserve the last element name

// Search recursively for the first name 

        if ((child = findNTreeStructureComponent1(child, targetlist[0])) == NULL) return NULL;        // Not found

// Search child nodes for all names but the last name

        for (i = 1; i < ntargets - 1; i++) {
            found = NULL;
            for (j = 0; j < child->branches; j++) {
                if (!strcmp(child->children[j]->name, targetlist[i])) {
                    found = child->children[j];
                    break;
                }
            }
            if (found == NULL) return NULL;        // Not found
            child = found;
        }

        addMalloc((void *) targetlist[ntargets - 1], (int) strlen(targetlist[ntargets - 1]) + 1, sizeof(char), "char");
        for (i = 0; i < ntargets - 1; i++) free((void *) targetlist[i]);    // Free all others
        free((void *) targetlist);                    // Free the list

// Search the user defined type definition for the last name - return if an atomic type

        for (i = 0; i < child->userdefinedtype->fieldcount; i++) {
            if (!strcmp(child->userdefinedtype->compoundfield[i].name, targetlist[ntargets - 1]) &&
                child->userdefinedtype->compoundfield[i].atomictype != TYPE_UNKNOWN)
                return (child);  // Atomic type found
        }

// Search child nodes for structured types

        for (j = 0; j < child->branches; j++) {
            if (!strcmp(child->children[j]->name, targetlist[ntargets - 1])) return child->children[j];
        }

        return NULL;    // Not Found
    }

// Recursively search using the single name passed

    *lastname = target;

    if ((child = findNTreeStructureComponent1(ntree, target)) != NULL) return child;        // Found

    return NULL;        // Not found

}

NTREE * findNTreeStructureComponent2Legacy(NTREE * ntree, const char * target, const char ** lastname)
{

    int i;
    NTREE * child = NULL;
    char * p;

    if (ntree == NULL) ntree = fullNTree;

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if (((p = strchr(target, '.')) != NULL) || (p = strchr(target, '/')) != NULL) {
        int ntargets;
        char ** targetlist = NULL;
        child = ntree;

        targetlist = parseTarget(target, &ntargets); // Deconstruct the Name and search for each hierarchy group

        for (i = 0; i < ntargets; i++) { // Drill Down to the requested named structure element

            if (i < ntargets - 1) {
                child = findNTreeStructure2(child, targetlist[i], lastname);        // Match to tree branch node name
            } else {
                child = findNTreeStructureComponent2(child, targetlist[i],
                                                     lastname);    // Confirm the target is a structure element
            }

            if (child == NULL) break;
        }

        *lastname = targetlist[ntargets - 1];   // Preserve the last element name

        addMalloc((void *) targetlist[ntargets - 1], (int) strlen(targetlist[ntargets - 1]) + 1, sizeof(char), "char");
        for (i = 0; i < ntargets - 1; i++) free((void *) targetlist[i]); // Free all others
        free((void *) targetlist);     // Free the list

        return child; // Always the last node you look in !
    }

    // Search using a single hierarchical group name

    *lastname = target;

    for (i = 0; i < ntree->userdefinedtype->fieldcount; i++) {
        if (!strcmp(ntree->userdefinedtype->compoundfield[i].name, target)) {
            return ntree;  // Test all structure components
        }
    }

    // Search Child nodes

    for (i = 0; i < ntree->branches; i++) {
        if ((child = findNTreeStructureComponent(ntree->children[i], target)) != NULL) {
            return child;
        }
    }

    return NULL;
}

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
NTREE * findNTreeStructure2(NTREE * ntree, const char * target, const char ** lastname)
{
    int i;
    NTREE * child = NULL;
    NTREE * test = NULL;
    char * p;
    if (ntree == NULL) ntree = fullNTree;

    // Is the hierarchical name of the form: a.b.c or a/b/c

    if (((p = strchr(target, '.')) != NULL) || (p = strchr(target, '/')) != NULL) {
        int ntargets;
        char ** targetlist = NULL;
        child = ntree;

        targetlist = parseTarget(target, &ntargets); // Deconstruct Name and search for each hierarchy group

        for (i = 0; i < ntargets; i++) {    // Drill Down to requested named structure element
            if (i < ntargets - 1) {
                child = findNTreeStructure2(child, targetlist[i], lastname);
            } else {
                if ((test = findNTreeStructure2(child, targetlist[i], lastname)) ==
                    NULL) { // Last element may not be a structure
                    if ((test = findNTreeStructureComponent2(child, targetlist[i], lastname)) == NULL) child = NULL;
                } else {
                    child = test;
                }
            }
            if (child == NULL) {
                //addIdamError(&idamerrorstack, CODEERRORTYPE, "findNTreeStructure2", 999, "Unable to locate a named structure data tree branch!");
                //child = NULL;
                break;
            }
        }

        *lastname = targetlist[ntargets - 1];   // Preserve the last element name

        addMalloc((void *) targetlist[ntargets - 1], (int) strlen(targetlist[ntargets - 1]) + 1, sizeof(char), "char");
        for (i = 0; i < ntargets - 1; i++) free((void *) targetlist[i]); // Free all others
        free((void *) targetlist);     // Free the list

        return child; // Always the last node you look in !
    }

    // Search using a single hierarchical group name

    *lastname = target;

    if (!strcmp(ntree->name, target)) return ntree;  // Test the current Tree Node

    // Search Child nodes

    for (i = 0; i < ntree->branches; i++) {
        if (!strcmp(ntree->children[i]->name, target)) return ntree->children[i];
    }

    return NULL;
}

/** Find (search type A) and return a Pointer to the Data Tree Node with a data structure that contains a named element.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @return the Data Tree Node.
*/
NTREE * findNTreeStructureComponent(NTREE * ntree, const char * target)
{
    const char * lastname;
    return findNTreeStructureComponent2(ntree, target, &lastname);
}

/** Find (search type A) and return a Pointer to the Child Data Tree Node with a data structure that contains a named element. 
*
* This is a public function with the child sub-trees in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the structure element or member (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* This element may be either a structure itself or an atomic typed element.
* @return the Data Tree Node.
*/
NTREE * findNTreeChildStructureComponent(NTREE * ntree, const char * target)
{
    int i;
    NTREE * child = NULL;

    if (ntree == NULL) ntree = fullNTree;

    // Search each child branch

    for (i = 0; i < ntree->branches; i++) {
        if ((child = findNTreeStructureComponent(ntree->children[i], target)) != NULL) return child;
    }

    return NULL;
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @return the Data Tree Node.
*/
NTREE * findNTreeStructure(NTREE * ntree, const char * target)
{
    const char * lastname;
    return findNTreeStructure2(ntree, target, &lastname);
}

/** Find (search type B) and return a Pointer to the named Data Tree Node with a data structure of the same name.
*
* This is a public function with child sub-trees in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed. 
* @param target The name of the Structure (case sensitive) using a hierachical naming syntax a.b.c or a/b/c.
* @return the child Data Tree Node.
*/
NTREE * findNTreeChildStructure(NTREE * ntree, const char * target)
{
    int i;
    NTREE * child = NULL;

    if (ntree == NULL) ntree = fullNTree;

// Search each child branch

    for (i = 0; i < ntree->branches; i++) {
        if ((child = findNTreeStructure(ntree->children[i], target)) != NULL) return child;
    }

    return NULL;
}

/** Find and return a Pointer to a Data Tree Node with a data structure located at a specific memory location.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed.
* @param data The heap address of the data.
* @return the Data Tree Node.
*/
NTREE * findNTreeStructureMalloc(NTREE * ntree, void * data)
{
    int i;
    NTREE * next;
    if (ntree == NULL) ntree = fullNTree;
    if (data == ntree->data) return ntree;
    for (i = 0; i < ntree->branches; i++)
        if ((next = findNTreeStructureMalloc(ntree->children[i], data)) != NULL) {
            return next;
        }
    return NULL;
}

/** Locate a tree node with structured data having the specified Structure Definition name.
*
* This is a public function with the whole sub-tree in scope.
*
* @param ntree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/
NTREE * findNTreeStructureDefinition(NTREE * ntree, const char * target)
{
    int i;
    NTREE * child = NULL;
    if (ntree == NULL) ntree = fullNTree;

    // Is the hierarchical name of the form: a.b.c or a/b/c

    char * p;
    if (((p = strchr(target, '.')) != NULL) || (p = strchr(target, '/')) != NULL) {
        int ntargets;
        char ** targetlist = NULL;
        child = ntree;

        targetlist = parseTarget(target, &ntargets); // Deconstruct the Name and search for each hierarchy group

        for (i = 0; i < ntargets; i++) { // Drill Down to requested named structure type
            if ((child = findNTreeStructureDefinition(child, targetlist[i])) == NULL) break;
        }

        for (i = 0; i < ntargets; i++) free((void *) targetlist[i]);    // Free all entries
        free((void *) targetlist);                    // Free the list

        return child;
    }

    if (!strcmp(ntree->userdefinedtype->name, target)) return ntree;
    for (i = 0; i < ntree->branches; i++)
        if ((child = findNTreeStructureDefinition(ntree->children[i], target)) != NULL)return child;
    return NULL;
}

NTREE * xfindNTreeStructureDefinition(NTREE * tree, const char * target)
{
    int i;
    NTREE * next;
    if (tree == NULL) tree = fullNTree;
    if (!strcmp(tree->userdefinedtype->name, target)) return tree;
    for (i = 0; i < tree->branches; i++)
        if ((next = findNTreeStructureDefinition(tree->children[i], target)) != NULL)return next;
    return NULL;
}

/** Locate a tree node with structured data having the specified Structure Definition name.
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the Structure Definition.
* @return A pointer to the First tree node found with the targeted structure definition.
*/
NTREE * findNTreeStructureComponentDefinition(NTREE * tree, const char * target)
{
    int i;
    NTREE * next;
    if (tree == NULL) tree = fullNTree;
    for (i = 0; i < tree->userdefinedtype->fieldcount; i++) {
        if (tree->userdefinedtype->compoundfield[i].atomictype == TYPE_UNKNOWN &&
            !strcmp(tree->userdefinedtype->compoundfield[i].type, target)) {
            return tree;
        }
    }
    for (i = 0; i < tree->branches; i++) {
        if ((next = findNTreeStructureComponentDefinition(tree->children[i], target)) != NULL) {
            return next;
        }
    }
    return NULL;
}

/** Locate a tree node with structured data having a Specific Structure Class.
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.
* @param class The Structure Class, e.g., TYPE_VLEN.
* @return A pointer to the First tree node found with the targeted structure class.
*/
NTREE * idam_findNTreeStructureClass(NTREE * tree, int class)
{
    int i;
    NTREE * next;
    if (tree == NULL) tree = fullNTree;
    if (tree->userdefinedtype->idamclass == class) {
        return tree;
    }
    for (i = 0; i < tree->branches; i++) {
        if ((next = idam_findNTreeStructureClass(tree->children[i], class)) != NULL) {
            return next;
        }
    }
    return NULL;
}

/** Identify the largest count of a Variable Length Array with a given structure type.
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the VLEN Structure Definition.
* @param reset Reset the counbter to zero.
* @return An integer returning the maximum count value.
*/
int idam_maxCountVlenStructureArray(NTREE * tree, const char * target, int reset)
{

    struct VLENTYPE
    {
        unsigned int len;
        void * data;
    };
    typedef struct VLENTYPE VLENTYPE;

    int i;
    static int count = 0;
    if (reset) count = 0;
    if (tree == NULL) tree = fullNTree;
    if (tree->userdefinedtype->idamclass == TYPE_VLEN && !strcmp(tree->userdefinedtype->name, target)) {
        VLENTYPE * vlen = (VLENTYPE *) tree->data;
        if (vlen->len > count) count = vlen->len;
    }
    for (i = 0; i < tree->branches; i++) count = idam_maxCountVlenStructureArray(tree->children[i], target, 0);
    return count;
}

/** Regularise a specific VLEN structure.
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.
* @param target The name of the VLEN Structure Definition.
* @param count The maximum count size for the VLEN data arrays.
* @return An integer returning an error code: 0 => OK.
*/
int idam_regulariseVlenStructures(NTREE * tree, const char * target, int count)
{

    struct VLENTYPE
    {
        unsigned int len;
        void * data;
    };
    typedef struct VLENTYPE VLENTYPE;

    int i, rc = 0, size = 0, resetBranches = 0;
    void * old = NULL, * newnew = NULL;
    if (tree == NULL) tree = fullNTree;
    if (tree->userdefinedtype->idamclass == TYPE_VLEN && !strcmp(tree->userdefinedtype->name, target)) {
        VLENTYPE * vlen = (VLENTYPE *) tree->data;

        // VLEN stuctures have only two fields: len and data
        // Need the size of the data component

        if (vlen->len == 0) return 1;   // No data!
        if (vlen->len > count) return 1;   // Incorrect count value

        // VLEN Memory is contiguous so re-allocate: regularise by expanding to a consistent array size (No longer a VLEN!)

        old = (void *) vlen->data;
        USERDEFINEDTYPE * child = findUserDefinedType(tree->userdefinedtype->compoundfield[1].type, 0);
        vlen->data = (void *) realloc((void *) vlen->data, count * child->size);    // Expand Heap to regularise
        newnew = (void *) vlen->data;
        size = child->size;
        changeMalloc(old, vlen->data, count, child->size, child->name);
        tree->data = (void *) vlen;

        // Write new data array to Original Tree Nodes

        for (i = 0; i < vlen->len; i++) tree->children[i]->data = newnew + i * size;

        resetBranches = vlen->len;  // Flag requirement to add extra tree nodes

    }
    for (i = 0; i < tree->branches; i++)
        if ((rc = idam_regulariseVlenStructures(tree->children[i], target, count)) != 0)return rc;

    // Update branch count and add new Child nodes with New data array

    if (resetBranches > 0) {
        tree->branches = count;   // Only update once all True children have been regularised
        old = (void *) tree->children;
        tree->children = (NTREE **) realloc((void *) tree->children, count * sizeof(void *));
        for (i = resetBranches; i < count; i++) {
            tree->children[i] = (NTREE *) malloc(sizeof(NTREE));
            addMalloc((void *) tree->children[i], 1, sizeof(NTREE), "NTREE");
            memcpy(tree->children[i], tree->children[0], sizeof(NTREE));
        }
        changeMalloc(old, (void *) tree->children, count, sizeof(NTREE), "NTREE");

        // Update All new Child Nodes with array element addresses

        for (i = resetBranches; i < count; i++)
            memcpy(newnew + i * size, newnew, size);   // write extra array items: use the first array element
        for (i = resetBranches; i < count; i++) tree->children[i]->data = newnew + i * size;

    }


    return rc;
}


/** Regularise the Shape of All VLEN structured data arrays in the data tree: necessary for accessing in some languages, e.g. IDL.
*
* This is a public function with the whole sub-tree in scope.
*
* @param tree A pointer to a parent tree node. If NULL the root node is assumed.
* @return An integer returning an error code: 0 => OK.
*/
int idam_regulariseVlenData(NTREE * tree)
{
    int rc = 0, count = 0;
    NTREE * nt = NULL;
    if (tree == NULL) tree = fullNTree;
    do {
        if ((nt = idam_findNTreeStructureClass(tree, TYPE_VLEN)) != NULL) {
            count = idam_maxCountVlenStructureArray(tree, nt->userdefinedtype->name, 1);
            if (count > 0) rc = idam_regulariseVlenStructures(tree, nt->userdefinedtype->name, count);
            if (rc != 0) return rc;
            nt->userdefinedtype->idamclass = TYPE_COMPOUND;   // Change the class to 'regular compound structure'
        }
    } while (nt != NULL);
    return 0;
}

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
int getNodeStructureDataCount(NTREE * ntree)
{
    int count, size;
    char * type;
    if (ntree == NULL) ntree = fullNTree;
    findMalloc((void *) &ntree->data, &count, &size, &type);
    return count;
}

/** Return the Size (bytes) of the structured data array attached to this tree node.
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed.
* @return the Size (bytes) of the structured data array.
*/
int getNodeStructureDataSize(NTREE * ntree)
{
    int count, size;
    char * type;
    if (ntree == NULL) ntree = fullNTree;
    findMalloc((void *) &ntree->data, &count, &size, &type);
    return size;
}

/** Return the rank of the structured data array attached to this tree node.
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed.
* @return The rank of the structured data array.
*/
int getNodeStructureDataRank(NTREE * ntree)
{
    int count, size, rank;
    int * shape;
    char * type;
    if (ntree == NULL) ntree = fullNTree;
    findMalloc2((void *) &ntree->data, &count, &size, &type, &rank, &shape);
    return rank;
}

/** Return the shape of the structured data array attached to this tree node.
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed.
* @return A pointer to the integer shape array of the structured data array.
*/
int * getNodeStructureDataShape(NTREE * ntree)
{
    int count, size, rank;
    int * shape;
    char * type;
    if (ntree == NULL) ntree = fullNTree;

    if (ntree->parent != NULL) {
        int i, branches = ntree->parent->branches;
        fprintf(stdout, "\n%p Parent Name %s\n", ntree, ntree->parent->name);
        fprintf(stdout, "%p Parent Type %s\n", ntree, ntree->parent->userdefinedtype->name);
        fprintf(stdout, "%p Siblings %d\n", ntree, branches);
        if (branches > 1) {
            int id = 0;
            for (i = 0; i < branches; i++) {
                if (ntree->parent->children[i] == ntree) {
                    id = i;
                    break;
                }
            }
            fprintf(stdout, "%p Child ID %d\n", ntree, id);
        }
        fflush(stdout);
    }

    findMalloc2((void *) &ntree->data, &count, &size, &type, &rank, &shape);
    return shape;
}

/** Return a pointer to the structured data type name of the data array attached to this tree node.
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed.
* @return the data type name of the structured data array.
*/
char * getNodeStructureDataDataType(NTREE * ntree)
{
    int count, size;
    char * type = NULL;
    if (ntree == NULL) ntree = fullNTree;
    findMalloc((void *) &ntree->data, &count, &size, &type);
    return type;
}

/** Return a pointer to the data attached to this tree node.
*
* This is a public function with the current tree node only in scope.
*
* @param ntree A pointer to a tree node. If NULL the root node is assumed.
* @return A void pointer to the data .
*/
void * getNodeStructureData(NTREE * ntree)
{
    if (ntree == NULL) ntree = fullNTree;
    return (void *) ntree->data;
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
void printImage(char * image, int imagecount)
{
    int next = 0;
    if (image == NULL || imagecount == '\0') return;
    while (next < imagecount) {
        idamLog(LOG_DEBUG, "%s", &image[next]);
        next = next + (int) strlen(&image[next]) + 1;
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
* @param TypeId Enumerated key indicating the type of data field, e.g. float array
* @return Void
*/
void defineField(COMPOUNDFIELD * field, char * name, char * desc, int * offset, unsigned short TypeId)
{
    initCompoundField(field);
    strcpy(field->name, name);

    field->pointer = 0;  // default for scalar values
    field->count = 1;

    if (TypeId == SCALARDOUBLE) {  // Single scalar double
        field->atomictype = TYPE_DOUBLE;
        strcpy(field->type, "double");
        sprintf(field->desc, "[double %s] %s", name, desc);
        field->size = field->count * sizeof(double);
    } else if (TypeId == ARRAYDOUBLE) {  // arbitrary number array of doubles
        field->atomictype = TYPE_DOUBLE;
        strcpy(field->type, "double *");
        sprintf(field->desc, "[double *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(double *);
    } else if (TypeId == SCALARFLOAT) {  // Single scalar float
        field->atomictype = TYPE_FLOAT;
        strcpy(field->type, "float");
        sprintf(field->desc, "[float %s] %s", name, desc);
        field->size = field->count * sizeof(float);
    } else if (TypeId == ARRAYFLOAT) {  // arbitrary number array of floats
        field->atomictype = TYPE_FLOAT;
        strcpy(field->type, "float *");
        sprintf(field->desc, "[float *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(float *);
    } else if (TypeId == SCALARLONG64) {  // Single scalar 8 byte integer
        field->atomictype = TYPE_LONG64;
        strcpy(field->type, "long long");
        sprintf(field->desc, "[long long %s] %s", name, desc);
        field->size = field->count * sizeof(long long);
    } else if (TypeId == ARRAYLONG64) {  // arbitrary number array of 8 byte integers
        field->atomictype = TYPE_LONG64;
        strcpy(field->type, "long long *");
        sprintf(field->desc, "[long long *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(long long *);
    } else if (TypeId == SCALARULONG64) {  // Single scalar unsigned 8 byteinteger
        field->atomictype = TYPE_UNSIGNED_LONG64;
        strcpy(field->type, "unsigned long long");
        sprintf(field->desc, "[unsigned long long %s] %s", name, desc);
        field->size = field->count * sizeof(unsigned long long);
    } else if (TypeId == ARRAYULONG64) {  // arbitrary number array of unsigned 8 byteintegers
        field->atomictype = TYPE_UNSIGNED_LONG64;
        strcpy(field->type, "unsigned long long *");
        sprintf(field->desc, "[unsigned long long *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(unsigned long long *);
    } else if (TypeId == SCALARINT) {  // Single scalar integer
        field->atomictype = TYPE_INT;
        strcpy(field->type, "int");
        sprintf(field->desc, "[int %s] %s", name, desc);
        field->size = field->count * sizeof(int);
    } else if (TypeId == ARRAYINT) {  // arbitrary number array of integers
        field->atomictype = TYPE_INT;
        strcpy(field->type, "int *");
        sprintf(field->desc, "[int *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(int *);
    } else if (TypeId == SCALARUINT) {  // Single scalar unsigned integer
        field->atomictype = TYPE_UNSIGNED_INT;
        strcpy(field->type, "unsigned int");
        sprintf(field->desc, "[unsigned int %s] %s", name, desc);
        field->size = field->count * sizeof(unsigned int);
    } else if (TypeId == ARRAYUINT) {  // arbitrary number array of unsigned integers
        field->atomictype = TYPE_UNSIGNED_INT;
        strcpy(field->type, "unsigned int *");
        sprintf(field->desc, "[unsigned int *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(unsigned int *);
    } else if (TypeId == SCALARSHORT) {  // Single scalar short integer
        field->atomictype = TYPE_SHORT;
        strcpy(field->type, "short");
        sprintf(field->desc, "[short %s] %s", name, desc);
        field->size = field->count * sizeof(short);
    } else if (TypeId == ARRAYSHORT) {  // arbitrary number array of short integers
        field->atomictype = TYPE_SHORT;
        strcpy(field->type, "short *");
        sprintf(field->desc, "[short *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(short *);
    } else if (TypeId == SCALARUSHORT) {  // Single scalar unsigned short integer
        field->atomictype = TYPE_UNSIGNED_SHORT;
        strcpy(field->type, "unsigned short");
        sprintf(field->desc, "[unsigned short %s] %s", name, desc);
        field->size = field->count * sizeof(unsigned short);
    } else if (TypeId == ARRAYUSHORT) {  // arbitrary number array of unsigned short integers
        field->atomictype = TYPE_UNSIGNED_SHORT;
        strcpy(field->type, "unsigned short *");
        sprintf(field->desc, "[unsigned short *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(unsigned short *);
    } else if (TypeId == SCALARCHAR) {  // Single scalar byte integer
        field->atomictype = TYPE_CHAR;
        strcpy(field->type, "char");
        sprintf(field->desc, "[char %s] %s", name, desc);
        field->size = field->count * sizeof(char);
    } else if (TypeId == ARRAYCHAR) {  // arbitrary number array of byte integers (Not a string!)
        field->atomictype = TYPE_CHAR;
        strcpy(field->type, "char *");
        sprintf(field->desc, "[char *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(char *);
    } else if (TypeId == SCALARSTRING) {  // Single scalar string of arbitrary length
        field->atomictype = TYPE_STRING;
        strcpy(field->type, "STRING");
        sprintf(field->desc, "[char *%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(char *);
        field->offset = newoffset(*offset, "char *"); // must be an explicit char pointer (STRING Convention!)
        field->offpad = padding(*offset, "char *");
        field->alignment = getalignmentof("char *");
    } else if (TypeId == ARRAYSTRING) {  // arbitrary number array of strings of arbitrary length
        //Bug Fix dgm 07Jul2014: atomictype was missing!
        field->atomictype = TYPE_STRING;
        strcpy(field->type, "STRING *");
        sprintf(field->desc, "[char **%s] %s", name, desc);
        field->pointer = 1;
        field->size = field->count * sizeof(char **);
    }

    field->rank = 0;
    field->shape = NULL;

    if (TypeId != SCALARSTRING) {
        field->offset = newoffset(*offset, field->type);
        field->offpad = padding(*offset, field->type);
        field->alignment = getalignmentof(field->type);
    }

    *offset = field->offset + field->size; // Next Offset
}

