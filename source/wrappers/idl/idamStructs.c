//---------------------------------------------------------------------------------------------
// IDL User Accessor functions to General Data Structures
//---------------------------------------------------------------------------------------------

// Keyword Structure
#include <stdio.h>

#include "idl_export.h"

#include <client/accAPI.h>
#include <structures/struct.h>
#include <structures/accessors.h>
#include <clientserver/stringUtils.h>

typedef struct {
    IDL_KW_RESULT_FIRST_FIELD;
    IDL_LONG children;
    IDL_LONG verbose;
    IDL_LONG debug;
    IDL_LONG help;
} KW_RESULT;


IDL_VPTR IDL_CDECL setidamdatatree(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 1 Args: IDAM handle (long32 int)

    // calls: int setIdamDataTree(int handle) - registers the data to be accessed via the data tree accessors

    int handle, registered;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nsetidamdatatree Help\n\n"
                "Registers the Data Tree associated with the IDAM handle with the Tree Accessor Library.\n"
                "If a zero (FALSE) is returned, there is No Data Tree associated with the IDAM handle.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n\n");

        return (IDL_GettmpLong(0));               // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    if (kw.debug) {
        fprintf(stdout, "Handle: %d\n", handle);
    }

    //---------------------------------------------------------------------------------------------
    // Set the Data Tree and Return

    registered = setIdamDataTree(handle);

    if (kw.debug) {
        fprintf(stdout, "Registered: %d\n", registered);
        USERDEFINEDTYPELIST* userdefinedtypelist = getIdamUserDefinedTypeList(handle);
        printNTree(NULL, userdefinedtypelist);
    }

    return (IDL_GettmpLong(registered));
}



//===============================================================================================
// Accessors with the whole (sub) tree in scope

IDL_VPTR IDL_CDECL findidamtreestructurecomponent(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args: IDAM handle (long32 int), Node address or null (0) (long64 int) and the
    // target structure component name (string)

    // calls: NTREE *findNTreeStructureComponent(NTREE *ntree, char *target)
    // or     NTREE *findNTreeChildStructureComponent(NTREE *ntree, char *target) - if the children keyword is set

    int handle;
    NTREE* ntree;
    char* target;

    IDL_MEMINT ntreeFound;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"CHILDREN", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(children)},
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.children = 0;
    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nfindIdamTreeStructureComponent Help\n\n"
                "Returns the tree node address containing data with a specific data structure component.\n"
                "If a null address is returned, no component was located.\n\n"
                "Arguments:\n"
                "\t(long)    handle - the IDAM data handle.\n"
                "\t(long64)  tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n"
                "\t(string)  target - the name of the Structure Component required.\n\n"
                "\t(keyword) children - Constrain the search scope to child node only.\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   String

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_STRING(argv[2]);
    IDL_ENSURE_SCALAR(argv[2]);
    target = IDL_STRING_STR(&(argv[2]->value.str));

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    if (!kw.children) {
        ntreeFound = (IDL_MEMINT) findNTreeStructureComponent(logmalloclist, ntree, target);
    } else {
        ntreeFound = (IDL_MEMINT) findNTreeChildStructureComponent(logmalloclist, ntree, target);
    }

    if (kw.debug) {
        fprintf(stdout, "+++ findIdamTreeStructureComponent +++\n");
        fprintf(stdout, "Handle: %d\n", handle);

        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Target: %s\n", target);
        fprintf(stdout, "Children: %ld\n", (long) kw.children);

        if ((void*) ntreeFound != NULL) {
            USERDEFINEDTYPE* udt = ((NTREE*) ntreeFound)->userdefinedtype;
            fprintf(stdout, "Found: %p\n", (void*) ntreeFound);
            fprintf(stdout, "Node Name: %s\n", ((NTREE*) ntreeFound)->name);
            fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        } else {
            fprintf(stdout, "Found: NOT FOUND!\n");
        }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result as a 32/64 bit integer address: IDL does not need to reveal contents of NTree structures

    return (IDL_GettmpMEMINT(ntreeFound));
}


IDL_VPTR IDL_CDECL findidamtreestructuredefinition(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args: IDAM handle (long32 int), Node address or null (0) (long64 int) and the
    // target structure component definition name (string)

    // calls: NTREE *findNTreeStructureDefinition(NTREE *ntree, char *target){

    int handle;
    NTREE* ntree;
    char* target;

    IDL_MEMINT ntreeFound;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nfindIdamTreeStructureDefinition Help\n\n"
                "Returns the tree node address containing data with a specific data structure type.\n"
                "If a null address is returned, no component was located.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n"
                "\t(string) target - the name of the Structure Definition required.\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   String

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_STRING(argv[2]);
    IDL_ENSURE_SCALAR(argv[2]);
    target = IDL_STRING_STR(&(argv[2]->value.str));

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    ntreeFound = (IDL_MEMINT) findNTreeStructureDefinition(ntree, target);

    if (kw.debug) {
        fprintf(stdout, "+++ findIdamTreeStructureDefinition +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Target: %s\n", target);

        if ((void*) ntreeFound != NULL) {
            USERDEFINEDTYPE* udt = ((NTREE*) ntreeFound)->userdefinedtype;
            fprintf(stdout, "Found: %x\n", (unsigned int) ntreeFound);
            fprintf(stdout, "Node Name: %s\n", ((NTREE*) ntreeFound)->name);
            fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        } else {
            fprintf(stdout, "Found: NOT FOUND!\n");
        }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result as a 32/64 bit integer address: IDL does not need to reveal contents of NTree structures

    return (IDL_GettmpMEMINT(ntreeFound));
}


IDL_VPTR IDL_CDECL findidamtreestructure(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args: IDAM handle (long32 int), Node address or null (0) (long64 int) and the
    // target structure component name (string)

    // calls: NTREE *findNTreeStructure(NTREE *ntree, char *target)
    // or     NTREE *findNTreeChildStructure(NTREE *ntree, char *target)

    int handle;
    NTREE* ntree;
    char* target;

    IDL_MEMINT ntreeFound;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"CHILDREN", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(children)},
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.children = 0;
    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nfindIdamTreeStructure Help\n\n"
                "Returns the tree node address containing data with a specific data structure name.\n"
                "If a null address is returned, no node was located.\n\n"
                "Arguments:\n"
                "\t(long)    handle - the IDAM data handle.\n"
                "\t(long64)  tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n"
                "\t(string)  target - the name of the Structure required.\n\n"
                "\t(keyword) children - constrain the search to child nodes.\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   String

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_STRING(argv[2]);
    IDL_ENSURE_SCALAR(argv[2]);
    target = IDL_STRING_STR(&(argv[2]->value.str));

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    if (!kw.children) {
        ntreeFound = (IDL_MEMINT) findNTreeStructure(logmalloclist, ntree, target);
    } else {
        ntreeFound = (IDL_MEMINT) findNTreeChildStructure(logmalloclist, ntree, target);
    }

    if (kw.debug) {
        fprintf(stdout, "+++ findIdamTreeStructure +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Target: %s\n", target);
        fprintf(stdout, "Children: %ld\n", (long) kw.children);

        if ((void*) ntreeFound != NULL) {
            USERDEFINEDTYPE* udt = ((NTREE*) ntreeFound)->userdefinedtype;
            fprintf(stdout, "Found: %x\n", (unsigned int) ntreeFound);
            fprintf(stdout, "Node Name: %s\n", ((NTREE*) ntreeFound)->name);
            fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        } else {
            fprintf(stdout, "Found: NOT FOUND!\n");
        }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result as a 32/64 bit integer address: IDL does not need to reveal contents of NTree structures

    return (IDL_GettmpMEMINT(ntreeFound));
}


//===============================================================================================
// Accessors with a single tree node in scope

IDL_VPTR IDL_CDECL getidamnodestructurecount(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int getNodeStructureCount(NTREE *ntree)

    int handle, count;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureCount Help\n\n"
                "Returns the number of structured components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    if (kw.debug) {
        fprintf(stdout, "Handle: %d\tTree: %p\n", handle, ntree);
    }

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    count = getNodeStructureCount(ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureCount +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(count));
}


