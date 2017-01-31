/*---------------------------------------------------------------
* Test IDAM Plugin: Test regular and sructured data passing middleware
*
* Input Arguments:	IDAM_PLUGIN_INTERFACE *idam_plugin_interface
*
* Returns:		testplugin	0 if read was successful
*					otherwise a Error Code is returned
*			DATA_BLOCK	Structure with Data from the File
*
* Calls		freeDataBlock	to free Heap memory if an Error Occurs
*
* Notes: 	All memory required to hold data is allocated dynamically
*		in heap storage. Pointers to these areas of memory are held
*		by the passed DATA_BLOCK structure. Local memory allocations
*		are freed on exit. However, the blocks reserved for data are
*		not and MUST BE FREED by the calling routine.
*
*
* Change History
*
* 25Oct2011	D.G.Muir	Original Version
* 05Feb2013	D.G.Muir	Added test calls to other server plugins
*---------------------------------------------------------------------------------------------------------------*/
#include "testplugin.h"

#include <stdlib.h>

#include <clientserver/initStructs.h>
#include <structures/struct.h>
#include <server/makeServerRequestBlock.h>
#include <clientserver/stringUtils.h>

#ifdef PUTDATAENABLED
#  include <structures/accessors.h>
#endif // PUTDATAENABLED

#ifdef TESTUDT
#  include <netdb.h>
#endif // TESTUDT

static void init_structure_definitions();
static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test0(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test2(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test4(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test5(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test6(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test7(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test8(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test9(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test9A(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test10(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test11(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test12(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test13(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test14(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test15(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test16(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test18(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test19(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test20(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test21(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test22(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test23(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test24(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test25(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test26(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test27(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test28(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test30(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test31(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_test32(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
#ifdef PUTDATAENABLED
static int do_test40(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
#endif // PUTDATAENABLED
static int do_plugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_errortest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_scalartest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
static int do_emptytest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
#ifdef TESTUDT
static int do_testudt(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);
#endif // TESTUDT

struct TEST9 {
    char v1[56];        // single fixed length string
    char v2[3][56];     // 3 fixed length strings
    char* v3;           // single arbitrary length string
    char* v4[3];        // 3 strings of arbitrary length
    char** v5;          // arbitrary number of strings of arbitrary length
};
typedef struct TEST9 TEST9;

struct TEST9A {
    char v1[56];        // single fixed length string
    char v2[3][56];     // 3 fixed length strings
    char* v3;           // single arbitrary length string
    char* v4[3];        // 3 strings of arbitrary length
    char** v5;          // arbitrary number of strings of arbitrary length
    TEST9 v6;           // Sub Structure with String types
};
typedef struct TEST9A TEST9A;

extern int testplugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    static short init = 0;

    //----------------------------------------------------------------------------------------
    // Standard v1 Plugin Interface

    REQUEST_BLOCK* request_block;

    unsigned short housekeeping;

    if (idam_plugin_interface->interfaceVersion >= 1) {
        idam_plugin_interface->pluginVersion = 1;
        request_block = idam_plugin_interface->request_block;
        housekeeping = idam_plugin_interface->housekeeping;
    } else {
        err = 999;
        idamLog(LOG_ERROR, "ERROR testplugin: Plugin Interface Version Unknown\n");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err,
                     "Plugin Interface Version is Not Known: Unable to execute the request!");
        return err;
    }

    idamLog(LOG_DEBUG, "testplugin: Interface exchanged on entry\n");

    //----------------------------------------------------------------------------------------
    // Heap Housekeeping

    // Plugin must maintain a list of open file handles and sockets: loop over and close all files and sockets
    // Plugin must maintain a list of plugin functions called: loop over and reset state and free heap.
    // Plugin must maintain a list of calls to other plugins: loop over and call each plugin with the housekeeping request
    // Plugin must destroy lists at end of housekeeping

    if (housekeeping || !strcasecmp(request_block->function, "reset")) {
        if (!init) return 0;        // Not previously initialised: Nothing to do!
        init = 0;
        idamLog(LOG_DEBUG, "testplugin: reset function executed\n");
        return 0;
    }

    //----------------------------------------------------------------------------------------
    // Initialise

    if (!init || !strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise")) {
        init = 1;
        idamLog(LOG_DEBUG, "testplugin: plugin initialised\n");
        if (!strcasecmp(request_block->function, "init") || !strcasecmp(request_block->function, "initialise"))
            return 0;
    }

    init_structure_definitions();

    //----------------------------------------------------------------------------------------
    // Plugin Functions
    //----------------------------------------------------------------------------------------

    if (!strcasecmp(request_block->function, "help")) {
        err = do_help(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test0") || !strcasecmp(request_block->function, "test1")) {
        // Single String - not a Structure
        //      test0: passed as a char/byte array
        //      test1: passed as type STRING
        err = do_test0(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test2") || !strcasecmp(request_block->function, "test3")) {
        // Array of Strings - not a Structure
        //      test2: as a rank 2 char/byte array
        //      test3: as an array of type STRING
        err = do_test2(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test4")) {    // Simple Structure
        err = do_test4(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test5")) {    // Simple Structure with String Array
        err = do_test5(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test6")) {    // Simple Structure with String Array
        err = do_test6(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test7")) {    // Simple Structure with String Array
        err = do_test7(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test8")) {    // Simple Structure with String Array
        err = do_test8(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test9")) {    // Array of Structures with various String types
        err = do_test9(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test9A")) {    // Array of Structures with string sub structures
        err = do_test9A(idam_plugin_interface);
    } else

    //=========================================================================================================
    // Integer Tests

    if (!strcasecmp(request_block->function, "test10")) {           // Single Integer
        err = do_test10(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test11")) {    // Simple Structure
        err = do_test11(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test12")) {    // Simple Structure
        err = do_test12(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test13")) {    // Simple Structure
        err = do_test13(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test14")) {    // Simple Structure
        err = do_test14(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test15")) {    // Simple Structure
        err = do_test15(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test16")) {    // Simple Structure
        err = do_test16(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test18")) {    // array of multi-typed Structures
        err = do_test18(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test19")) {    // array of multi-typed Structures
        err = do_test19(idam_plugin_interface);
    }

    //=========================================================================================================
    // Short Integer Tests

    if (!strcasecmp(request_block->function, "test20")) {           // Single Short Integer
        err = do_test20(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test21")) {    // Simple Structure
        err = do_test21(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test22")) {    // Simple Structure
        err = do_test22(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test23")) {    // Simple Structure
        err = do_test23(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test24")) {    // Simple Structure
        err = do_test24(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test25")) {    // Simple Structure
        err = do_test25(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test26")) {    // Simple Structure
        err = do_test26(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test27")) {    // Simple Structure
        err = do_test27(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test28")) {    // Simple Structure
        err = do_test28(idam_plugin_interface);
    } else

    //=====================================================================================================
    // Doubles

    if (!strcasecmp(request_block->function, "test30")) {           // Simple Structure
        err = do_test30(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test31")) {    // Rank 2 Array of Structures
        err = do_test31(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "test32")) {    // Compound Structure
        err = do_test32(idam_plugin_interface);
#ifdef PUTDATAENABLED
    } else if(!strcasecmp(request_block->function, "test40")) {
        err = do_test40(idam_plugin_interface);
#endif

    //=====================================================================================================
    // Misc

    } else if (!strcasecmp(request_block->function, "plugin")) {
        err = do_plugin(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "errortest")) {
        err = do_errortest(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "scalartest")) {
        err = do_scalartest(idam_plugin_interface);
    } else if (!strcasecmp(request_block->function, "emptytest")) {
        err = do_emptytest(idam_plugin_interface);	
#ifdef TESTUDT
    } else if(!strcasecmp(request_block->function, "test40")) {
        err = do_testudt(idam_plugin_interface);
#endif
    } else {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err, "Unknown function requested!");
    }

    //--------------------------------------------------------------------------------------
    // Housekeeping

    idamLog(LOG_DEBUG, "testplugin: housekeeping started\n");

#ifndef USE_PLUGIN_DIRECTLY
    closeIdamError(&idamerrorstack);            // Free local plugin error stack
#endif

    idamLog(LOG_DEBUG, "testplugin: housekeeping completed\n");

    return err;
}

void testError1()
{
// Test of Error Management within Plugins
    int err = 9991;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err, "Test #1 of Error State Management");
}

void testError2()
{
// Test of Error Management within Plugins
    int err = 9992;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err, "Test #2 of Error State Management");
}

// Help: A Description of library functionality
static int do_help(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    idamLog(LOG_DEBUG, "testplugin: help function called\n");

    char* p = (char*) malloc(sizeof(char) * 2 * 1024);

    strcpy(p, "/nTestplugin: Functions Names and Test Descriptions/n/n"
            "test0-test9: String passing tests\n"
            "\ttest0: single string as a char array\n"
            "\ttest1: single string\n"
            "\ttest2: multiple strings as a 2D array of chars\n"
            "\ttest3: array of strings\n"
            "\ttest4: data structure with a fixed length single string\n"
            "\ttest5: data structure with a fixed length multiple string\n"
            "\ttest6: data structure with an arbitrary length single string\n"
            "\ttest7: data structure with a fixed number of arbitrary length strings\n"
            "\ttest8: data structure with an arbitrary number of arbitrary length strings\n\n"
            "\ttest9: array of data structures with a variety of string types\n\n"
            "\ttest9A: array of data structures with a variety of string types and single sub structure\n\n"

            "***test10-test18: Integer passing tests\n"
            "\ttest10: single integer\n"
            "\ttest11: fixed number (rank 1 array) of integers\n"
            "\ttest12: arbitrary number (rank 1 array) of integers\n"
            "\ttest13: fixed length rank 2 array of integers\n"
            "\ttest14: arbitrary length rank 2 array of integers\n"
            "\ttest15: data structure with a single integer\n"
            "\ttest16: data structure with a fixed number of integers\n"
            "\ttest17: data structure with a arbitrary number of integers\n"
            "\ttest18: array of data structures with a variety of integer types\n\n"

            "***test20-test28: Short Integer passing tests\n"
            "\ttest20: single integer\n"
            "\ttest21: fixed number (rank 1 array) of integers\n"
            "\ttest22: arbitrary number (rank 1 array) of integers\n"
            "\ttest23: fixed length rank 2 array of integers\n"
            "\ttest24: arbitrary length rank 2 array of integers\n"
            "\ttest25: data structure with a single integer\n"
            "\ttest26: data structure with a fixed number of integers\n"
            "\ttest27: data structure with a arbitrary number of integers\n"
            "\ttest28: array of data structures with a variety of integer types\n\n"

            "***test30-test32: double passing tests\n"
            "\ttest30: pair of doubles (Coordinate)\n"

            "***test40-test40: put data block receiving tests\n"

            "plugin: test calling other plugins\n"

            "error: Error reporting and server termination tests\n"
    );

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->data_type = TYPE_STRING;
    strcpy(data_block->data_desc, "testplugins: help = description of this plugin");

    data_block->data = p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int) strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    idamLog(LOG_DEBUG, "testplugin: help function completed\n");

    return 0;
}

//----------------------------------------------------------------------------------------
// Structure Definitions
//----------------------------------------------------------------------------------------
static void init_structure_definitions()
{
    USERDEFINEDTYPE* old;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST9");
    strcpy(usertype.source, "Test #9");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST9);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);
    strcpy(field.name, "v1");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char [56]");
    field.pointer = 0;
    field.count = 56;
    field.rank = 1;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "v2");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char [3][56]");
    field.pointer = 0;
    field.count = 3 * 56;
    field.rank = 2;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 56;
    field.shape[1] = 3;
    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "v3");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: char *");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "v4");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING *");            // Array of String pointers
    strcpy(field.desc, "string structure element: char *[3]");
    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 3;
    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "v5");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING *");                // Array of String pointers
    strcpy(field.desc, "string structure element: char **");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(char**);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    idamLog(LOG_DEBUG, "testplugin: Type TEST9 defined\n");

    old = findUserDefinedType("TEST9", 0);            // Clone existing structure & modify
    copyUserDefinedType(old, &usertype);

    idamLog(LOG_DEBUG, "testplugin: Type TEST9 located\n");

    strcpy(usertype.name, "TEST9A");
    strcpy(usertype.source, "Test #9A");
    usertype.size = sizeof(TEST9A);            // Structure size

    offset = old->size;

    initCompoundField(&field);
    strcpy(field.name, "v6");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "TEST9");                // Array of String pointers
    strcpy(field.desc, "string structure elements with sub structure");
    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;
    field.size = field.count * sizeof(TEST9);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    idamLog(LOG_DEBUG, "testplugin: Type TEST9A defined\n");
}

static int do_test0(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    char* p = (char*) malloc(sizeof(char) * 30);

    strcpy(p, "Hello World!");

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    int i;
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    if (!strcasecmp(request_block->function, "test0")) {
        data_block->data_type = TYPE_CHAR;
        strcpy(data_block->data_desc, "testplugins: test0 = single string as a char array");
    } else {
        data_block->data_type = TYPE_STRING;
        strcpy(data_block->data_desc, "testplugins: test1 = single string");
    }

    data_block->data = p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int) strlen(p) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

static int do_test2(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;


// An array of strings can be formed in two distinct ways.
// 1> A fixed block of contiguous memory with each string beginning at a well defined regular location - as if each
//    string were the same length.
// 2> An array of string pointers: each string has its own length. Memory is not contiguous. This is the normal
//    representation of string arrays.

// To pass back the data as a block of chars/bytes or as type STRING, model 1 must be adopted - its how the middleware operates.
// By labeling the type as STRING, we can convert the data within the client to the correct type


// create original data using model 2

    int sCount = 3;
    char** sarr = (char**) malloc(sCount * sizeof(char*));

    int i;
    for (i = 0; i < sCount; i++) {
        sarr[i] = (char*) malloc(30 * sizeof(char));
    }

    strcpy(sarr[0], "Hello World!");
    strcpy(sarr[1], "Qwerty keyboard");
    strcpy(sarr[2], "MAST Upgrade");

// Maximum size of any individual string

    int sLen = 0, sMax = 0;
    for (i = 0; i < sCount; i++) if ((sLen = (int) strlen(sarr[1]) + 1) > sMax) sMax = sLen;

// Create a block of contigous memory and assign all bytes to NULL character

    char* p = (char*) malloc(sLen * sCount * sizeof(char));
    for (i = 0; i < sLen * sCount; i++) p[i] = (char) 0;

// Copy string data into the block positioned at regular intervals

    for (i = 0; i < sCount; i++) strcpy(&p[i * sMax], sarr[i]);

// Free original data

    for (i = 0; i < sCount; i++) free((void*) sarr[i]);
    free((void*) sarr);

    initDataBlock(data_block);

    data_block->rank = 2;
    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    if (!strcasecmp(request_block->function, "test2")) {
        data_block->data_type = TYPE_CHAR;
        strcpy(data_block->data_desc, "testplugins: test2 = 2D array of chars");
    } else {
        data_block->data_type = TYPE_STRING;
        strcpy(data_block->data_desc, "testplugins: test3 = array of strings");
    }

    data_block->data = p;

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = sMax;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->dims[1].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[1].dim_n = sCount;
    data_block->dims[1].compressed = 1;
    data_block->dims[1].dim0 = 0.0;
    data_block->dims[1].diff = 1.0;
    data_block->dims[1].method = 0;

    data_block->data_n = data_block->dims[0].dim_n * data_block->dims[1].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

static int do_test4(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST4 {
        char value[56];
    };
    typedef struct TEST4 TEST4;

    TEST4* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST4");
    strcpy(usertype.source, "Test #4");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST4);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: value[56]");

    field.pointer = 0;
    field.count = 56;
    field.rank = 1;

    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST4*) malloc(sizeof(TEST4));            // Structured Data Must be a heap variable
    strcpy(data->value, "012345678901234567890");
    addMalloc((void*) data, 1, sizeof(TEST4), "TEST4");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #4");
    strcpy(data_block->data_label, "Values: 012345678901234567890");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST4", 0);

    return 0;
}

static int do_test5(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST5 {
        char value[3][56];
    };
    typedef struct TEST5 TEST5;

    TEST5* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST5");
    strcpy(usertype.source, "Test #5");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST5);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: value[3][56]");

    field.pointer = 0;
    field.count = 3 * 56;
    field.rank = 2;

    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 56;
    field.shape[1] = 3;

    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST5*) malloc(sizeof(TEST5));            // Structured Data Must be a heap variable
    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");
    addMalloc((void*) data, 1, sizeof(TEST5), "TEST5");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #5");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST5", 0);

    return 0;
}

static int do_test6(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST6 {
        char* value;
    };
    typedef struct TEST6 TEST6;

    TEST6* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST6");
    strcpy(usertype.source, "Test #6");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST6);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING");            // convert atomic type to a string label
    strcpy(field.desc, "string structure element: *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;

    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST6*) malloc(sizeof(TEST6));            // Structured Data Must be a heap variable
    data->value = (char*) malloc(56 * sizeof(char));
    strcpy(data->value, "PI=3.1415927");
    addMalloc((void*) data, 1, sizeof(TEST6), "TEST6");
    addMalloc((void*) data->value, 1, 56 * sizeof(char), "char");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #6");
    strcpy(data_block->data_label, "Value: PI=3.1415927");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST6", 0);

    return 0;
}

static int do_test7(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST7 {
        char* value[3];        // 3 strings of arbitrary length
    };
    typedef struct TEST7 TEST7;

    TEST7* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST7");
    strcpy(usertype.source, "Test #7");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST7);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING *");            // Array of String pointers
    strcpy(field.desc, "string structure element: *value[3]");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;

    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 3;

    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST7*) malloc(sizeof(TEST7));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST7), "TEST7");

    data->value[0] = (char*) malloc(56 * sizeof(char));
    data->value[1] = (char*) malloc(55 * sizeof(char));
    data->value[2] = (char*) malloc(54 * sizeof(char));

    addMalloc((void*) data->value[0], 56, sizeof(char), "char");
    addMalloc((void*) data->value[1], 55, sizeof(char), "char");
    addMalloc((void*) data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");


// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #7");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST7", 0);

    return 0;
}

static int do_test8(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST8 {
        char** value;        // arbitrary number of strings of arbitrary length
    };
    typedef struct TEST8 TEST8;

    TEST8* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST8");
    strcpy(usertype.source, "Test #8");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST8);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_STRING;
    strcpy(field.type, "STRING *");                // Array of String pointers
    strcpy(field.desc, "string structure element: **value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;

    field.size = field.count * sizeof(char**);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST8*) malloc(sizeof(TEST8));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST8), "TEST8");

    data->value = (char**) malloc(3 * sizeof(char*));
    addMalloc((void*) data->value, 3, sizeof(char*), "STRING *");

    data->value[0] = (char*) malloc(56 * sizeof(char));
    data->value[1] = (char*) malloc(55 * sizeof(char));
    data->value[2] = (char*) malloc(54 * sizeof(char));

    addMalloc((void*) data->value[0], 56, sizeof(char), "char");
    addMalloc((void*) data->value[1], 55, sizeof(char), "char");
    addMalloc((void*) data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #8");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST8", 0);

    return 0;
}

static int do_test9(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Create Data

    TEST9* data = (TEST9*) malloc(4 * sizeof(TEST9));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 4, sizeof(TEST9), "TEST9");

    int i;
    for (i = 0; i < 4; i++) {
        strcpy(data[i].v1, "123212321232123212321");
        strcpy(data[i].v2[0], "012345678901234567890");
        strcpy(data[i].v2[1], "QWERTY KEYBOARD");
        strcpy(data[i].v2[2], "MAST TOKAMAK");

        data[i].v3 = (char*) malloc(56 * sizeof(char));
        strcpy(data[i].v3, "PI=3.1415927");
        addMalloc((void*) data[i].v3, 1, 56 * sizeof(char), "char");

        data[i].v4[0] = (char*) malloc(56 * sizeof(char));
        data[i].v4[1] = (char*) malloc(55 * sizeof(char));
        data[i].v4[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v4[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v4[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v4[2], 54, sizeof(char), "char");
        strcpy(data[i].v4[0], "012345678901234567890");
        strcpy(data[i].v4[1], "QWERTY KEYBOARD");
        strcpy(data[i].v4[2], "MAST TOKAMAK");

        data[i].v5 = (char**) malloc(3 * sizeof(char*));
        addMalloc((void*) data[i].v5, 3, sizeof(char*), "STRING *");
        data[i].v5[0] = (char*) malloc(56 * sizeof(char));
        data[i].v5[1] = (char*) malloc(55 * sizeof(char));
        data[i].v5[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v5[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v5[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v5[2], 54, sizeof(char), "char");
        strcpy(data[i].v5[0], "012345678901234567890");
        strcpy(data[i].v5[1], "QWERTY KEYBOARD");
        strcpy(data[i].v5[2], "MAST TOKAMAK");
    }

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data_n = 4;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #9");
    strcpy(data_block->data_label, "Multiple test results");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST9", 0);

    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));

    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

static int do_test9A(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Create Data

    TEST9A* data = (TEST9A*) malloc(4 * sizeof(TEST9A));    // Structured Data Must be a heap variable
    addMalloc((void*) data, 4, sizeof(TEST9A), "TEST9A");

    int i;
    for (i = 0; i < 4; i++) {
        strcpy(data[i].v1, "123212321232123212321");
        strcpy(data[i].v2[0], "012345678901234567890");
        strcpy(data[i].v2[1], "QWERTY KEYBOARD");
        strcpy(data[i].v2[2], "MAST TOKAMAK");

        data[i].v3 = (char*) malloc(56 * sizeof(char));
        strcpy(data[i].v3, "PI=3.1415927");
        addMalloc((void*) data[i].v3, 1, 56 * sizeof(char), "char");

        data[i].v4[0] = (char*) malloc(56 * sizeof(char));
        data[i].v4[1] = (char*) malloc(55 * sizeof(char));
        data[i].v4[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v4[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v4[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v4[2], 54, sizeof(char), "char");
        strcpy(data[i].v4[0], "012345678901234567890");
        strcpy(data[i].v4[1], "QWERTY KEYBOARD");
        strcpy(data[i].v4[2], "MAST TOKAMAK");

        data[i].v5 = (char**) malloc(3 * sizeof(char*));
        addMalloc((void*) data[i].v5, 3, sizeof(char*), "STRING *");
        data[i].v5[0] = (char*) malloc(56 * sizeof(char));
        data[i].v5[1] = (char*) malloc(55 * sizeof(char));
        data[i].v5[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v5[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v5[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v5[2], 54, sizeof(char), "char");
        strcpy(data[i].v5[0], "012345678901234567890");
        strcpy(data[i].v5[1], "QWERTY KEYBOARD");
        strcpy(data[i].v5[2], "MAST TOKAMAK");

        strcpy(data[i].v6.v1, "123212321232123212321");
        strcpy(data[i].v6.v2[0], "012345678901234567890");
        strcpy(data[i].v6.v2[1], "QWERTY KEYBOARD");
        strcpy(data[i].v6.v2[2], "MAST TOKAMAK");

        data[i].v6.v3 = (char*) malloc(56 * sizeof(char));
        strcpy(data[i].v6.v3, "PI=3.1415927");
        addMalloc((void*) data[i].v6.v3, 1, 56 * sizeof(char), "char");

        data[i].v6.v4[0] = (char*) malloc(56 * sizeof(char));
        data[i].v6.v4[1] = (char*) malloc(55 * sizeof(char));
        data[i].v6.v4[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v6.v4[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v6.v4[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v6.v4[2], 54, sizeof(char), "char");
        strcpy(data[i].v6.v4[0], "012345678901234567890");
        strcpy(data[i].v6.v4[1], "QWERTY KEYBOARD");
        strcpy(data[i].v6.v4[2], "MAST TOKAMAK");

        data[i].v6.v5 = (char**) malloc(3 * sizeof(char*));
        addMalloc((void*) data[i].v6.v5, 3, sizeof(char*), "STRING *");
        data[i].v6.v5[0] = (char*) malloc(56 * sizeof(char));
        data[i].v6.v5[1] = (char*) malloc(55 * sizeof(char));
        data[i].v6.v5[2] = (char*) malloc(54 * sizeof(char));
        addMalloc((void*) data[i].v6.v5[0], 56, sizeof(char), "char");
        addMalloc((void*) data[i].v6.v5[1], 55, sizeof(char), "char");
        addMalloc((void*) data[i].v6.v5[2], 54, sizeof(char), "char");
        strcpy(data[i].v6.v5[0], "012345678901234567890");
        strcpy(data[i].v6.v5[1], "QWERTY KEYBOARD");
        strcpy(data[i].v6.v5[2], "MAST TOKAMAK");

    }

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data_n = 4;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #9A");
    strcpy(data_block->data_label, "Multiple test results");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST9A", 0);

    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

static int do_test10(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Create Data

    int* data = (int*) malloc(sizeof(int));
    data[0] = 7;

// Pass Data

    data_block->data_type = TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #10");
    strcpy(data_block->data_label, "Value: 7");
    strcpy(data_block->data_units, "");

    return 0;
}

static int do_test11(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST11 {
        int value;
    };
    typedef struct TEST11 TEST11;

    TEST11* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST11");
    strcpy(usertype.source, "Test #11");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST11);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST11*) malloc(sizeof(TEST11));            // Structured Data Must be a heap variable
    data[0].value = 11;
    addMalloc((void*) data, 1, sizeof(TEST11), "TEST11");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #11");
    strcpy(data_block->data_label, "Value: 11");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST11", 0);

    return 0;
}

static int do_test12(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST12 {
        int value[3];
    };
    typedef struct TEST12 TEST12;

    TEST12* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST12");
    strcpy(usertype.source, "Test #12");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST12);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST12*) malloc(sizeof(TEST12));            // Structured Data Must be a heap variable
    data[0].value[0] = 10;
    data[0].value[1] = 11;
    data[0].value[2] = 12;
    addMalloc((void*) data, 1, sizeof(TEST12), "TEST12");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #12");
    strcpy(data_block->data_label, "Values: 10,11,12");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST12", 0);

    return 0;
}

static int do_test13(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST13 {
        int value[2][3];
    };
    typedef struct TEST13 TEST13;

    TEST13* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST13");
    strcpy(usertype.source, "Test #13");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST13);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 6;
    field.rank = 2;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 2;
    field.shape[1] = 3;                // Reversed ... Fortran/IDL like

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST13*) malloc(sizeof(TEST13));            // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    addMalloc((void*) data, 1, sizeof(TEST13), "TEST13");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #13");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST13", 0);

    return 0;
}

static int do_test14(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST14 {
        int* value;
    };
    typedef struct TEST14 TEST14;

    TEST14* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST14");
    strcpy(usertype.source, "Test #14");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST14);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: int *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST14*) malloc(sizeof(TEST14));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST14), "TEST14");

    data[0].value = (int*) malloc(sizeof(int));
    addMalloc((void*) data[0].value, 1, sizeof(int), "int");

    data[0].value[0] = 14;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #14");
    strcpy(data_block->data_label, "int *value: 14");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST14", 0);

    return 0;
}

static int do_test15(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST15 {
        int* value;
    };
    typedef struct TEST15 TEST15;

    TEST15* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST15");
    strcpy(usertype.source, "Test #15");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST15);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST15*) malloc(sizeof(TEST15));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST15), "TEST15");

    data[0].value = (int*) malloc(3 * sizeof(int));
    addMalloc((void*) data[0].value, 3, sizeof(int), "int");

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #15");
    strcpy(data_block->data_label, "Values: 13,14,15");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST15", 0);

    return 0;
}

static int do_test16(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST16 {
        int* value;
    };
    typedef struct TEST16 TEST16;

    TEST16* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST16");
    strcpy(usertype.source, "Test #16");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST16);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST16*) malloc(sizeof(TEST16));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST16), "TEST16");

    int* shape = (int*) malloc(2 * sizeof(int));
    shape[0] = 2;
    shape[1] = 3;
    int count = shape[0] * shape[1];
    int rank = 2;
    data[0].value = (int*) malloc(count * sizeof(int));
    addMalloc2((void*) data[0].value, count, sizeof(int), "int", rank, shape);

    data[0].value[0] = 0;
    data[0].value[1] = 1;
    data[0].value[2] = 2;
    data[0].value[3] = 10;
    data[0].value[4] = 11;
    data[0].value[5] = 12;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #16");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST16", 0);

    return 0;
}

static int do_test18(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST18 {
        int value;
    };
    typedef struct TEST18 TEST18;

    TEST18* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST18");
    strcpy(usertype.source, "Test #18");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST18);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data_block->data_n = 100000;
    data = (TEST18*) malloc(data_block->data_n * sizeof(TEST18)); // Structured Data Must be a heap variable

    int i;
    for (i = 0; i < data_block->data_n; i++) {
        data[i].value = i;
    }
    addMalloc((void*) data, data_block->data_n, sizeof(TEST18), "TEST18");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Array Integer Data Test #18");
    strcpy(data_block->data_label, "100000 Values: i 0, 100000");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST18", 0);

    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

static int do_test19(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST19A {
        int value;
    };
    typedef struct TEST19A TEST19A;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST19A");
    strcpy(usertype.source, "Test #19");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST19A);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");
    strcpy(field.desc, "integer structure element");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    struct TEST19 {
        int value;
        TEST19A vals[7];
    };
    typedef struct TEST19 TEST19;

    TEST19* data;

    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST19");
    strcpy(usertype.source, "Test #19");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST19);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");
    strcpy(field.desc, "integer");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);

    strcpy(field.name, "vals");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "TEST19A");
    strcpy(field.desc, "structure TEST19A");

    field.pointer = 0;
    field.count = 7;
    field.rank = 1;

    field.shape = (int*) malloc(field.rank * sizeof(int));            // Needed when rank >= 1
    field.shape[0] = 7;

    field.size = field.count * sizeof(TEST19A);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data_block->data_n = 3;
    data = (TEST19*) malloc(data_block->data_n * sizeof(TEST19)); // Structured Data Must be a heap variable

    int i, j;
    for (i = 0; i < data_block->data_n; i++) {
        data[i].value = 3 + i;
        for (j = 0; j < 7; j++) {
            data[i].vals[j].value = 10 * i + j;
        }
    }
    addMalloc((void*) data, data_block->data_n, sizeof(TEST19), "TEST19");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Test #19");
    strcpy(data_block->data_label, "Values: ");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST19", 0);

    data_block->dims = (DIMS*) malloc(data_block->rank * sizeof(DIMS));
    for (i = 0; i < data_block->rank; i++) initDimBlock(&data_block->dims[i]);

    data_block->dims[0].data_type = TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

static int do_test20(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

// Create Data

    short* data = (short*) malloc(sizeof(short));
    data[0] = 7;

// Pass Data

    data_block->data_type = TYPE_SHORT;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #20");
    strcpy(data_block->data_label, "Short Value: 7");
    strcpy(data_block->data_units, "");

    return 0;
}

static int do_test21(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST21 {
        short value;
    };
    typedef struct TEST21 TEST21;

    TEST21* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST21");
    strcpy(usertype.source, "Test #21");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST21);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "single short integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST21*) malloc(sizeof(TEST21));            // Structured Data Must be a heap variable
    data[0].value = 21;
    addMalloc((void*) data, 1, sizeof(TEST21), "TEST21");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #21");
    strcpy(data_block->data_label, "Short Value: 21");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST21", 0);

    return 0;
}

static int do_test22(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST22 {
        short value[3];
    };
    typedef struct TEST22 TEST22;

    TEST22* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST22");
    strcpy(usertype.source, "Test #22");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST22);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "single short integer structure element: value");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST22*) malloc(sizeof(TEST22));            // Structured Data Must be a heap variable
    data[0].value[0] = 20;
    data[0].value[1] = 21;
    data[0].value[2] = 22;
    addMalloc((void*) data, 1, sizeof(TEST22), "TEST22");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #22");
    strcpy(data_block->data_label, "Short Array Values: 20,21,22");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST22", 0);

    return 0;
}

static int do_test23(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST23 {
        short value[2][3];
    };
    typedef struct TEST23 TEST23;

    TEST23* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST23");
    strcpy(usertype.source, "Test #23");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST23);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short integer array: value");

    field.pointer = 0;
    field.count = 6;
    field.rank = 2;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 3;
    field.shape[1] = 2;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST23*) malloc(sizeof(TEST23));            // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    addMalloc((void*) data, 1, sizeof(TEST23), "TEST23");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #23");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST23", 0);

    return 0;
}

static int do_test24(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST24 {
        short* value;
    };
    typedef struct TEST24 TEST24;

    TEST24* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST24");
    strcpy(usertype.source, "Test #24");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST24);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST24*) malloc(sizeof(TEST24));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST24), "TEST24");

    data[0].value = (short*) malloc(sizeof(short));
    addMalloc((void*) data[0].value, 1, sizeof(short), "short");

    data[0].value[0] = 14;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #24");
    strcpy(data_block->data_label, "short *value: 14");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST24", 0);

    return 0;
}

static int do_test25(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST25 {
        short* value;
    };
    typedef struct TEST25 TEST25;

    TEST25* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST25");
    strcpy(usertype.source, "Test #25");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST25);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST25*) malloc(sizeof(TEST25));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST25), "TEST25");

    data[0].value = (short*) malloc(3 * sizeof(short));
    addMalloc((void*) data[0].value, 3, sizeof(short), "short");

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #25");
    strcpy(data_block->data_label, "Short Values: 13,14,15");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST25", 0);

    return 0;
}

static int do_test26(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST26 {
        short* value;
    };
    typedef struct TEST26 TEST26;

    TEST26* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST26");
    strcpy(usertype.source, "Test #26");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST26);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data Structure

    data = (TEST26*) malloc(sizeof(TEST26));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST26), "TEST26");

// Data is a compact Fortran like rank 2 array

    data[0].value = (short*) malloc(6 * sizeof(short));
    int* shape = (int*) malloc(2 * sizeof(int));
    shape[0] = 3;
    shape[1] = 2;
    addMalloc2((void*) data[0].value, 6, sizeof(short), "short", 2, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    data[0].value[3] = 23;
    data[0].value[4] = 24;
    data[0].value[5] = 25;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #26");
    strcpy(data_block->data_label, "Short Values: 13,14,15   23,24,25");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST26", 0);

    return 0;
}

static int do_test27(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST27 {
        short value[2][3][4];
    };
    typedef struct TEST27 TEST27;

    TEST27* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST27");
    strcpy(usertype.source, "Test #27");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST27);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short integer array: value");

    field.pointer = 0;
    field.count = 24;
    field.rank = 3;
    field.shape = (int*) malloc(field.rank * sizeof(int));        // Needed when rank >= 1
    field.shape[0] = 4;
    field.shape[1] = 3;
    field.shape[2] = 2;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST27*) malloc(sizeof(TEST27));            // Structured Data Must be a heap variable

    data[0].value[0][0][0] = 0;
    data[0].value[0][0][1] = 1;
    data[0].value[0][0][2] = 2;
    data[0].value[0][0][3] = 3;
    data[0].value[0][1][0] = 10;
    data[0].value[0][1][1] = 11;
    data[0].value[0][1][2] = 12;
    data[0].value[0][1][3] = 13;
    data[0].value[0][2][0] = 20;
    data[0].value[0][2][1] = 21;
    data[0].value[0][2][2] = 22;
    data[0].value[0][2][3] = 23;

    data[0].value[1][0][0] = 100;
    data[0].value[1][0][1] = 101;
    data[0].value[1][0][2] = 102;
    data[0].value[1][0][3] = 103;
    data[0].value[1][1][0] = 110;
    data[0].value[1][1][1] = 111;
    data[0].value[1][1][2] = 112;
    data[0].value[1][1][3] = 113;
    data[0].value[1][2][0] = 120;
    data[0].value[1][2][1] = 121;
    data[0].value[1][2][2] = 122;
    data[0].value[1][2][3] = 123;

    addMalloc((void*) data, 1, sizeof(TEST27), "TEST27");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #27");
    strcpy(data_block->data_label, "Values: {0,1,2,3},{10,11,12,13},...");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST27", 0);

    return 0;
}

static int do_test28(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST28 {
        short* value;
    };
    typedef struct TEST28 TEST28;

    TEST28* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST28");
    strcpy(usertype.source, "Test #28");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST28);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = TYPE_SHORT;
    strcpy(field.type, "short");            // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data Structure

    data = (TEST28*) malloc(sizeof(TEST28));            // Structured Data Must be a heap variable
    addMalloc((void*) data, 1, sizeof(TEST28), "TEST28");

// Data is a compact Fortran like rank 3 array

    data[0].value = (short*) malloc(24 * sizeof(short));
    int* shape = (int*) malloc(3 * sizeof(int));
    shape[0] = 4;
    shape[1] = 3;
    shape[2] = 2;
    addMalloc2((void*) data[0].value, 24, sizeof(short), "short", 3, shape);

    int index = 0;
    data[0].value[index++] = 0;
    data[0].value[index++] = 1;
    data[0].value[index++] = 2;
    data[0].value[index++] = 3;
    data[0].value[index++] = 10;
    data[0].value[index++] = 11;
    data[0].value[index++] = 12;
    data[0].value[index++] = 13;
    data[0].value[index++] = 20;
    data[0].value[index++] = 21;
    data[0].value[index++] = 22;
    data[0].value[index++] = 23;
    data[0].value[index++] = 100;
    data[0].value[index++] = 101;
    data[0].value[index++] = 102;
    data[0].value[index++] = 103;
    data[0].value[index++] = 110;
    data[0].value[index++] = 111;
    data[0].value[index++] = 112;
    data[0].value[index++] = 113;
    data[0].value[index++] = 120;
    data[0].value[index++] = 121;
    data[0].value[index++] = 122;
    data[0].value[index] = 123;

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #28");
    strcpy(data_block->data_label, "Short Values: 13,14,15   23,24,25");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST28", 0);

    return 0;
}

static int do_test30(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST30 {
        double R;
        double Z;
    };
    typedef struct TEST30 TEST30;

    TEST30* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST30");
    strcpy(usertype.source, "Test #30");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST30);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);
    strcpy(field.name, "R");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: R");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "Z");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: Z");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    data = (TEST30*) malloc(sizeof(TEST30));            // Structured Data Must be a heap variable
    data[0].R = 1.0;
    data[0].Z = 2.0;
    addMalloc((void*) data, 1, sizeof(TEST30), "TEST30");

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #30");
    strcpy(data_block->data_label, "Double Values: (1, 2)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST30", 0);

    return 0;
}

static int do_test31(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST30 {
        double R;
        double Z;
    };
    typedef struct TEST30 TEST30;

    TEST30* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST30");
    strcpy(usertype.source, "Test #31");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST30);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);
    strcpy(field.name, "R");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: R");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "Z");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: Z");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    int count = 100;
    data = (TEST30*) malloc(count * sizeof(TEST30));            // Structured Data Must be a heap variable

    offset = 0;

    int i, j;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 20; j++) {
            data[offset].R = (double) offset;
            data[offset].Z = 10.0 * (double) offset;
            offset++;
        }
    }

    int rank = 2;
    int* shape = (int*) malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    addMalloc((void*) shape, 2, sizeof(int), "int");
    addMalloc2((void*) data, count, sizeof(TEST30), "TEST30", rank, shape);

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Structure Data Test #30");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST30", 0);

    return 0;
}