IDL_VPTR IDL_CDECL getidamnodestructurenames(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeStructureCount(NTREE *ntree)
    //      char **getNodeStructureNames(NTREE *ntree)

    int handle, count, length;
    NTREE* ntree;
    char** names;

    IDL_STRING* idl_names;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];

    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureNames Help\n\n"
                "Returns the Names of structured components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureCount(ntree);
    names = getNodeStructureNames(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureNames +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %s\n", i, names[i]);
        }

        printNode(ntree);

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the string array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_names = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, 1, idl_shape, IDL_ARR_INI_NOP, &vReturn);

    // Copy strings into IDL array

    for (int i = 0; i < count; i++) {
        length = strlen(names[i]);
        IDL_StrEnsureLength(&idl_names[i], length);   // Ensure length is sufficient
        IDL_StrStore(&idl_names[i], names[i]);        // Copy the string
        idl_names[i].slen = length;           // Set the string length
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}

IDL_VPTR IDL_CDECL getidamnodestructuretypes(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeStructureCount(NTREE *ntree)
    //      char **getNodeStructureTypes(NTREE *ntree)

    int handle, count, length;
    NTREE* ntree;
    char** names;

    IDL_STRING* idl_names;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureTypes Help\n\n"
                "Returns the Types of structure components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureCount(ntree);
    names = getNodeStructureTypes(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureTypes +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %s\n", i, names[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the string array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_names = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, 1, idl_shape, IDL_ARR_INI_NOP, &vReturn);

    // Copy strings into IDL array

    for (int i = 0; i < count; i++) {
        length = strlen(names[i]);
        IDL_StrEnsureLength(&idl_names[i], length);   // Ensure length is sufficient
        IDL_StrStore(&idl_names[i], names[i]);        // Copy the string
        idl_names[i].slen = length;           // Set the string length
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodestructurepointers(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeStructureCount(NTREE *ntree)
    //      int *getNodeStructurePointers(NTREE *ntree)

    int handle, count;
    NTREE* ntree;
    int* pointers;

    IDL_INT* idl_pointers;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructurePointers Help\n\n"
                "Returns the Pointer Property of structure components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureCount(ntree);
    pointers = getNodeStructurePointers(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructurePointers +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %d\n", i, pointers[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_pointers = (short*) IDL_MakeTempArray((int) IDL_TYP_INT, 1, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    for (int i = 0; i < count; i++) {
        idl_pointers[i] = (short) pointers[i];
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}

IDL_VPTR IDL_CDECL getidamnodestructurerank(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeStructureCount(NTREE *ntree)
    //      int *getNodeStructureRank(NTREE *ntree)

    int handle, count;
    NTREE* ntree;
    int* ranks;

    IDL_LONG* idl_data;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureRank Help\n\n"
                "Returns the Rank Property of structure components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureCount(ntree);
    ranks = getNodeStructureRank(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureRank +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %d\n", i, ranks[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_data = (IDL_LONG*) IDL_MakeTempArray((int) IDL_TYP_LONG, 1, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    for (int i = 0; i < count; i++) {
        idl_data[i] = (IDL_LONG) ranks[i];
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}

IDL_VPTR IDL_CDECL getidamnodestructureshape(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeStructureCount(NTREE *ntree)
    //      int *getNodeStructureRank(NTREE *ntree)
    //      int **getNodeStructureShape(NTREE *ntree)

    int handle, count, maxrank = 1;
    NTREE* ntree;
    int* ranks;
    int** shapes;

    IDL_LONG* idl_data;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureShape Help\n\n"
                "Returns the Shape Property of structure components recorded within this tree node.\n\n"
                "This may not be available for rank 1 or less arrays - use the Data Count property.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureCount(ntree);
    shapes = getNodeStructureShape(logmalloclist, ntree);
    ranks = getNodeStructureRank(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureShape +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fprintf(stdout, "Shape: %p\n", shapes);
        fprintf(stdout, "Rank : %p\n", ranks);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: [", i);

            if (shapes != NULL && ranks != NULL) {
                for (int j = 0; j < ranks[i]; j++) {
                    if (shapes[i] != NULL) {
                        fprintf(stdout, "%d ", shapes[i][j]);
                    } else {
                        fprintf(stdout, "0 ");
                    }
                }
            }

            fprintf(stdout, "]\n");
        }

        fflush(NULL);

    }

    if (count == 0 || ranks == NULL) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Maximum Rank

    for (int i = 0; i < count; i++)
        if (ranks[i] > maxrank) {
            maxrank = ranks[i];    // Shapes are returned as a regular array
        }

    // If maxrank = 1 then idl_data will not have the correct shape! A bug in IDL?

    if (maxrank == 1) {
        maxrank = 2;
    }

    // Shape of the returned array

    idl_shape[0] = (IDL_MEMINT) count;
    idl_shape[1] = (IDL_MEMINT) maxrank;

    // Return rank 2 array

    idl_data = (IDL_LONG*) IDL_MakeTempArray((int) IDL_TYP_LONG, 2, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    int k = 0;

    for (int i = 0; i < count; i++)
        for (int j = 0; j < maxrank; j++) {
            idl_data[k++] = 0;    // Assume No information on shape
        }

    // IDL arrays have fortran memory arrangment [first, second] not c arrangement [second, first]

    if (shapes != NULL) {
        k = 0;

        for (int j = 0; j < maxrank; j++) {
            for (int i = 0; i < count; i++) {
                if (j < ranks[i] && shapes[i] != NULL) {
                    idl_data[k++] = (IDL_LONG) (IDL_LONG) shapes[i][j];
                } else {
                    k++;
                }
            }
        }
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomiccount(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int getNodeAtomicCount(NTREE *ntree)

    int handle, count;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicCount Help\n\n"
                "Returns the number of atomic components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    count = getNodeAtomicCount(ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicCount +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(count));
}


IDL_VPTR IDL_CDECL getidamnodeatomicnames(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeAtomicCount(NTREE *ntree)
    //      char **getNodeAtomicNames(NTREE *ntree)

    int handle, count, length;
    NTREE* ntree;
    char** names;

    IDL_STRING* idl_names;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicNames Help\n\n"
                "Returns the Names of atomic components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeAtomicCount(ntree);
    names = getNodeAtomicNames(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicNames +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %s\n", i, names[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the string array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_names = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, 1, idl_shape, IDL_ARR_INI_NOP, &vReturn);

    // Copy strings into IDL array

    for (int i = 0; i < count; i++) {
        length = strlen(names[i]);
        IDL_StrEnsureLength(&idl_names[i], length);   // Ensure length is sufficient
        IDL_StrStore(&idl_names[i], names[i]);        // Copy the string
        idl_names[i].slen = length;           // Set the string length
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomictypes(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeAtomicCount(NTREE *ntree)
    //      char **getNodeAtomicTypes(NTREE *ntree)

    int handle, count, length;
    NTREE* ntree;
    char** names;

    IDL_STRING* idl_names;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicTypes Help\n\n"
                "Returns the Types of atomic components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeAtomicCount(ntree);
    names = getNodeAtomicTypes(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicTypes +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %s\n", i, names[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the string array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_names = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, 1, idl_shape, IDL_ARR_INI_NOP, &vReturn);

    // Copy strings into IDL array

    for (int i = 0; i < count; i++) {
        length = strlen(names[i]);
        IDL_StrEnsureLength(&idl_names[i], length);   // Ensure length is sufficient
        IDL_StrStore(&idl_names[i], names[i]);        // Copy the string
        idl_names[i].slen = length;           // Set the string length
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomicpointers(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeAtomicCount(NTREE *ntree)
    //      int *getNodeAtomicPointers(NTREE *ntree)

    int handle, count;
    NTREE* ntree;
    int* pointers;

    IDL_INT* idl_pointers;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicPointers Help\n\n"
                "Returns the Pointer Property of atomic components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeAtomicCount(ntree);
    pointers = getNodeAtomicPointers(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicPointers +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %d\n", i, pointers[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_pointers = (short*) IDL_MakeTempArray((int) IDL_TYP_INT, 1, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    for (int i = 0; i < count; i++) {
        idl_pointers[i] = (short) pointers[i];
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomicrank(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeAtomicCount(NTREE *ntree)
    //      int *getNodeAtomicRank(NTREE *ntree)

    int handle, count;
    NTREE* ntree;
    int* ranks;

    IDL_LONG* idl_data;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicRank Help\n\n"
                "Returns the Rank Property of atomic components recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeAtomicCount(ntree);
    ranks = getNodeAtomicRank(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicRank +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: %d\n", i, ranks[i]);
        }

        fflush(NULL);
    }

    if (count == 0) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Shape of the array

    idl_shape[0] = (IDL_MEMINT) count;

    // Return array

    idl_data = (IDL_LONG*) IDL_MakeTempArray((int) IDL_TYP_LONG, 1, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    for (int i = 0; i < count; i++) {
        idl_data[i] = (IDL_LONG) ranks[i];
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomicshape(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:   int getNodeAtomicCount(NTREE *ntree)
    //      int *getNodeAtomicRank(NTREE *ntree)
    //      int **getNodeAtomicShape(NTREE *ntree)

    int handle, count, maxrank = 1;
    NTREE* ntree;
    int* ranks;
    int** shapes;

    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];
    IDL_VPTR vReturn;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicShape Help\n\n"
                "Returns the Shape Property of atomic components recorded within this tree node.\n\n"
                "This may not be available for rank 1 or less arrays - use the Data Count property.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeAtomicCount(ntree);
    shapes = getNodeAtomicShape(logmalloclist, ntree);
    ranks = getNodeAtomicRank(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicShape +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fprintf(stdout, "Shape: %p\n", shapes);
        fprintf(stdout, "Rank : %p\n", ranks);

        for (int i = 0; i < count; i++) {
            fprintf(stdout, "[%2d]: [", i);

            if (shapes != NULL && ranks != NULL) {
                for (int j = 0; j < ranks[i]; j++) {
                    if (shapes[i] != NULL) {
                        fprintf(stdout, "%d ", shapes[i][j]);
                    } else {
                        fprintf(stdout, "0 ");
                    }
                }
            }

            fprintf(stdout, "]\n");
        }

        fflush(NULL);

    }

    if (count == 0 || ranks == NULL) {
        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Structure

    // Maximum Rank

    for (int i = 0; i < count; i++)
        if (ranks[i] > maxrank) {
            maxrank = ranks[i];    // Shapes are returned as a regular array
        }

    // If maxrank = 1 then idl_data will not have the correct shape! A bug in IDL?

    if (maxrank == 1) {
        maxrank = 2;
    }

    // Shape of the returned array

    idl_shape[0] = (IDL_MEMINT) count;
    idl_shape[1] = (IDL_MEMINT) maxrank;

    // Return rank 2 array
    IDL_LONG* idl_data;
    idl_data = (IDL_LONG*) IDL_MakeTempArray((int) IDL_TYP_LONG, 2, idl_shape, IDL_ARR_INI_ZERO, &vReturn);

    int k = 0;

    for (int i = 0; i < count; i++)
        for (int j = 0; j < maxrank; j++) {
            idl_data[k++] = 0;    // Assume No information on shape
        }

    // IDL arrays have fortran memory arrangment [first, second] not c arrangement [second, first]

    if (shapes != NULL) {
        k = 0;

        for (int j = 0; j < maxrank; j++) {
            for (int i = 0; i < count; i++) {
                if (j < ranks[i] && shapes[i] != NULL) {
                    idl_data[k++] = (IDL_LONG) (IDL_LONG) shapes[i][j];
                } else {
                    k++;
                }
            }
        }
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (vReturn);
}


IDL_VPTR IDL_CDECL getidamnodeatomicdatacount(int argc, IDL_VPTR argv[], char* argk) {
    //
    // Returns the number of array elements
    //
    // 3 Args:  IDAM handle (long32 int), Node address or null (0) (long64 int),
    //      Component Name (IDL String)

    // calls:   getNodeStructureComponentDataCount(ntree, name)

    int handle, count;
    NTREE* ntree;
    char* name;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicDataCount Help\n\n"
                "Returns atomic component Data Array count (size) recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   Component Name

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_STRING(argv[2]);
    name = IDL_STRING_STR(&(argv[2]->value.str));

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Count

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureComponentDataCount(logmalloclist, ntree, name);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicDataCount +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fflush(NULL);
    }

    return (IDL_GettmpLong(count));
}


IDL_VPTR IDL_CDECL getidamnodeatomicdata(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args:  IDAM handle (long32 int), Node address or null (0) (long64 int),
    //      Component Name (IDL String)

    // calls:   getNodeStructureComponentDataRank(ntree, name)
    //          getNodeStructureComponentDataCount(ntree, name)
    //      getNodeStructureComponentDataShape(ntree, name)
    //      getNodeStructureComponentDataDataType(ntree, name)
    //      getNodeStructureComponentDataIsPointer(ntree, name)
    //      getNodeStructureComponentData(ntree, name)

    int handle, count, rank, pointer, test = 1;
    int* shape;
    const char* name;
    const char* type;
    void* data;
    NTREE* ntree;

    IDL_VPTR vData = NULL;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeAtomicData Help\n\n"
                "Returns atomic component Data recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   Component Name

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_STRING(argv[2]);
    name = IDL_STRING_STR(&(argv[2]->value.str));

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Type, Pointer class, Count, Rank, Shape and Data

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    rank = getNodeStructureComponentDataRank(logmalloclist, ntree, name);
    count = getNodeStructureComponentDataCount(logmalloclist, ntree, name);
    shape = getNodeStructureComponentDataShape(logmalloclist, ntree, name);
    type = getNodeStructureComponentDataDataType(logmalloclist, ntree, name);
    pointer = getNodeStructureComponentDataIsPointer(logmalloclist, ntree, name);
    data = getNodeStructureComponentData(logmalloclist, ntree, name);

    if (rank <= 1) {
        idl_shape[0] = (IDL_MEMINT) count;
    } else {
        if (shape != NULL) {

            // IDL arrays have fortran memory arrangment [first, second] not c arrangement [second, first]

            //for(i=0;i<rank;i++) idl_shape[i] = (IDL_LONG)shape[i];
            for (int i = 0; i < rank; i++) {
                idl_shape[rank - i - 1] = (IDL_MEMINT) shape[i];
            }

        } else {
            fprintf(stdout,
                    "WARNING: Atomic Structure Component %s has Rank %d but no Shape Information: Reducing Rank\n",
                    name, rank);
            rank = 1;
            idl_shape[0] = (IDL_MEMINT) count;
        }
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeAtomicData +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Data: %p\n", data);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "[%s]   Rank: %d\tCount: %d\tType: %s\tPointer: %d\n", name, rank, count, type, pointer);
        fprintf(stdout, "Shape: %p\n", shape);

        if (shape != NULL) {
            fprintf(stdout, "Shape[0]: %d\n", (int) shape[0]);

            if (rank > 1) {
                fprintf(stdout, "Shape[1]: %d\n", (int) shape[1]);
            }

            if (rank > 2) {
                fprintf(stdout, "Shape[2]: %d\n", (int) shape[2]);
            }
        }

        fflush(NULL);
    }

    if (rank > 1) {
        for (int i = 0; i < rank; i++) {
            test = test * (int) idl_shape[i];    // Check shape values
        }

        if (test != count) {
            fprintf(stdout, "ERROR: Atomic Structure Component %s has an Inconsistent Shape\n", name);
            return (IDL_GettmpLong(0));
        }
    }

    if (data == NULL && count > 0) {
        fprintf(stdout, "ERROR: Atomic Structure Component %s is NULL\n", name);
        return (IDL_GettmpLong(0));
    }

    if (data != NULL && type == NULL) {
        fprintf(stdout, "ERROR: Atomic Structure Component %s has No Type!\n", name);
        return (IDL_GettmpLong(0));
    }

    if (data != NULL && count == 0) {
        fprintf(stdout, "ERROR: Atomic Structure Component %s has Zero Count value!\n", name);
        return (IDL_GettmpLong(0));
    }

    // dgm 13May2011: added to trap zero length array data, e.g. UNLIMITED coordinates with 0 length set!
    // Just return a Zero scalar!

    if (count == 0) {

        if (kw.verbose) {
            fprintf(stdout, "Returning a NULL address!\n");
        }

        return (IDL_GettmpMEMINT(0));     // Return a Null address

        vData = IDL_Gettmp();
        vData->type = IDL_TYP_DOUBLE;
        vData->value.f = 0.0;
        return (vData);
    }

    if (data == NULL) {
        return (IDL_GettmpLong(0));    // No Data to return (need a NaN value to return depending on type)
    }

    //---------------------------------------------------------------------------------------------
    // Create Return Data Array

    // FIX ******************************* Need to pass shape via mallocLog

    //if(rank == 1 && pointer)idl_shape[0] = (IDL_LONG)count;

    // FIX *******************************

    switch (getIdamDataTypeId(type)) {

        case UDA_TYPE_STRING: {

            //fprintf(stdout,"[%s]   Rank: %d\tCount: %d\tType: %s\tPointer: %d\n",name, rank, count, type, pointer);
            //if(shape != NULL){
            //fprintf(stdout,"Shape[0]: %d\n",(int) shape[0]);
            //if(rank > 1)fprintf(stdout,"Shape[1]: %d\n",(int) shape[1]);
            //if(rank > 2)fprintf(stdout,"Shape[2]: %d\n",(int) shape[2]);
            //}


            int length, nstr;
            char** Data = (char**) data;
            IDL_STRING* idlData;

            if (rank >= 1 && pointer) {
                if (kw.debug) {
                    fprintf(stdout, "String #1a\n");
                    fflush(stdout);
                }                       // Array of Strings of arbitrary length

                idlData = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, rank, idl_shape, IDL_ARR_INI_NOP,
                                                          &vData);

                for (int i = 0; i < count; i++) {
                    length = strlen(Data[i]);
                    IDL_StrEnsureLength(&idlData[i], length + 1);      // Ensure length is sufficient
                    IDL_StrStore(&idlData[i], Data[i]);          // Copy the string
                    idlData[i].slen = length;                // Set the string length
                }

                if (kw.debug) {
                    fprintf(stdout, "String #1b\n");
                    fflush(stdout);
                }
            } else {
                if (STR_EQUALS(type, "STRING *")) {            // Array of Strings from Pointer
                    if (kw.debug) {
                        fprintf(stdout, "String #2a\n");
                        fprintf(stdout, "[%s]   Rank: %d\tCount: %d\tType: %s\tPointer: %d\n", name, rank, count, type,
                                pointer);
                        fprintf(stdout, "shape: %d\n", (int) idl_shape[0]);
                        //fprintf(stdout, "&Data: %p\n", Data[i]);
                        fflush(stdout);
                    }

                    rank = 1;
                    idlData = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, rank, idl_shape, IDL_ARR_INI_NOP,
                                                              &vData);

                    for (int i = 0; i < count; i++) {

                        if (Data[i] == NULL) {
                            IDL_StrEnsureLength(&idlData[i], 1);       // Ensure length is sufficient
                            IDL_StrStore(&idlData[i], "");         // Copy the string
                            idlData[i].slen = 0;               // Set the string length
                            continue;
                        }

                        length = strlen(Data[i]);

                        if (kw.debug) {
                            fprintf(stdout, "length[%d] %d\n", i, length);
                            fflush(stdout);
                        }

                        IDL_StrEnsureLength(&idlData[i], length + 1);       // Ensure length is sufficient
                        IDL_StrStore(&idlData[i], Data[i]);           // Copy the string
                        idlData[i].slen = length;             // Set the string length
                    }

                    //vData = IDL_StrToSTRING((char *)Data[0]);      // Single String from Pointer
                    if (kw.debug) {
                        fprintf(stdout, "String #2b\n");
                        fflush(stdout);
                    }
                } else {
                    if (rank < 2) {                      // Fixed Length Single String
                        if (kw.debug) {
                            fprintf(stdout, "String #3a\n");
                            fprintf(stdout, "[0] %c\n", ((char*) data)[0]);
                            fprintf(stdout, "[1] %c\n", ((char*) data)[1]);
                            fprintf(stdout, "%d [%s]\n", (int) strlen((char*) data), (char*) data);
                        }

                        vData = IDL_StrToSTRING((char*) data);

                        if (kw.debug) {
                            fprintf(stdout, "String #3b\n");
                            fflush(stdout);
                        }
                    } else {                             // Array of Fixed Length Single String: Reduce Definition Rank
                        if (rank > 2) {   // Not implmented
                            fprintf(stdout, "ERROR: String Array with Rank > 2 - not implemented!\n");
                            return (IDL_GettmpLong(0));
                        }

                        if (kw.debug) {
                            fprintf(stdout, "String #4a\n");
                            fflush(stdout);
                        }

                        char* str = (char*) data;
                        idlData = (IDL_STRING*) IDL_MakeTempArray((int) IDL_TYP_STRING, rank - 1, &idl_shape[0],
                                                                  IDL_ARR_INI_NOP, &vData);
                        nstr = (int) idl_shape[0];           // Number of Strings
                        length = (int) idl_shape[1];           // Maximum Length
                        //fprintf(stdout,"Rank    %d\n",rank);
                        //fprintf(stdout,"Strings %d\n",nstr);
                        //fprintf(stdout,"Length  %d\n",length);
                        //for(i=0;i<nstr*length;i++)fprintf(stdout,"[%d] %d   %c\n", i, (int)str[i], str[i]);
                        //fflush(stdout);

                        char* s = NULL;
                        int lstr, offset = 0;

                        for (int i = 0; i < nstr; i++) {
                            lstr = strlen(&str[offset]);

                            if (lstr > length) {
                                lstr = length;
                            }

                            s = (char*) malloc((lstr + 1) * sizeof(char));
                            strncpy(s, &str[offset], lstr);
                            s[lstr] = '\0';
                            //fprintf(stdout,"s[%d]    %s\n",i, s);
                            //fprintf(stdout,"l[%d]    %d\n",i, lstr);
                            //fprintf(stdout,"offset   %d\n",offset);

                            IDL_StrEnsureLength(&idlData[i], lstr);            // Ensure fixed length is sufficient
                            IDL_StrStore(&idlData[i], &str[offset]);           // Copy each string
                            idlData[i].slen = lstr;                    // Set the fixed string length
                            offset = (i + 1) * length;
                            free((void*) s);
                        }

                        if (kw.debug) {
                            fprintf(stdout, "String #4b\n");
                            fflush(stdout);
                        }
                    }
                }
            }

            return (vData);
        }

        case UDA_TYPE_FLOAT: {
            float* Data = (float*) data;
            float* idlData;

            if (rank > 0) {
                idlData = (float*) IDL_MakeTempArray((int) IDL_TYP_FLOAT, rank, idl_shape, IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (float*) IDL_MakeTempVector((int) IDL_TYP_FLOAT, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                          &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_FLOAT;
                    vData->value.f = Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_DOUBLE: {
            double* Data = (double*) data;
            double* idlData;

            if (rank > 0) {
                if (0 && kw.debug) {
                    fprintf(stdout, "[%s] UDA_TYPE_DOUBLE!\n", name);
                    fprintf(stdout, "rank %d\n", rank);
                    fprintf(stdout, "count %d\n", count);
                    fprintf(stdout, "shape[0] %d\n", (int) idl_shape[0]);

                    if (rank > 1) {
                        fprintf(stdout, "shape[1] %d\n", (int) idl_shape[1]);
                    }

                    if (rank > 2) {
                        fprintf(stdout, "shape[2] %d\n", (int) idl_shape[2]);
                    }
                }

                idlData = (double*) IDL_MakeTempArray((int) IDL_TYP_DOUBLE, rank, idl_shape, IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }

            } else {
                if (count > 1) {
                    idlData = (double*) IDL_MakeTempVector((int) IDL_TYP_DOUBLE, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                           &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_DOUBLE;
                    vData->value.d = Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_CHAR: {
            char* Data = (char*) data;
            char* idlData;

            if (rank > 0) {
                idlData = (char*) IDL_MakeTempArray((int) IDL_TYP_BYTE, rank, idl_shape, IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (char*) IDL_MakeTempVector((int) IDL_TYP_BYTE, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                         &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_BYTE;
                    vData->value.c = Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_SHORT: {
            short* Data = (short*) data;
            short* idlData;

            if (rank > 0) {
                idlData = (short*) IDL_MakeTempArray((int) IDL_TYP_INT, rank, idl_shape, IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (short*) IDL_MakeTempVector((int) IDL_TYP_INT, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                          &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_GettmpInt(Data[0]);
                }
            }

            return (vData);
        }

        case UDA_TYPE_INT: {
            int* Data = (int*) data;
            int* idlData;

            if (rank > 0) {
                idlData = (int*) IDL_MakeTempArray((int) IDL_TYP_LONG, rank, idl_shape, IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (int*) IDL_MakeTempVector((int) IDL_TYP_LONG, (IDL_LONG) count, IDL_ARR_INI_ZERO, &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_GettmpLong(Data[0]);
                }
            }

            return (vData);
        }

        case UDA_TYPE_LONG64: {
            long long* Data = (long long*) data;
            long long* idlData;

            if (rank > 0) {
                idlData = (long long*) IDL_MakeTempArray((int) IDL_TYP_LONG64, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                         &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (long long*) IDL_MakeTempVector((int) IDL_TYP_LONG64, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                              &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_LONG64;
                    vData->value.l64 = Data[0];      // Use appropriate structure union
                }
            }

            return (vData);
        }

        case UDA_TYPE_UNSIGNED_CHAR: {
            unsigned char* Data = (unsigned char*) data;
            unsigned char* idlData;

            if (rank > 0) {
                idlData = (unsigned char*) IDL_MakeTempArray((int) IDL_TYP_BYTE, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                             &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (unsigned char*) IDL_MakeTempVector((int) IDL_TYP_BYTE, (IDL_LONG) count,
                                                                  IDL_ARR_INI_ZERO, &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_BYTE;
                    vData->value.c = (char) Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_UNSIGNED_SHORT: {
            unsigned short* Data = (unsigned short*) data;
            unsigned short* idlData;

            if (rank > 0) {
                idlData = (unsigned short*) IDL_MakeTempArray((int) IDL_TYP_UINT, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                              &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (unsigned short*) IDL_MakeTempVector((int) IDL_TYP_UINT, (IDL_LONG) count,
                                                                   IDL_ARR_INI_ZERO, &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_GettmpUInt(Data[0]);
                }
            }

            return (vData);
        }

        case UDA_TYPE_UNSIGNED_INT: {
            unsigned int* Data = (unsigned int*) data;
            unsigned int* idlData;

            if (rank > 0) {
                idlData = (unsigned int*) IDL_MakeTempArray((int) IDL_TYP_ULONG, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                            &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (unsigned int*) IDL_MakeTempVector((int) IDL_TYP_ULONG, (IDL_LONG) count,
                                                                 IDL_ARR_INI_ZERO, &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_GettmpULong(Data[0]);
                }
            }

            return (vData);
        }

        case UDA_TYPE_UNSIGNED_LONG64: {
            unsigned long long* Data = (unsigned long long*) data;
            unsigned long long* idlData;

            if (rank > 0) {
                idlData = (unsigned long long*) IDL_MakeTempArray((int) IDL_TYP_ULONG64, rank, idl_shape,
                                                                  IDL_ARR_INI_ZERO, &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 1) {
                    idlData = (unsigned long long*) IDL_MakeTempVector((int) IDL_TYP_ULONG64, (IDL_LONG) count,
                                                                       IDL_ARR_INI_ZERO, &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    vData = IDL_Gettmp();
                    vData->type = IDL_TYP_ULONG64;
                    vData->value.ul64 = Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_COMPLEX: {
            COMPLEX* Data = (COMPLEX*) data;
            COMPLEX* idlData;

            if (rank > 0) {
                idlData = (COMPLEX*) IDL_MakeTempArray((int) IDL_TYP_COMPLEX, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                       &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 0) {
                    idlData = (COMPLEX*) IDL_MakeTempVector((int) IDL_TYP_COMPLEX, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                            &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    //vData = IDL_Gettmp();
                    //IDL_StoreScalarZero(vData, IDL_TYP_COMPLEX);
                    //vData->value.cmp = (IDL_COMPLEX) Data[0];
                }
            }

            return (vData);
        }

        case UDA_TYPE_DCOMPLEX: {
            DCOMPLEX* Data = (DCOMPLEX*) data;
            DCOMPLEX* idlData;

            if (rank > 0) {
                idlData = (DCOMPLEX*) IDL_MakeTempArray((int) IDL_TYP_DCOMPLEX, rank, idl_shape, IDL_ARR_INI_ZERO,
                                                        &vData);

                for (int i = 0; i < count; i++) {
                    idlData[i] = Data[i];
                }
            } else {
                if (count > 0) {
                    idlData = (DCOMPLEX*) IDL_MakeTempVector((int) IDL_TYP_DCOMPLEX, (IDL_LONG) count, IDL_ARR_INI_ZERO,
                                                             &vData);

                    for (int i = 0; i < count; i++) {
                        idlData[i] = Data[i];
                    }
                } else {
                    //vData             = IDL_Gettmp();
                    //vData->type       = IDL_TYP_DCOMPLEX;
                    //vData->value.dcmp = (IDL_DCOMPLEX) Data[0];
                }
            }

            return (vData);
        }

    }


    //---------------------------------------------------------------------------------------------
    // Return result

    if (kw.verbose) {
        fprintf(stdout, "ERROR: Atomic Structure Component Type [%s] not known\n", type);
    }

    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamnodestructuredatacount(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int getNodeStructureDataCount(NTREE *ntree)

    int handle, count;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureDataCount Help\n\n"
                "Returns the number of structure array elements recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureDataCount(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureDataCount +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(count));
}

IDL_VPTR IDL_CDECL getidamnodestructuredatarank(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int getNodeStructureDataRank(NTREE *ntree)

    int handle, rank;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureDataRank Help\n\n"
                "Returns the rank of the structure array recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    rank = getNodeStructureDataRank(logmalloclist, ntree);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureDataRank +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Rank: %d\n", rank);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(rank));
}

IDL_VPTR IDL_CDECL getidamnodestructuredatashape(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int *getNodeStructureDataShape(NTREE *ntree)
    //    int  getNodeStructureDataRank(NTREE *ntree)
    //    int  getNodeStructureDataCount(NTREE *ntree)

    int handle, count, rank, test;
    NTREE* ntree;

    IDL_VPTR vData;
    IDL_MEMINT idl_shape[IDL_MAX_ARRAY_DIM];

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeStructureDataShape Help\n\n"
                "Returns the shape of the structure array recorded within this tree node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    count = getNodeStructureDataCount(logmalloclist, ntree);
    rank = getNodeStructureDataRank(logmalloclist, ntree);
    int* shape;

    if (rank <= 1) {
        idl_shape[0] = 1;
    } else {
        if ((shape = getNodeStructureDataShape(logmalloclist, ntree)) == NULL) {
            fprintf(stdout, "ERROR: Structure Component has no Shape data when expected!\n");
            return (IDL_GettmpLong(0));
        }

        idl_shape[0] = rank;
        test = 1;

        for (int i = 0; i < rank; i++) {
            test = test * shape[i];    // Check shape values
        }

        if (test != count) {
            fprintf(stdout, "ERROR: Structure Component has an Inconsistent Shape\n");
            return (IDL_GettmpLong(0));
        }
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeStructureDataShape +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fprintf(stdout, "Count: %d\n", count);
        fprintf(stdout, "Rank : %d\n", rank);

        if (rank > 1)
            for (int i = 0; i < rank; i++) {
                fprintf(stdout, "Shape[%d]: %d\n", i, shape[i]);
            }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    int* idlData = (int*) IDL_MakeTempArray((int) IDL_TYP_LONG, 1, idl_shape, IDL_ARR_INI_ZERO, &vData);

    if (rank <= 1) {
        idlData[0] = count;
    } else
        for (int i = 0; i < rank; i++) {
            idlData[i] = shape[i];
        }

    return (vData);
}


IDL_VPTR IDL_CDECL getidamnodeparent(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: NTREE *getNodeParent(NTREE *ntree)

    int handle;
    NTREE* ntree;

    IDL_MEMINT ntreeFound;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeParent Help\n\n"
                "Returns the Parent tree node address.\n"
                "If a null address is returned there is no Parent node. The child node given is the Root node.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the child tree node. If null (0) the root node is used.\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    ntreeFound = (IDL_MEMINT) getNodeParent(ntree);

    if (kw.debug) {
        fprintf(stdout, "+++ getIdamNodeParent +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);

        if ((void*) ntreeFound != NULL) {
            USERDEFINEDTYPE* udt = ((NTREE*) ntreeFound)->userdefinedtype;
            fprintf(stdout, "Found: %x\n", (unsigned int) ntreeFound);
            fprintf(stdout, "Node Name: %s\n", ((NTREE*) ntreeFound)->name);
            fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        } else {
            fprintf(stdout, "Found: NOT FOUND!\n");
        }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result as a 32/64 bit integer address: IDL does not need to reveal contents of NTree structures

    return (IDL_GettmpMEMINT(ntreeFound));
}


IDL_VPTR IDL_CDECL getidamnodechild(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args: IDAM handle (long32 int), Node address or null (0) (long64 int) and the
    // child branch number (long32 int)

    // calls: NTREE *getNodeChild(NTREE *ntree, int child){

    int handle, child;
    NTREE* ntree;

    IDL_MEMINT ntreeFound;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeChild Help\n\n"
                "Returns the tree node address of a child node identified by the branch number.\n"
                "If a null address is returned, no child node was located.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n"
                "\t(long)   child - the tree branch number.\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)
    // Arg#3:   node child branch

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_SCALAR(argv[2]);
    child = (int) IDL_LongScalar(argv[2]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    ntreeFound = (IDL_MEMINT) getNodeChild(ntree, child);

    if (kw.debug) {
        fprintf(stdout, "+++ getIdamNodeChild +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Child: %d\n", child);

        if ((void*) ntreeFound != NULL) {
            USERDEFINEDTYPE* udt = ((NTREE*) ntreeFound)->userdefinedtype;
            fprintf(stdout, "Found: %x\n", (unsigned int) ntreeFound);
            fprintf(stdout, "Node Name: %s\n", ((NTREE*) ntreeFound)->name);
            fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        } else {
            fprintf(stdout, "Found: NOT FOUND!\n");
        }

        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result as a 32/64 bit integer address: IDL does not need to reveal contents of NTree structures

    return (IDL_GettmpMEMINT(ntreeFound));
}

IDL_VPTR IDL_CDECL getidamnodechildrencount(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: int getNodeChildrenCount(NTREE *ntree)

    int handle;
    NTREE* ntree;

    int count;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeChildrenCount Help\n\n"
                "Returns the number of child nodes or branches attached to a tree node.\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the tree node\n\n");

        return (IDL_GettmpMEMINT(0));             // Return a Null address
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpMEMINT(0));      // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    count = (IDL_MEMINT) getNodeChildrenCount(ntree);

    if (kw.debug) {
        fprintf(stdout, "+++ getIdamNodeChildrenCount +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Count of Child Nodes or Branches: %d\n", count);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(count));
}


IDL_VPTR IDL_CDECL getidamnodechildid(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 3 Args: IDAM handle (long32 int), Parent Node address or null (0) (long64 int) and the
    // child Node address (long64 int)

    // calls: NTREE *getNodeChildId(NTREE *ntree, int child){

    int handle, childId;
    NTREE* ntree, * child;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\ngetIdamNodeChildId Help\n\n"
                "Returns the Tree Branch ID of a child node identified by its node address.\n"
                "If a negative integer value is returned, the child node was not located.\n\n"
                "Arguments:\n"
                "\t(long)   handle - the IDAM data handle.\n"
                "\t(long64) tree - the Parent tree node. If null (0) the root node is used.\n"
                "\t(long64) child - the Child tree node.\n\n");

        return (IDL_GettmpLong(-1));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to Parent Node
    // Arg#3:   Pointer to Child Node

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    IDL_ENSURE_SCALAR(argv[2]);
    child = (NTREE*) IDL_MEMINTScalar(argv[2]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(-1));       // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    childId = getNodeChildId(ntree, child);

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ getIdamNodeChildId +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree : %p\n", ntree);
        fprintf(stdout, "Child: %p\n", child);
        fprintf(stdout, "Id: %d\n", childId);
        fprintf(stdout, "Parent Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Return result

    return (IDL_GettmpLong(childId));
}


//===============================================================================================
// Print

IDL_VPTR IDL_CDECL printidamtree(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: void *printNTree(stdout, NTREE *ntreet)
    //        void *printNTreeList(stdout, NTREE *ntreet)

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nprintidamtree Help\n\n"
                "\tPrints the contents of a Data Tree.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {

        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ printidamtree +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    printNTreeList(ntree);
    fflush(stdout);

    //---------------------------------------------------------------------------------------------
    // Return

    return (IDL_GettmpLong(0));
}


IDL_VPTR IDL_CDECL printidamtreestructurenames(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: void printNTreeStructureNames(stdout, NTREE *tree)

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nprintIdamNTreeStructureNames Help\n\n"
                "\tPrints the contents of a Data Tree.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {
        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ printIdamNTreeStructureNames +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    printNTreeStructureNames(logmalloclist, ntree);

    //---------------------------------------------------------------------------------------------
    // Return

    return (IDL_GettmpLong(0));
}


IDL_VPTR IDL_CDECL printidamtreestructurecomponentnames(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: void printNTreeStructureComponentNames(stdout, NTREE *tree)

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nprintIdamNTreeStructureComponentNames Help\n\n"
                "\tPrints the contents of a Data Tree.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {
        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ printIdamNTreeStructureComponentNames +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    printNTreeStructureComponentNames(logmalloclist, ntree);

    //---------------------------------------------------------------------------------------------
    // Return

    return (IDL_GettmpLong(0));
}


IDL_VPTR IDL_CDECL printidamnodestructure(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: void printNodeStructure(stdout, NTREE *ntree)

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nprintIdamNodeStructure Help\n\n"
                "\tPrints the contents of a Data Node.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {
        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ printIdamNodeStructure +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);

    printNodeStructure(logmalloclist, ntree);

    //---------------------------------------------------------------------------------------------
    // Return

    return (IDL_GettmpLong(0));
}


IDL_VPTR IDL_CDECL regulariseidamvlenstructures(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls: void idam_regulariseVlenData(NTREE *ntree)

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nregulariseidamvlenstructures Help\n\n"
                "\tRegularise the Shape of all VLEN structure array data.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {
        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    if (kw.debug) {
        USERDEFINEDTYPE* udt = ntree->userdefinedtype;
        fprintf(stdout, "+++ printIdamNodeStructure +++\n");
        fprintf(stdout, "Handle: %d\n", handle);
        fprintf(stdout, "Tree: %p\n", ntree);
        fprintf(stdout, "Node Name: %s\n", ntree->name);
        fprintf(stdout, "Structure Definition Name: %s\n", udt->name);
        fflush(NULL);
    }

    //---------------------------------------------------------------------------------------------
    // Call accessor

    LOGMALLOCLIST* logmalloclist = getIdamLogMallocList(handle);
    USERDEFINEDTYPELIST* userdefinedtypelist = getIdamUserDefinedTypeList(handle);

    int rc = idam_regulariseVlenData(logmalloclist, ntree, userdefinedtypelist);

    return (IDL_GettmpLong(rc));
}


//=============================================================================================
//=============================================================================================

IDL_VPTR IDL_CDECL makeidamstructure(int argc, IDL_VPTR argv[], char* argk) {
    //
    // 2 Args: IDAM handle (long32 int), Node address or null (0) (long64 int)

    // calls:

    int handle;
    NTREE* ntree;

    // Maintain an Alphabetical Order of Keywords

    static IDL_KW_PAR kw_pars[] = {IDL_KW_FAST_SCAN,
                                   {"DEBUG", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(debug)},
                                   {"HELP", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(help)},
                                   {"VERBOSE", IDL_TYP_LONG, 1, IDL_KW_ZERO, 0, IDL_KW_OFFSETOF(verbose)},
                                   {NULL}
    };

    KW_RESULT kw;

    //---------------------------------------------------------------------------------------------
    // keywords

    kw.debug = 0;
    kw.help = 0;
    kw.verbose = 0;

    // Get Passed Keywords and Parameters

    IDL_KWProcessByOffset(argc, argv, argk, kw_pars, (IDL_VPTR*) 0, 1, &kw);

    if (kw.help) {
        fprintf(stdout, "\n\nmakeStructure Help\n\n"
                "\tCreates a Data Structure from the contents of a specific tree node.\n\n"
                "\tArguments:\n"
                "\t\t(long)   handle - the IDAM data handle.\n"
                "\t\t(long64) tree - the starting tree node within the data tree. If null (0)"
                "the root node is used.\n\n");

        return (IDL_GettmpLong(0));
    }

    //---------------------------------------------------------------------------------------------
    // Arguments
    //
    // Arg#1:   IDAM data handle
    // Arg#2:   Pointer to NTree Node (root of tree)

    IDL_ENSURE_SCALAR(argv[0]);
    handle = (int) IDL_LongScalar(argv[0]);

    IDL_ENSURE_SCALAR(argv[1]);
    ntree = (NTREE*) IDL_MEMINTScalar(argv[1]);

    if (kw.debug) {
        fprintf(stdout, "Handle: %d\tTree: %p\n", handle, ntree);
    }

    //---------------------------------------------------------------------------------------------
    // Set the Tree if NULL

    if (ntree == 0) {
        if (!setIdamDataTree(handle)) {   // Check and register that data is hierarchical
            fprintf(stdout, "Error: The Data specified is NOT Hierarchical - use the regular IDAM accessors\n");
            return (IDL_GettmpLong(0));    // Return a Null address
        }

        ntree = getIdamDataTree(handle);      // the Root Node of the Data tree required
    }

    //---------------------------------------------------------------------------------------------
    // Make the Structure



    //---------------------------------------------------------------------------------------------
    // Return

    return (IDL_GettmpLong(0));
}


IDL_VPTR IDL_CDECL getidamstructuredatasize(int argc, IDL_VPTR argv[], char* argk) {
    //int getNodeStructureDataSize(NTREE *ntree)
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamstructuredatadatatype(int argc, IDL_VPTR argv[], char* argk) {
    //char *getNodeStructureDataDataType(NTREE *ntree)
    return (IDL_GettmpLong(0));
}

IDL_VPTR IDL_CDECL getidamstructuredata(int argc, IDL_VPTR argv[], char* argk) {
    //void *getNodeStructureData(NTREE *ntree)
    return (IDL_GettmpLong(0));
}