static int do_test32(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    struct TEST30 {
        double R;
        double Z;
    };
    typedef struct TEST30 TEST30;

    struct TEST32 {
        int count;
        TEST30* coords;
    };
    typedef struct TEST32 TEST32;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST30");
    strcpy(usertype.source, "Test #32");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST30);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);
    strcpy(field.name, "R");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: R");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "Z");
    field.atomictype = TYPE_DOUBLE;
    strcpy(field.type, "double");            // convert atomic type to a string label
    strcpy(field.desc, "double structure element: Z");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(double);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST32");
    strcpy(usertype.source, "Test #32");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST32);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "count");
    field.atomictype = TYPE_INT;
    strcpy(field.type, "int");            // convert atomic type to a string label
    strcpy(field.desc, "int structure element: count");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    initCompoundField(&field);
    strcpy(field.name, "coords");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "TEST30");            // convert atomic type to a string label
    strcpy(field.desc, "TEST30 array structure element: coords");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(TEST30);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    TEST32* data = (TEST32*) malloc(sizeof(TEST32));

    int count = 100;
    TEST30* coords = (TEST30*) malloc(count * sizeof(TEST30)); // Structured Data Must be a heap variable

    offset = 0;

    int i, j;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 20; j++) {
            coords[offset].R = (double) offset;
            coords[offset].Z = 10.0 * (double) offset;
            offset++;
        }
    }
    data->coords = coords;
    data->count = 100;

    int rank = 2;
    int* shape = (int*) malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    addMalloc((void*) data, 1, sizeof(TEST32), "TEST32");
    addMalloc2((void*) coords, count, sizeof(TEST30), "TEST30", rank, shape);

// Pass Data

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Compound Structure Data Test #32");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST32", 0);

    return 0;
}

#ifdef PUTDATAENABLED
//======================================================================================
// Receiving Put Data Blocks - structured data as arguments to a plugin
// Echo passed data back as an array of structures
// The library won't build if the server version is not OK for this functionality
// If the client cannot pass putdata blocks then no data will appear here to process.
static int do_test40(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    struct TEST40 {
        unsigned int dataCount;
        void* data;
    };
    typedef struct TEST40 TEST40;

    struct TEST41 {
        int count;
        TEST40* blocks;
    };
    typedef struct TEST41 TEST41;

    int err = 0;

    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST40");
    strcpy(usertype.source, "Test #40");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST40);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

// The number of data blocks is given by: request_block->putDataBlockList.blockCount
// For this test, all blocks must be of the same type: request_block->putDataBlockList.putDataBlock[0].data_type;
// Repeat call with changing types may cause client side issues!

    idamLog(LOG_DEBUG, "Number of PutData Blocks: %d\n", request_block->putDataBlockList.blockCount);

    if (request_block->putDataBlockList.blockCount == 0) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err, "No Put Data Blocks to process!");
        return err;
    }

    defineField(&field, "dataCount", "the number of data array elements", &offset, SCALARUINT);

    switch (request_block->putDataBlockList.putDataBlock[0].data_type) {
        case (TYPE_INT):
            defineField(&field, "data", "the block data array", &offset, ARRAYINT);
            break;
        case (TYPE_FLOAT):
            defineField(&field, "data", "the block data array", &offset, ARRAYFLOAT);
            break;
        case (TYPE_DOUBLE):
            defineField(&field, "data", "the block data array", &offset, ARRAYDOUBLE);
            break;
    }

    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype);            // New structure definition

    strcpy(usertype.name, "TEST41");
    strcpy(usertype.source, "Test #41");
    usertype.ref_id = 0;
    usertype.imagecount = 0;                // No Structure Image data
    usertype.image = NULL;
    usertype.size = sizeof(TEST41);            // Structure size
    usertype.idamclass = TYPE_COMPOUND;

    offset = 0;
    initCompoundField(&field);

    defineField(&field, "count", "the number of data blocks", &offset, SCALARUINT);

    strcpy(field.name, "blocks");
    field.atomictype = TYPE_UNKNOWN;
    strcpy(field.type, "TEST40");
    strcpy(field.desc, "Array of Block Data");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = NULL;            // Needed when rank >= 1

    field.size = field.count * sizeof(TEST40);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size;    // Next Offset
    addCompoundField(&usertype, field);        // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

// Create Data

    TEST41* data = (TEST41*) malloc(sizeof(TEST41));
    TEST40* blocks = (TEST40*) malloc(request_block->putDataBlockList.blockCount * sizeof(TEST40));

    addMalloc((void*) data, 1, sizeof(TEST41), "TEST41");
    addMalloc((void*) blocks, request_block->putDataBlockList.blockCount, sizeof(TEST40), "TEST40");

    data->count = request_block->putDataBlockList.blockCount;

    int i;
    for (i = 0; i < request_block->putDataBlockList.blockCount; i++) {
        blocks[i].dataCount = request_block->putDataBlockList.putDataBlock[i].count;
        blocks[i].data = (void*) request_block->putDataBlockList.putDataBlock[i].data;

        idamLog(LOG_DEBUG, "data type : %d\n", request_block->putDataBlockList.putDataBlock[0].data_type);
        idamLog(LOG_DEBUG, "data count: %d\n", request_block->putDataBlockList.putDataBlock[0].count);

// Data blocks already allocated and will be freed by a separate process so use addNonMalloc instead of addMalloc

        switch (request_block->putDataBlockList.putDataBlock[0].data_type) {
            case (TYPE_INT):
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(int), "int");
                break;
            case (TYPE_FLOAT):
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(float), "float");
                float* f = (float*) blocks[i].data;
                break;
            case (TYPE_DOUBLE):
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(double), "double");
                break;
        }
    }

// Pass Data back

    data_block->data_type = TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*) data;

    strcpy(data_block->data_desc, "Test of receiving passed data blocks #40");
    strcpy(data_block->data_label, "Data type TEST41 with array of TEST40");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*) findUserDefinedType("TEST41", 0);

    return 0;
}
#endif // PUTDATAENABLED

//======================================================================================
// Test direct calling of plugins from this plugin
// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy
static int do_plugin(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    int err = 0;

    PLUGINLIST* pluginList;    // List of all data reader plugins (internal and external shared libraries)
    IDAM_PLUGIN_INTERFACE next_plugin_interface;
    REQUEST_BLOCK next_request_block;

    pluginList = idam_plugin_interface->pluginList;

    if (pluginList == NULL) {
        err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE,
                     "No plugins available for this data request", err, "");
        return err;
    }

// Test specifics

    char* signal = NULL;
    char* source = NULL;

    int i;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "signal")) {
            signal = request_block->nameValueList.nameValue[i].value;
            continue;
        }
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "source")) {
            source = request_block->nameValueList.nameValue[i].value;
            continue;
        }
    }

    if (signal != NULL || source != NULL) {            // Identify the plugin to test

        next_plugin_interface = *idam_plugin_interface;    // New plugin interface

        next_plugin_interface.request_block = &next_request_block;
        initServerRequestBlock(&next_request_block);
        strcpy(next_request_block.api_delim, request_block->api_delim);

        if (signal != NULL) strcpy(next_request_block.signal, signal);
        if (source != NULL) strcpy(next_request_block.source, source);

        makeServerRequestBlock(&next_request_block, *pluginList);

        for (i = 0; i < pluginList->count; i++) {
            if (next_request_block.request == pluginList->plugin[i].request) {
                if (pluginList->plugin[i].idamPlugin != NULL) {
                    err = pluginList->plugin[i].idamPlugin(
                            &next_plugin_interface);        // Call the data reader
                } else {
                    err = 999;
                    addIdamError(&idamerrorstack, CODEERRORTYPE,
                                 "No Data Access plugin available for this data request", err, "");
                }
                break;
            }
        }

        freeNameValueList(&next_request_block.nameValueList);
    }

    return err;
}

//======================================================================================
// Test error condition passing...

// The IDAM server uses a data structure with global scope to stack error messages and codes.
// The scope does not extend to external plugin libraries.
// Adding errors to an error stack is done using the function exposed by the server library.
// No state is maintained by this function and the stack structure is passed by argument.
// The server error structure is passed into plugins using an accessor function: getIdamServerPluginErrorStack()
// To maintain consitency with existing legacy code, use a local stucture with a global scope (plugin library only)
// A final necessary step is to concatenate this local structure with the server structure before returning.
// When testing the plugin, errors are doubled (tripled!) up as both stacks are the same.
static int do_errortest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    int err = 0;
    int test = 0;

    REQUEST_BLOCK* request_block = idam_plugin_interface->request_block;

    int i;
    for (i = 0; i < request_block->nameValueList.pairCount; i++) {
        if (!strcasecmp(request_block->nameValueList.nameValue[i].name, "test")) {
            if (IsNumber(request_block->nameValueList.nameValue[i].value))
                test = atoi(request_block->nameValueList.nameValue[i].value);
            break;
        }
    }

    if (test == 1) {
        testError1();
        return err;
    }
    if (test == 2) {
        testError2();
        return err;
    }
    if (test == 3) {
        char* p = "crash!";        // force a server crash! (write to read-only memory)
        *p = '*';

        p = NULL;
        free(p);

        int* p2 = NULL;
        *p2 = 1;
    }

    err = 9990 + test;
    addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin", err, "Test of Error State Management");
    return err;
}

static int do_scalartest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;

    initDataBlock(data_block);

    int* p = malloc(sizeof(int));
    *p = 10;
    data_block->data = (char*) p;
    data_block->data_n = 1;
    data_block->data_type = TYPE_INT;

    return 0;
}


static int do_emptytest(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    DATA_BLOCK* data_block = idam_plugin_interface->data_block;
    initDataBlock(data_block);
    return 0;
}

#ifdef TESTUDT

// rendezvous == false is default
int createUDTSocket(int* usock, int port, int rendezvous)
{
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char service[16];
    sprintf(service, "%d", port);

    if (0 != getaddrinfo(NULL, service, &hints, &res)) {
        int err = 9991;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createUDTSocket", err,
                     "Illegal port number or port is busy");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createUDTSocket", err,
                     (char*) udt_getlasterror_desc());
        return -1;
    }

    *usock = udt_socket(res->ai_family, res->ai_socktype,
                        res->ai_protocol);    // AF_INET, SOCK_STREAM, default protocol

// since we will start a lot of connections, we set the buffer size to smaller value.

// UDT buffer size limit (default 10MB)
    int snd_buf = 16000;
    int rcv_buf = 16000;
    udt_setsockopt(*usock, 0, UDT_UDT_SNDBUF, &snd_buf, sizeof(int));
    udt_setsockopt(*usock, 0, UDT_UDT_RCVBUF, &rcv_buf, sizeof(int));

// UDP buffer size limit (default 1MB)
    snd_buf = 8192;
    rcv_buf = 8192;
    udt_setsockopt(*usock, 0, UDT_UDP_SNDBUF, &snd_buf, sizeof(int));
    udt_setsockopt(*usock, 0, UDT_UDP_RCVBUF, &rcv_buf, sizeof(int));

// Maximum window size (packets) (default 25600) *** change with care!
    int fc = 16;
    udt_setsockopt(*usock, 0, UDT_UDT_FC, &fc, sizeof(int));


// Reuse an existing address or create a new one (default true)
    bool reuse = 1;
    udt_setsockopt(*usock, 0, UDT_UDT_REUSEADDR, &reuse, sizeof(bool));

// Rendezvous connection setup (default false)
    udt_setsockopt(*usock, 0, UDT_UDT_RENDEZVOUS, &rendezvous, sizeof(bool));

// Bind socket to port
    int err;
    udt_bind(*usock, res->ai_addr, res->ai_addrlen);
/*
   if((err= udt_bind(*usock, res->ai_addr, res->ai_addrlen)) != UDT_SUCCESS){
      fprintf(stderr, "UDT bind: [%s]\n",udt_getlasterror_desc());
      return -1;
   }
*/
    freeaddrinfo(res);
    return 0;
}

// rendezvous = false is default
// port = 0 is default
int createTCPSocket(SYSSOCKET* ssock, int port, bool rendezvous)
{
    struct addrinfo hints;
    struct addrinfo* res;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char service[16];
    sprintf(service, "%d", port);

    if (0 != getaddrinfo(NULL, service, &hints, &res)) {
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createTCPSocket", err,
                     "Illegal port number or port is busy");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createTCPSocket", err,
                     (char*) udt_getlasterror_desc());
        return -1;
    }

    *ssock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (bind(*ssock, res->ai_addr, res->ai_addrlen) != 0) {
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createTCPSocket", err, "Socket Bind error");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:createTCPSocket", err,
                     (char*) udt_getlasterror_desc());
        return -1;
    }

    freeaddrinfo(res);
    return 0;
}

// connect conflicts with system function
int c_connect(UDTSOCKET* usock, int port)
{
    struct addrinfo hints, * peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char buffer[16];
    sprintf(buffer, "%d", port);

    if (0 != getaddrinfo(g_Localhost, buffer, &hints, &peer)) {
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:c_connect", err, "Socket Connect error");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:c_connect", err, (char*) udt_getlasterror_desc());
        return -1;
    }

    udt_connect(*usock, peer->ai_addr, peer->ai_addrlen);

    freeaddrinfo(peer);
    return 0;
}

int tcp_connect(SYSSOCKET* ssock, int port)
{
    struct addrinfo hints, * peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char buffer[16];
    sprintf(buffer, "%d", port);

    if (0 != getaddrinfo(g_Localhost, buffer, &hints, &peer)) {
        int err = 999;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:tcp_connect", err, "Socket Connect error");
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:tcp_connect", err, (char*) udt_getlasterror_desc());
        return -1;
    }

    connect(*ssock, peer->ai_addr, peer->ai_addrlen);

    freeaddrinfo(peer);
    return 0;
}

//======================================================================================
// Test UDT (UDP/IP) Communication: data transfer from server to client

// The IDAM server uses a regular TCP socket to instance the server via XINETD
// TCP has performance issues in high bandwidth high latency (RTT) networks
// UDP has better performance but data packets may get lost - it is not reliable
// UDT is an application level protocol, based on UDP, that is reliable and has the performance of UDP
// The IDAM server acts as a UDT client and the IDAM client acts as the UDT server!
static int do_testudt(IDAM_PLUGIN_INTERFACE* idam_plugin_interface)
{
    // Start a mini server loop and create a separate communiation channel with the client bye-passing the TCP socket

    int i;
    int client;                // listening socket id
    int false = 0;
    int err = 0;

// Create a UDT socket without specifying the port number

    if (createUDTSocket(&client, 0, false) < 0) { ;
        err = 9990;
        addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:udt", err, "Unable to create a UDT Socket");
        return err;
    }

// Connect to the IDAM client on a specific port
// Client and server sockets are connected

    c_connect(&client, g_Server_Port);

// Create data to send

    int32_t buffer[g_TotalNum];
    int32_t sum = 0;
    for (int i = 0; i < g_TotalNum; ++i) {
        buffer[i] = i;
        sum += buffer[i];
    }

// Send the data (*** NOT Architecture independent ***)

    struct timeval tm1, tm2;
    gettimeofday(&tm1, NULL);

    time_t ticks = time(NULL);
    char sendBuff[1025];
    snprintf(sendBuff, sizeof(sendBuff), "%.24s", ctime(&ticks));
    int tosend = strlen(sendBuff) + 1;
    int sent = udt_send(client, sendBuff, tosend, 0);

    tosend = g_TotalNum * sizeof(int32_t);
    while (tosend > 0) {
        int sent = udt_send(client, (char*) buffer + g_TotalNum * sizeof(int32_t) - tosend, tosend, 0);
        if (sent < 0) {
            err = 9990;
            addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:udt", err, "Unable to Send Data");
            addIdamError(&idamerrorstack, CODEERRORTYPE, "testplugin:udt", err, (char*) udt_getlasterror_desc());
            break;
        }
        tosend -= sent;
    }

    gettimeofday(&tm2, NULL);
    int dsecs = (int) (tm2.tv_sec - tm1.tv_sec);
    int dmics = (int) (tm2.tv_usec - tm1.tv_usec);

    buffer[0] = dsecs;
    buffer[1] = dmics;
    tosend = 2 * sizeof(int32_t);
    sent = udt_send(client, (char*) buffer, tosend, 0);

    ticks = time(NULL);
    snprintf(sendBuff, sizeof(sendBuff), "%.24s", ctime(&ticks));
    tosend = (int) strlen(sendBuff) + 1;
    sent = udt_send(client, sendBuff, tosend, 0);

// Close the connection

    udt_close(client);

// Return IDAM status

    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = TYPE_INT;

    int* status = (int*) malloc(sizeof(int));
    status[0] = 0;
    data_block->data = (char*) status;

    strcpy(data_block->data_desc, "testplugins:udt status");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return err;
}

#endif // TESTUDT
