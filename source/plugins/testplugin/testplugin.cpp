#include "testplugin.h"

#include <cstddef>
#include <cstdlib>
#include <vector>

#include "clientserver/initStructs.h"
#include "clientserver/makeRequestBlock.h"
#include "clientserver/printStructs.h"
#include "clientserver/stringUtils.h"
#include "include/uda/uda_plugin_base.hpp"
#include <serialisation/capnp_serialisation.h>

#include <boost/filesystem.hpp>
#include <fmt/format.h>

#include "teststructs.h"

#ifdef PUTDATAENABLED
#  include <structures/accessors.h>
#endif // PUTDATAENABLED

#ifdef TESTUDT
#  include "udtc.h"
#  include <netdb.h>
#  include <pthread.h>

typedef int bool;
int g_IP_Version = AF_INET;      // IPv4 family of addresses
int g_Socket_Type = SOCK_STREAM; // use reliable transport layer protocol with Aknowledgements (default TCP)

char g_Localhost[] = "192.168.16.88"; //"192.168.16.125"; //"127.0.0.1"; // "192.168.16.88";    // Client IP address
                                      //(*** passed as a parameter)
int g_Server_Port = 50000;            // port number (*** passed as a parameter)

int g_TotalNum = 1000000; // Test data

int tcp_connect(SYSSOCKET* ssock, int port);
int c_connect(UDTSOCKET* usock, int port);
int createUDTSocket(int* usock, int port, int rendezvous);
int createTCPSocket(SYSSOCKET* ssock, int port, bool rendezvous);
#endif

class TestPlugin : public UDAPluginBase
{
  public:
    TestPlugin();
    int test0(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test2(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test4(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test5(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test6(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test7(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test8(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test9(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test9A(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test10(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test11(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test12(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test13(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test14(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test15(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test16(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test18(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test19(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test20(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test21(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test22(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test23(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test24(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test25(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test26(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test27(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test28(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test30(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test31(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test32(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test33(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test34(UDA_PLUGIN_INTERFACE* plugin_interface);
#ifdef PUTDATAENABLED
    int test40(UDA_PLUGIN_INTERFACE* plugin_interface);
#endif // PUTDATAENABLED
    int test50(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test60(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test61(UDA_PLUGIN_INTERFACE* plugin_interface);
    int test62(UDA_PLUGIN_INTERFACE* plugin_interface);
    int plugin(UDA_PLUGIN_INTERFACE* plugin_interface);
    int errortest(UDA_PLUGIN_INTERFACE* plugin_interface);
    int scalartest(UDA_PLUGIN_INTERFACE* plugin_interface);
    int array1dtest(UDA_PLUGIN_INTERFACE* plugin_interface);
    int call_plugin_test(UDA_PLUGIN_INTERFACE* plugin_interface);
    int call_plugin_test_index(UDA_PLUGIN_INTERFACE* plugin_interface);
    int call_plugin_test_slice(UDA_PLUGIN_INTERFACE* plugin_interface);
    int call_plugin_test_stride(UDA_PLUGIN_INTERFACE* plugin_interface);
    int emptytest(UDA_PLUGIN_INTERFACE* plugin_interface);
#ifdef CAPNP_ENABLED
    int capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface);
    int nested_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface);
    int long_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface);
    int large_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface);
#endif // CAPNP_ENABLED
#ifdef TESTUDT
    int testudt(UDA_PLUGIN_INTERFACE* plugin_interface);
#endif // TESTUDT

    void init(UDA_PLUGIN_INTERFACE* plugin_interface) override {}
    void reset() override {}
};

TestPlugin::TestPlugin()
    : UDAPluginBase("TESTPLUGIN", 1, "test0",
                    boost::filesystem::path(__FILE__).parent_path().append("help.txt").string())
{
    register_method("test0", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test0));
    register_method("test2", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test2));
    register_method("test4", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test4));
    register_method("test5", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test5));
    register_method("test6", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test6));
    register_method("test7", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test7));
    register_method("test8", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test8));
    register_method("test9", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test9));
    register_method("test9A", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test9A));
    register_method("test10", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test10));
    register_method("test11", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test11));
    register_method("test12", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test12));
    register_method("test13", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test13));
    register_method("test14", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test14));
    register_method("test15", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test15));
    register_method("test16", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test16));
    register_method("test18", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test18));
    register_method("test19", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test19));
    register_method("test20", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test20));
    register_method("test21", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test21));
    register_method("test22", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test22));
    register_method("test23", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test23));
    register_method("test24", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test24));
    register_method("test25", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test25));
    register_method("test26", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test26));
    register_method("test27", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test27));
    register_method("test28", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test28));
    register_method("test30", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test30));
    register_method("test31", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test31));
    register_method("test32", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test32));
    register_method("test33", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test33));
    register_method("test34", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test34));
#ifdef PUTDATAENABLED
    register_method("test40", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test40));
#endif // PUTDATAENABLED
    register_method("test50", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test50));
    register_method("test60", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test60));
    register_method("test61", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test61));
    register_method("test62", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test62));
    register_method("plugin", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::plugin));
    register_method("errortest", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::errortest));
    register_method("scalartest", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::scalartest));
    register_method("array1dtest", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::array1dtest));
    register_method("call_plugin_test", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::call_plugin_test));
    register_method("call_plugin_test_index",
                    static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::call_plugin_test_index));
    register_method("call_plugin_test_slice",
                    static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::call_plugin_test_slice));
    register_method("call_plugin_test_stride",
                    static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::call_plugin_test_stride));
    register_method("emptytest", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::emptytest));
#ifdef CAPNP_ENABLED
    register_method("capnp_test", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::capnp_test));
    register_method("nested_capnp_test",
                    static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::nested_capnp_test));
    register_method("long_capnp_test", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::long_capnp_test));
    register_method("large_capnp_test", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::large_capnp_test));
#endif // CAPNP_ENABLED
#ifdef TESTUDT
    register_method("testudt", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::testudt));
#endif // TESTUDT
}

extern int testPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static TestPlugin plugin = {};
    return plugin.call(plugin_interface);
}

void testError1()
{
    // Test of Error Management within Plugins
    int err = 9991;
    addIdamError(UDA_CODE_ERROR_TYPE, "testplugin", err, "Test #1 of Error State Management");
}

void testError2()
{
    // Test of Error Management within Plugins
    int err = 9992;
    addIdamError(UDA_CODE_ERROR_TYPE, "testplugin", err, "Test #2 of Error State Management");
}

int TestPlugin::test0(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    REQUEST_DATA* request = plugin_interface->request_data;

    const char* help = "Hello World!";

    initDataBlock(data_block);

    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    if (STR_IEQUALS(request->function, "test0")) {
        data_block->data_type = UDA_TYPE_CHAR;
        strcpy(data_block->data_desc, "testplugins: test0 = single string as a char array");
    } else {
        data_block->data_type = UDA_TYPE_STRING;
        strcpy(data_block->data_desc, "testplugins: test1 = single string");
    }

    data_block->data = strdup(help);

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = (int)strlen(help) + 1;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->data_n = data_block->dims[0].dim_n;

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

int TestPlugin::test2(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    REQUEST_DATA* request = plugin_interface->request_data;

    // An array of strings can be formed in two distinct ways.
    // 1> A fixed block of contiguous memory with each string beginning at a well defined regular location - as if each
    //    string were the same length.
    // 2> An array of string pointers: each string has its own length. Memory is not contiguous. This is the normal
    //    representation of string arrays.

    // To pass back the data as a block of chars/bytes or as type STRING, model 1 must be adopted - its how the
    // middleware operates. By labeling the type as STRING, we can convert the data within the client to the correct
    // type

    // create original data using model 2

    int sCount = 3;
    char** sarr = (char**)malloc(sCount * sizeof(char*));

    {
        for (int i = 0; i < sCount; i++) {
            sarr[i] = (char*)malloc(30 * sizeof(char));
        }
    }

    strcpy(sarr[0], "Hello World!");
    strcpy(sarr[1], "Qwerty keyboard");
    strcpy(sarr[2], "MAST Upgrade");

    // Maximum size of any individual string

    int sMax = 0;
    {
        for (int i = 0; i < sCount; i++) {
            int sLen;
            if ((sLen = (int)strlen(sarr[i]) + 1) > sMax) {
                sMax = sLen;
            }
        }
    }

    // Create a block of contigous memory and assign all bytes to nullptr character

    char* p = (char*)malloc(sMax * sCount * sizeof(char));
    memset(p, '\0', sMax * sCount);

    // Copy string data into the block positioned at regular intervals

    {
        for (int i = 0; i < sCount; i++) {
            strcpy(&p[i * sMax], sarr[i]);
        }
    }

    // Free original data

    {
        for (int i = 0; i < sCount; i++) {
            free(sarr[i]);
        }
    }
    free(sarr);

    initDataBlock(data_block);

    data_block->rank = 2;
    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    if (STR_IEQUALS(request->function, "test2")) {
        data_block->data_type = UDA_TYPE_CHAR;
        strcpy(data_block->data_desc, "testplugins: test2 = 2D array of chars");
    } else {
        data_block->data_type = UDA_TYPE_STRING;
        strcpy(data_block->data_desc, "testplugins: test3 = array of strings");
    }

    data_block->data = p;

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = sMax;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    data_block->dims[1].data_type = UDA_TYPE_UNSIGNED_INT;
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

int TestPlugin::test4(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test4 {
        char value[56];
    } TEST4;

    TEST4* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST4");
    strcpy(usertype.source, "Test #4");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST4); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: value[56]");

    field.pointer = 0;
    field.count = 56;
    field.rank = 1;

    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST4*)malloc(sizeof(TEST4)); // Structured Data Must be a heap variable
    strcpy(data->value, "012345678901234567890");
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST4), "TEST4");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #4");
    strcpy(data_block->data_label, "Values: 012345678901234567890");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST4", 0);

    return 0;
}

int TestPlugin::test5(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test5 {
        char value[3][56];
    } TEST5;

    TEST5* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST5");
    strcpy(usertype.source, "Test #5");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST5); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: value[3][56]");

    field.pointer = 0;
    field.count = 3 * 56;
    field.rank = 2;

    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 56;
    field.shape[1] = 3;

    field.size = field.count * sizeof(char);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST5*)malloc(sizeof(TEST5)); // Structured Data Must be a heap variable
    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST5), "TEST5");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #5");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST5", 0);

    return 0;
}

int TestPlugin::test6(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test6 {
        char* value;
    } TEST6;

    TEST6* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST6");
    strcpy(usertype.source, "Test #6");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST6); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "string structure element: *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr;

    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST6*)malloc(sizeof(TEST6)); // Structured Data Must be a heap variable
    data->value = (char*)malloc(56 * sizeof(char));
    strcpy(data->value, "PI=3.1415927");
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST6), "TEST6");
    addMalloc(plugin_interface->logmalloclist, (void*)data->value, 1, 56 * sizeof(char), "char");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #6");
    strcpy(data_block->data_label, "Value: PI=3.1415927");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST6", 0);

    return 0;
}

int TestPlugin::test7(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test7 {
        char* value[3]; // 3 strings of arbitrary length
    } TEST7;

    TEST7* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST7");
    strcpy(usertype.source, "Test #7");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST7); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING *"); // Array of String pointers
    strcpy(field.desc, "string structure element: *value[3]");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;

    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 3;

    field.size = field.count * sizeof(char*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST7*)malloc(sizeof(TEST7)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST7), "TEST7");

    data->value[0] = (char*)malloc(56 * sizeof(char));
    data->value[1] = (char*)malloc(55 * sizeof(char));
    data->value[2] = (char*)malloc(54 * sizeof(char));

    addMalloc(plugin_interface->logmalloclist, (void*)data->value[0], 56, sizeof(char), "char");
    addMalloc(plugin_interface->logmalloclist, (void*)data->value[1], 55, sizeof(char), "char");
    addMalloc(plugin_interface->logmalloclist, (void*)data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #7");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST7", 0);

    return 0;
}

int TestPlugin::test8(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test8 {
        char** value; // arbitrary number of strings of arbitrary length
    } TEST8;

    TEST8* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST8");
    strcpy(usertype.source, "Test #8");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST8); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING *"); // Array of String pointers
    strcpy(field.desc, "string structure element: **value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr;

    field.size = field.count * sizeof(char**);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST8*)malloc(sizeof(TEST8)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST8), "TEST8");

    data->value = (char**)malloc(3 * sizeof(char*));
    addMalloc(plugin_interface->logmalloclist, (void*)data->value, 3, sizeof(char*), "STRING *");

    data->value[0] = (char*)malloc(56 * sizeof(char));
    data->value[1] = (char*)malloc(55 * sizeof(char));
    data->value[2] = (char*)malloc(54 * sizeof(char));

    addMalloc(plugin_interface->logmalloclist, (void*)data->value[0], 56, sizeof(char), "char");
    addMalloc(plugin_interface->logmalloclist, (void*)data->value[1], 55, sizeof(char), "char");
    addMalloc(plugin_interface->logmalloclist, (void*)data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #8");
    strcpy(data_block->data_label, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST8", 0);

    return 0;
}

int TestPlugin::test9(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    // Create Data

    TEST9* data = (TEST9*)malloc(4 * sizeof(TEST9)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 4, sizeof(TEST9), "TEST9");

    {
        for (int i = 0; i < 4; i++) {
            strcpy(data[i].v1, "123212321232123212321");
            strcpy(data[i].v2[0], "012345678901234567890");
            strcpy(data[i].v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v2[2], "MAST TOKAMAK");

            data[i].v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v3, "PI=3.1415927");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v3, 1, 56 * sizeof(char), "char");

            data[i].v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v4[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v4[0], "012345678901234567890");
            strcpy(data[i].v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v4[2], "MAST TOKAMAK");

            data[i].v5 = (char**)malloc(3 * sizeof(char*));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5, 3, sizeof(char*), "STRING *");
            data[i].v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v5[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v5[0], "012345678901234567890");
            strcpy(data[i].v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v5[2], "MAST TOKAMAK");
        }
    }

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data_n = 4;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #9");
    strcpy(data_block->data_label, "Multiple test results");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST9", 0);

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));

    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

int TestPlugin::test9A(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    // Create Data

    TEST9A* data = (TEST9A*)malloc(4 * sizeof(TEST9A)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 4, sizeof(TEST9A), "TEST9A");

    {
        for (int i = 0; i < 4; i++) {
            strcpy(data[i].v1, "123212321232123212321");
            strcpy(data[i].v2[0], "012345678901234567890");
            strcpy(data[i].v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v2[2], "MAST TOKAMAK");

            data[i].v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v3, "PI=3.1415927");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v3, 1, 56 * sizeof(char), "char");

            data[i].v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v4[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v4[0], "012345678901234567890");
            strcpy(data[i].v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v4[2], "MAST TOKAMAK");

            data[i].v5 = (char**)malloc(3 * sizeof(char*));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5, 3, sizeof(char*), "STRING *");
            data[i].v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v5[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v5[0], "012345678901234567890");
            strcpy(data[i].v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v5[2], "MAST TOKAMAK");

            strcpy(data[i].v6.v1, "123212321232123212321");
            strcpy(data[i].v6.v2[0], "012345678901234567890");
            strcpy(data[i].v6.v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v2[2], "MAST TOKAMAK");

            data[i].v6.v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v6.v3, "PI=3.1415927");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v3, 1, 56 * sizeof(char), "char");

            data[i].v6.v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v6.v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v6.v4[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v4[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v4[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v6.v4[0], "012345678901234567890");
            strcpy(data[i].v6.v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v4[2], "MAST TOKAMAK");

            data[i].v6.v5 = (char**)malloc(3 * sizeof(char*));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v5, 3, sizeof(char*), "STRING *");
            data[i].v6.v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v6.v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v6.v5[2] = (char*)malloc(54 * sizeof(char));
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v5[0], 56, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v5[1], 55, sizeof(char), "char");
            addMalloc(plugin_interface->logmalloclist, (void*)data[i].v6.v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v6.v5[0], "012345678901234567890");
            strcpy(data[i].v6.v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v5[2], "MAST TOKAMAK");
        }
    }

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data_n = 4;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #9A");
    strcpy(data_block->data_label, "Multiple test results");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST9A", 0);

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

int TestPlugin::test10(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    // Create Data

    int* data = (int*)malloc(sizeof(int));
    data[0] = 7;

    // Pass Data

    data_block->data_type = UDA_TYPE_INT;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #10");
    strcpy(data_block->data_label, "Value: 7");
    strcpy(data_block->data_units, "");

    return 0;
}

int TestPlugin::test11(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test11 {
        int value;
    } TEST11;

    TEST11* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST11");
    strcpy(usertype.source, "Test #11");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST11); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST11*)malloc(sizeof(TEST11)); // Structured Data Must be a heap variable
    data[0].value = 11;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST11), "TEST11");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #11");
    strcpy(data_block->data_label, "Value: 11");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST11", 0);

    return 0;
}

int TestPlugin::test12(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test12 {
        int value[3];
    } TEST12;

    TEST12* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST12");
    strcpy(usertype.source, "Test #12");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST12); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST12*)malloc(sizeof(TEST12)); // Structured Data Must be a heap variable
    data[0].value[0] = 10;
    data[0].value[1] = 11;
    data[0].value[2] = 12;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST12), "TEST12");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #12");
    strcpy(data_block->data_label, "Values: 10,11,12");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST12", 0);

    return 0;
}

int TestPlugin::test13(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test13 {
        int value[2][3];
    } TEST13;

    TEST13* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST13");
    strcpy(usertype.source, "Test #13");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST13); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 6;
    field.rank = 2;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 2;
    field.shape[1] = 3; // Reversed ... Fortran/IDL like

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST13*)malloc(sizeof(TEST13)); // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST13), "TEST13");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #13");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST13", 0);

    return 0;
}

int TestPlugin::test14(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test14 {
        int* value;
    } TEST14;

    TEST14* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST14");
    strcpy(usertype.source, "Test #14");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST14); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: int *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(int*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST14*)malloc(sizeof(TEST14)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST14), "TEST14");

    data[0].value = (int*)malloc(sizeof(int));
    addMalloc(plugin_interface->logmalloclist, (void*)data[0].value, 1, sizeof(int), "int");

    data[0].value[0] = 14;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #14");
    strcpy(data_block->data_label, "int *value: 14");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST14", 0);

    return 0;
}

int TestPlugin::test15(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test15 {
        int* value;
    } TEST15;

    TEST15* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST15");
    strcpy(usertype.source, "Test #15");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST15); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(int*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST15*)malloc(sizeof(TEST15)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST15), "TEST15");

    data[0].value = (int*)malloc(3 * sizeof(int));
    int shape[] = {3};
    addMalloc2(plugin_interface->logmalloclist, (void*)data[0].value, 3, sizeof(int), "int", 1, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #15");
    strcpy(data_block->data_label, "Values: 13,14,15");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST15", 0);

    return 0;
}

int TestPlugin::test16(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test16 {
        int* value;
    } TEST16;

    TEST16* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST16");
    strcpy(usertype.source, "Test #16");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST16); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(int*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST16*)malloc(sizeof(TEST16)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST16), "TEST16");

    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 2;
    shape[1] = 3;
    int count = shape[0] * shape[1];
    int rank = 2;
    data[0].value = (int*)malloc(count * sizeof(int));
    addMalloc2(plugin_interface->logmalloclist, (void*)data[0].value, count, sizeof(int), "int", rank, shape);

    data[0].value[0] = 0;
    data[0].value[1] = 1;
    data[0].value[2] = 2;
    data[0].value[3] = 10;
    data[0].value[4] = 11;
    data[0].value[5] = 12;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #16");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST16", 0);

    return 0;
}

int TestPlugin::test18(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test18 {
        int value;
    } TEST18;

    TEST18* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST18");
    strcpy(usertype.source, "Test #18");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST18); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int"); // convert atomic type to a string label
    strcpy(field.desc, "single integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data_block->data_n = 100000;
    data = (TEST18*)malloc(data_block->data_n * sizeof(TEST18)); // Structured Data Must be a heap variable

    {
        for (int i = 0; i < data_block->data_n; i++) {
            data[i].value = i;
        }
    }
    addMalloc(plugin_interface->logmalloclist, (void*)data, data_block->data_n, sizeof(TEST18), "TEST18");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Array Integer Data Test #18");
    strcpy(data_block->data_label, "100000 Values: i 0, 100000");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST18", 0);

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

int TestPlugin::test19(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test19A {
        int value;
    } TEST19A;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST19A");
    strcpy(usertype.source, "Test #19");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST19A); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int");
    strcpy(field.desc, "integer structure element");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    typedef struct Test19 {
        int value;
        TEST19A vals[7];
    } TEST19;

    TEST19* data;

    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST19");
    strcpy(usertype.source, "Test #19");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST19); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_INT;
    strcpy(field.type, "int");
    strcpy(field.desc, "integer");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(int);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);

    strcpy(field.name, "vals");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST19A");
    strcpy(field.desc, "structure TEST19A");

    field.pointer = 0;
    field.count = 7;
    field.rank = 1;

    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 7;

    field.size = field.count * sizeof(TEST19A);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data_block->data_n = 3;
    data = (TEST19*)malloc(data_block->data_n * sizeof(TEST19)); // Structured Data Must be a heap variable

    {
        for (int i = 0; i < data_block->data_n; i++) {
            data[i].value = 3 + i;
            for (int j = 0; j < 7; j++) {
                data[i].vals[j].value = 10 * i + j;
            }
        }
    }
    addMalloc(plugin_interface->logmalloclist, (void*)data, data_block->data_n, sizeof(TEST19), "TEST19");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Test #19");
    strcpy(data_block->data_label, "Values: ");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST19", 0);

    data_block->dims = (DIMS*)malloc(data_block->rank * sizeof(DIMS));
    for (unsigned int i = 0; i < data_block->rank; i++) {
        initDimBlock(&data_block->dims[i]);
    }

    data_block->dims[0].data_type = UDA_TYPE_UNSIGNED_INT;
    data_block->dims[0].dim_n = data_block->data_n;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0.0;
    data_block->dims[0].diff = 1.0;
    data_block->dims[0].method = 0;

    return 0;
}

int TestPlugin::test20(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    // Create Data

    short* data = (short*)malloc(sizeof(short));
    data[0] = 7;

    // Pass Data

    data_block->data_type = UDA_TYPE_SHORT;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #20");
    strcpy(data_block->data_label, "Short Value: 7");
    strcpy(data_block->data_units, "");

    return 0;
}

int TestPlugin::test21(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test21 {
        short value;
    } TEST21;

    TEST21* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST21");
    strcpy(usertype.source, "Test #21");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST21); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "single short integer structure element: value");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST21*)malloc(sizeof(TEST21)); // Structured Data Must be a heap variable
    data[0].value = 21;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST21), "TEST21");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #21");
    strcpy(data_block->data_label, "Short Value: 21");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST21", 0);

    return 0;
}

int TestPlugin::test22(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test22 {
        short value[3];
    } TEST22;

    TEST22* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST22");
    strcpy(usertype.source, "Test #22");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST22); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "single short integer structure element: value");

    field.pointer = 0;
    field.count = 3;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST22*)malloc(sizeof(TEST22)); // Structured Data Must be a heap variable
    data[0].value[0] = 20;
    data[0].value[1] = 21;
    data[0].value[2] = 22;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST22), "TEST22");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #22");
    strcpy(data_block->data_label, "Short Array Values: 20,21,22");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST22", 0);

    return 0;
}

int TestPlugin::test23(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test23 {
        short value[2][3];
    } TEST23;

    TEST23* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST23");
    strcpy(usertype.source, "Test #23");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST23); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short integer array: value");

    field.pointer = 0;
    field.count = 6;
    field.rank = 2;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 3;
    field.shape[1] = 2;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST23*)malloc(sizeof(TEST23)); // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST23), "TEST23");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #23");
    strcpy(data_block->data_label, "Values: {0,1,2},{10,11,12}");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST23", 0);

    return 0;
}

int TestPlugin::test24(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test24 {
        short* value;
    } TEST24;

    TEST24* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST24");
    strcpy(usertype.source, "Test #24");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST24); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;

    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(short*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST24*)malloc(sizeof(TEST24)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST24), "TEST24");

    data[0].value = (short*)malloc(sizeof(short));
    addMalloc(plugin_interface->logmalloclist, (void*)data[0].value, 1, sizeof(short), "short");

    data[0].value[0] = 24;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #24");
    strcpy(data_block->data_label, "short *value: 14");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST24", 0);

    return 0;
}

int TestPlugin::test25(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test25 {
        short* value;
    } TEST25;

    TEST25* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST25");
    strcpy(usertype.source, "Test #25");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST25); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(short*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST25*)malloc(sizeof(TEST25)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST25), "TEST25");

    data[0].value = (short*)malloc(3 * sizeof(short));
    int shape[] = {3};
    addMalloc2(plugin_interface->logmalloclist, (void*)data[0].value, 3, sizeof(short), "short", 1, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #25");
    strcpy(data_block->data_label, "Short Values: 13,14,15");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST25", 0);

    return 0;
}

int TestPlugin::test26(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test26 {
        short* value;
    } TEST26;

    TEST26* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST26");
    strcpy(usertype.source, "Test #26");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST26); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(short*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data Structure

    data = (TEST26*)malloc(sizeof(TEST26)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST26), "TEST26");

    // Data is a compact Fortran like rank 2 array

    data[0].value = (short*)malloc(6 * sizeof(short));
    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 3;
    shape[1] = 2;
    addMalloc2(plugin_interface->logmalloclist, (void*)data[0].value, 6, sizeof(short), "short", 2, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    data[0].value[3] = 23;
    data[0].value[4] = 24;
    data[0].value[5] = 25;

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #26");
    strcpy(data_block->data_label, "Short Values: 13,14,15   23,24,25");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST26", 0);

    return 0;
}

int TestPlugin::test27(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test27 {
        short value[2][3][4];
    } TEST27;

    TEST27* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST27");
    strcpy(usertype.source, "Test #27");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST27); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short integer array: value");

    field.pointer = 0;
    field.count = 24;
    field.rank = 3;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = 4;
    field.shape[1] = 3;
    field.shape[2] = 2;

    field.size = field.count * sizeof(short);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST27*)malloc(sizeof(TEST27)); // Structured Data Must be a heap variable

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

    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST27), "TEST27");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #27");
    strcpy(data_block->data_label, "Values: {0,1,2,3},{10,11,12,13},...");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST27", 0);

    return 0;
}

int TestPlugin::test28(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test28 {
        short* value;
    } TEST28;

    TEST28* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST28");
    strcpy(usertype.source, "Test #28");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST28); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    strcpy(field.name, "value");
    field.atomictype = UDA_TYPE_SHORT;
    strcpy(field.type, "short"); // convert atomic type to a string label
    strcpy(field.desc, "short *value");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(short*);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data Structure

    data = (TEST28*)malloc(sizeof(TEST28)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST28), "TEST28");

    // Data is a compact Fortran like rank 3 array

    data[0].value = (short*)malloc(24 * sizeof(short));
    int* shape = (int*)malloc(3 * sizeof(int));
    shape[0] = 4;
    shape[1] = 3;
    shape[2] = 2;
    addMalloc2(plugin_interface->logmalloclist, (void*)data[0].value, 24, sizeof(short), "short", 3, shape);

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

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #28");
    strcpy(data_block->data_label, "Short Values: 13,14,15   23,24,25");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST28", 0);

    return 0;
}

int TestPlugin::test30(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test30 {
        double R;
        double Z;
    } TEST30;

    TEST30* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST30");
    strcpy(usertype.source, "Test #30");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST30); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    defineField(&field, "R", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "Z", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data = (TEST30*)malloc(sizeof(TEST30)); // Structured Data Must be a heap variable
    data[0].R = 1.0;
    data[0].Z = 2.0;
    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST30), "TEST30");

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #30");
    strcpy(data_block->data_label, "Double Values: (1, 2)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST30", 0);

    return 0;
}

int TestPlugin::test31(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test30 {
        double R;
        double Z;
    } TEST30;

    TEST30* data;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST30");
    strcpy(usertype.source, "Test #31");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST30); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    defineField(&field, "R", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "Z", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field);

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    int count = 100;
    data = (TEST30*)malloc(count * sizeof(TEST30)); // Structured Data Must be a heap variable

    offset = 0;

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 20; j++) {
            data[offset].R = (double)offset;
            data[offset].Z = 10.0 * (double)offset;
            offset++;
        }
    }

    int rank = 2;
    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    addMalloc2(plugin_interface->logmalloclist, (void*)data, count, sizeof(TEST30), "TEST30", rank, shape);

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #30");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST30", 0);

    return 0;
}

int TestPlugin::test32(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test32A {
        double R;
        double Z;
    } TEST32A;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST32A");
    strcpy(usertype.source, "Test #32");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST32A); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    defineField(&field, "R", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);
    defineField(&field, "Z", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    typedef struct Test32 {
        int count;
        TEST32A coords[100];
    } TEST32;

    TEST32* data;

    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST32");
    strcpy(usertype.source, "Test #32");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST32); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    defineField(&field, "count", "int structure element", &offset, SCALARINT);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);

    strcpy(field.name, "coords");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST32A");
    strcpy(field.desc, "structure TEST32A");

    field.pointer = 0;
    field.count = 100;
    field.rank = 1;

    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;

    field.size = field.count * sizeof(TEST32A);
    field.offset = offsetof(TEST32, coords);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);

    addCompoundField(&usertype, field); // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data_block->data_n = 1;
    data = (TEST32*)malloc(data_block->data_n * sizeof(TEST32)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, data_block->data_n, sizeof(TEST32), "TEST32");

    data[0].count = field.count;

    for (int i = 0; i < field.count; i++) {
        data[0].coords[i].R = 1.0 * i;
        data[0].coords[i].Z = 10.0 * i;
    }

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #32");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST32", 0);

    return 0;
}

int TestPlugin::test33(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test33A {
        double R;
        double Z;
    } TEST33A;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST33A");
    strcpy(usertype.source, "Test #33");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST33A); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    defineField(&field, "R", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);
    defineField(&field, "Z", "double structure element", &offset, SCALARDOUBLE);

    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    typedef struct Test33 {
        int count;
        TEST33A* coords;
    } TEST33;

    TEST33* data;

    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST33");
    strcpy(usertype.source, "Test #33");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST33); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    defineField(&field, "count", "int structure element", &offset, SCALARINT);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);

    strcpy(field.name, "coords");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST33A");
    strcpy(field.desc, "structure TEST33A");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(TEST33A*);
    field.offset = offsetof(TEST33, coords);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);

    addCompoundField(&usertype, field); // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data_block->data_n = 1;
    data = (TEST33*)malloc(data_block->data_n * sizeof(TEST33)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, data_block->data_n, sizeof(TEST33), "TEST33");

    data->count = 100;
    data->coords = (TEST33A*)malloc(data->count * sizeof(TEST33A));

    int rank = 2;
    auto shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    addMalloc2(plugin_interface->logmalloclist, (void*)data->coords, data->count, sizeof(TEST33A), "TEST33A", rank,
               shape);

    for (int i = 0; i < data->count; i++) {
        data->coords[i].R = 1.0 * i;
        data->coords[i].Z = 10.0 * i;
    }

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #33");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST33", 0);

    return 0;
}

int TestPlugin::test34(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    typedef struct Test33A {
        unsigned char* R;
        unsigned char* Z;
    } TEST33A;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST33A");
    strcpy(usertype.source, "Test #33");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST33A); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;

    initCompoundField(&field);
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    defineField(&field, "R", "unsigned char structure element", &offset, ARRAYUCHAR);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    defineField(&field, "Z", "unsigned char structure element", &offset, ARRAYUCHAR);

    addCompoundField(&usertype, field); // Single Structure element

    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;
    addUserDefinedType(userdefinedtypelist, usertype);

    typedef struct Test33 {
        int count;
        TEST33A* coords;
    } TEST33;

    TEST33* data;

    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST33");
    strcpy(usertype.source, "Test #33");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST33); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    defineField(&field, "count", "int structure element", &offset, SCALARINT);

    addCompoundField(&usertype, field); // Single Structure element

    initCompoundField(&field);

    strcpy(field.name, "coords");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST33A");
    strcpy(field.desc, "structure TEST33A");

    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;

    field.size = field.count * sizeof(TEST33A*);
    field.offset = (int)offsetof(TEST33, coords);
    field.offpad = (int)padding((size_t)offset, field.type);
    field.alignment = getalignmentof(field.type);

    addCompoundField(&usertype, field); // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    data_block->data_n = 1;
    data = (TEST33*)malloc(data_block->data_n * sizeof(TEST33)); // Structured Data Must be a heap variable
    addMalloc(plugin_interface->logmalloclist, (void*)data, data_block->data_n, sizeof(TEST33), "TEST33");

    data->count = 100;
    data->coords = (TEST33A*)malloc(data->count * sizeof(TEST33A));

    int rank = 2;
    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    addMalloc2(plugin_interface->logmalloclist, (void*)data->coords, data->count, sizeof(TEST33A), "TEST33A", rank,
               shape);

    for (int i = 0; i < data->count; i++) {
        data->coords[i].R = (unsigned char*)malloc(10 * sizeof(unsigned char));
        data->coords[i].Z = (unsigned char*)malloc(10 * sizeof(unsigned char));

        addMalloc(plugin_interface->logmalloclist, (void*)data->coords[i].R, 10, sizeof(unsigned char),
                  "unsigned char *");
        addMalloc(plugin_interface->logmalloclist, (void*)data->coords[i].Z, 10, sizeof(unsigned char),
                  "unsigned char *");

        for (int j = 0; j < 10; ++j) {
            data->coords[i].R[j] = (unsigned char)(1 * i);
            data->coords[i].Z[j] = (unsigned char)(10 * i);
        }

        //        data->coords[i].R = 1 * i;
        //        data->coords[i].Z = 10 * i;
    }

    // Pass Data

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Structure Data Test #33");
    strcpy(data_block->data_label, "Double Values [5, 20] : (1*, 10*)");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST33", 0);

    return 0;
}

#ifdef PUTDATAENABLED
//======================================================================================
// Receiving Put Data Blocks - structured data as arguments to a plugin
// Echo passed data back as an array of structures
// The library won't build if the server version is not OK for this functionality
// If the client cannot pass putdata blocks then no data will appear here to process.
int TestPlugin::test40(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test40 {
        unsigned int dataCount;
        void* data;
    } TEST40;

    typedef struct Test41 {
        int count;
        TEST40* blocks;
    } TEST41;

    int err = 0;

    DATA_BLOCKX* data_block = plugin_interface->data_block;
    REQUEST_BLOCK* request_block = plugin_interface->request_block;

    USERDEFINEDTYPE usertype;
    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST40");
    strcpy(usertype.source, "Test #40");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST40); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    initCompoundField(&field);

    // The number of data blocks is given by: request_block->putDataBlockList.blockCount
    // For this test, all blocks must be of the same type: request_block->putDataBlockList.putDataBlock[0].data_type;
    // Repeat call with changing types may cause client side issues!

    UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: %d\n", request_block->putDataBlockList.blockCount);

    if (request_block->putDataBlockList.blockCount == 0) {
        err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin", err, "No Put Data Blocks to process!");
        return err;
    }

    defineField(&field, "dataCount", "the number of data array elements", &offset, SCALARUINT);

    switch (request_block->putDataBlockList.putDataBlock[0].data_type) {
        case UDA_TYPE_INT:
            defineField(&field, "data", "the block data array", &offset, ARRAYINT);
            break;
        case UDA_TYPE_FLOAT:
            defineField(&field, "data", "the block data array", &offset, ARRAYFLOAT);
            break;
        case UDA_TYPE_DOUBLE:
            defineField(&field, "data", "the block data array", &offset, ARRAYDOUBLE);
            break;
    }

    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype); // New structure definition

    strcpy(usertype.name, "TEST41");
    strcpy(usertype.source, "Test #41");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST41); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;
    initCompoundField(&field);

    defineField(&field, "count", "the number of data blocks", &offset, SCALARUINT);

    strcpy(field.name, "blocks");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "TEST40");
    strcpy(field.desc, "Array of Block Data");

    field.pointer = 0;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr; // Needed when rank >= 1

    field.size = field.count * sizeof(TEST40);
    field.offset = newoffset(offset, field.type);
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    offset = field.offset + field.size; // Next Offset
    addCompoundField(&usertype, field); // Single Structure element

    addUserDefinedType(userdefinedtypelist, usertype);

    // Create Data

    TEST41* data = (TEST41*)malloc(sizeof(TEST41));
    TEST40* blocks = (TEST40*)malloc(request_block->putDataBlockList.blockCount * sizeof(TEST40));

    addMalloc(plugin_interface->logmalloclist, (void*)data, 1, sizeof(TEST41), "TEST41");
    addMalloc(plugin_interface->logmalloclist, (void*)blocks, request_block->putDataBlockList.blockCount,
              sizeof(TEST40), "TEST40");

    data->count = request_block->putDataBlockList.blockCount;

    for (int i = 0; i < request_block->putDataBlockList.blockCount; i++) {
        blocks[i].dataCount = request_block->putDataBlockList.putDataBlock[i].count;
        blocks[i].data = (void*)request_block->putDataBlockList.putDataBlock[i].data;

        UDA_LOG(UDA_LOG_DEBUG, "data type : %d\n", request_block->putDataBlockList.putDataBlock[0].data_type);
        UDA_LOG(UDA_LOG_DEBUG, "data count: %d\n", request_block->putDataBlockList.putDataBlock[0].count);

        // Data blocks already allocated and will be freed by a separate process so use addNonMalloc instead of
        // addMalloc

        switch (request_block->putDataBlockList.putDataBlock[0].data_type) {
            case UDA_TYPE_INT:
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(int), "int");
                break;
            case UDA_TYPE_FLOAT:
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(float), "float");
                float* f = (float*)blocks[i].data;
                break;
            case UDA_TYPE_DOUBLE:
                addNonMalloc(blocks[i].data, blocks[i].dataCount, sizeof(double), "double");
                break;
        }
    }

    // Pass Data back

    data_block->data_type = UDA_TYPE_COMPOUND;
    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data = (char*)data;

    strcpy(data_block->data_desc, "Test of receiving passed data blocks #40");
    strcpy(data_block->data_label, "Data type TEST41 with array of TEST40");
    strcpy(data_block->data_units, "");

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "TEST41", 0);

    return 0;
}
#endif // PUTDATAENABLED

//======================================================================================
// Passing parameters into plugins using placeholders and substitution
// Shot number passed via request_block->exp_number
// Placeholder values passed via request_block->tpass

int TestPlugin::test50(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    REQUEST_DATA* request = plugin_interface->request_data;

    UDA_LOG(UDA_LOG_DEBUG, "TESTPLUGIN test50\n");
    printRequestData(*request);

    // Return an array of strings with all passed parameters and substitutions

    std::string work = "test50 passed parameters and substitutions\n";
    work += fmt::format("Shot number: {}\n", request->exp_number);
    work += fmt::format("Pass number: {}\n", request->pass);
    work += fmt::format("substitution parameters: {}\n", request->tpass);
    work += fmt::format("Number of name-value pairs: {}\n", request->nameValueList.pairCount);
    for (int i = 0; i < request->nameValueList.pairCount; i++) {
        work += fmt::format("name: {}, value: {}\n", request->nameValueList.nameValue[i].name,
                            request->nameValueList.nameValue[i].value);
    }

    UDA_LOG(UDA_LOG_DEBUG, "test50: %s\n", work.c_str());

    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = work.size();
    data_block->data_type = UDA_TYPE_STRING;
    strcpy(data_block->data_desc, "testplugins:test50 = passing placeholders and substitution values to plugins");

    data_block->data = strdup(work.c_str());

    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return 0;
}

//======================================================================================
// Returning ENUM data: Values with labels
//
// Unless the ENUM structures are defined when the ENUM data are accessed, the integer type cannot be pre-specified.
// A void pointer type is used as the placeholder in the structure
// When the type is known the structure definition can be created with the correct type
// otherwise, an unsigned long long array can be returned as this will provide for all integer types. The application
// will have to deal with the type conversion.

typedef struct EnumMember60 {
    char name[MAXELEMENTNAME]; // The Enumeration member name
    long long value;           // The value of the member
} ENUMMEMBER60;

typedef struct EnumList60 {
    char name[MAXELEMENTNAME]; // The Enumeration name
    int type;                  // The original integer base type
    int count;                 // The number of members of this enumeration class
    ENUMMEMBER60* enummember;  // Array of enum members
    void* arraydata;           // Generalised data pointer for all integer type arrays
    int arraydata_rank;
    int arraydata_count;
    int* arraydata_shape;
} ENUMLIST60;

int TestPlugin::test60(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto enumlist = (ENUMLIST60*)malloc(sizeof(ENUMLIST60));
    strcpy(enumlist->name, "TEST60 ENUM of type unsigned short");
    enumlist->type = UDA_TYPE_UNSIGNED_SHORT;
    enumlist->count = 3;
    enumlist->enummember = (ENUMMEMBER60*)malloc(enumlist->count * sizeof(ENUMMEMBER60));
    strcpy(enumlist->enummember[0].name, "ENUM Value 1");
    strcpy(enumlist->enummember[1].name, "ENUM Value 2");
    strcpy(enumlist->enummember[2].name, "ENUM Value 3");
    enumlist->enummember[0].value = (long long)1;
    enumlist->enummember[1].value = (long long)2;
    enumlist->enummember[2].value = (long long)3;
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST60), "ENUMLIST60");
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER60),
              "ENUMMEMBER60");

    int count = 10;
    auto data = (unsigned short*)malloc(count * sizeof(unsigned short));
    data[0] = 3;
    data[1] = 2;
    data[2] = 1;
    data[3] = 2;
    data[4] = 3;
    data[5] = 2;
    data[6] = 1;
    data[7] = 2;
    data[8] = 3;
    data[9] = 2;
    enumlist->arraydata = (void*)data;
    enumlist->arraydata_rank = 1;
    enumlist->arraydata_count = count;
    enumlist->arraydata_shape = (int*)malloc(sizeof(int));
    enumlist->arraydata_shape[0] = count;

    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->arraydata, count, sizeof(unsigned short),
              "unsigned short");

    count = 1;
    int rank = 1;
    int shape[] = {1}; // Shape of the shape array!
    addMalloc2(plugin_interface->logmalloclist, (void*)enumlist->arraydata_shape, count, sizeof(int), "int", rank,
               shape);

    USERDEFINEDTYPE usertype;
    int offset;
    COMPOUNDFIELD field;
    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMMEMBER60");
    strcpy(usertype.source, "Test #60 ENUMMEMBER structure");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMMEMBER60); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM label");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(ENUMMEMBER60, name);
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "value", "The ENUM value", &offset, SCALARLONG64);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMLIST60");
    strcpy(usertype.source, "Test #60 ENUMLIST structure");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMLIST60); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM name");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(ENUMLIST60, name);
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "type", "The ENUM base integer atomic type", &offset, SCALARINT);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "count", "The number of ENUM values", &offset, SCALARINT);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "enummember");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "ENUMMEMBER60");
    strcpy(field.desc, "The ENUM list members: labels and value");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = sizeof(ENUMMEMBER60*);
    field.offset = offsetof(ENUMLIST60, enummember); // Different to newoffset
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    switch (enumlist->type) {
        case (UDA_TYPE_UNSIGNED_SHORT): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYUSHORT);
            break;
        }
        case (UDA_TYPE_SHORT): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYSHORT);
            break;
        }
        case (UDA_TYPE_UNSIGNED_INT): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYUINT);
            break;
        }
        case (UDA_TYPE_INT): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYINT);
            break;
        }
        case (UDA_TYPE_UNSIGNED_LONG64): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYULONG64);
            break;
        }
        case (UDA_TYPE_LONG64): {
            defineField(&field, "arraydata", "The array of values defined by the ENUM", &offset, ARRAYLONG64);
            break;
        }
    }
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "arraydata_rank", "The rank of arraydata", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "arraydata_count", "The count of arraydata", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "arraydata_shape", "The shape of arraydata", &offset, ARRAYINT);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = UDA_TYPE_COMPOUND;
    strcpy(data_block->data_desc, "testplugins:test60 = ENUM Values");

    data_block->data = (char*)enumlist;

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "ENUMLIST60", 0);

    return 0;
}

int TestPlugin::test61(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto enumlist = (ENUMLIST60*)malloc(sizeof(ENUMLIST60));
    strcpy(enumlist->name, "TEST61 ENUM of type unsigned long long");
    enumlist->type = UDA_TYPE_UNSIGNED_SHORT;
    enumlist->count = 3;
    enumlist->enummember = (ENUMMEMBER60*)malloc(enumlist->count * sizeof(ENUMMEMBER60));
    strcpy(enumlist->enummember[0].name, "ENUM Value 1");
    strcpy(enumlist->enummember[1].name, "ENUM Value 2");
    strcpy(enumlist->enummember[2].name, "ENUM Value 3");
    enumlist->enummember[0].value = (long long)1;
    enumlist->enummember[1].value = (long long)2;
    enumlist->enummember[2].value = (long long)3;
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST60), "ENUMLIST60");
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER60),
              "ENUMMEMBER60");

    int count = 10;
    auto data = (unsigned long long*)malloc(count * sizeof(unsigned long long));
    data[0] = 3;
    data[1] = 2;
    data[2] = 1;
    data[3] = 2;
    data[4] = 3;
    data[5] = 2;
    data[6] = 1;
    data[7] = 2;
    data[8] = 3;
    data[9] = 2;
    enumlist->arraydata = (void*)data;
    enumlist->arraydata_rank = 1;
    enumlist->arraydata_count = count;
    enumlist->arraydata_shape = (int*)malloc(sizeof(int));
    enumlist->arraydata_shape[0] = count;

    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->arraydata, count, sizeof(unsigned long long),
              "unsigned long long");

    count = 1;
    int rank = 1;
    int shape[] = {1}; // Shape of the shape array!
    addMalloc2(plugin_interface->logmalloclist, (void*)enumlist->arraydata_shape, count, sizeof(int), "int", rank,
               shape);

    USERDEFINEDTYPE usertype;
    int offset;
    COMPOUNDFIELD field;
    USERDEFINEDTYPELIST* userdefinedtypelist = plugin_interface->userdefinedtypelist;

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMMEMBER60");
    strcpy(usertype.source, "Test #61 ENUMMEMBER structure");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMMEMBER60); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM label");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(ENUMMEMBER60, name);
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "value", "The ENUM value", &offset, SCALARLONG64);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    initUserDefinedType(&usertype); // New structure definition
    strcpy(usertype.name, "ENUMLIST60");
    strcpy(usertype.source, "Test #61 ENUMLIST structure");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(ENUMLIST60); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;

    initCompoundField(&field);
    strcpy(field.name, "name");
    field.atomictype = UDA_TYPE_STRING;
    strcpy(field.type, "STRING"); // convert atomic type to a string label
    strcpy(field.desc, "The ENUM name");
    field.pointer = 0;
    field.count = MAXELEMENTNAME;
    field.rank = 1;
    field.shape = (int*)malloc(field.rank * sizeof(int)); // Needed when rank >= 1
    field.shape[0] = field.count;
    field.size = field.count * sizeof(char);
    field.offset = offsetof(ENUMLIST60, name);
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "type", "The ENUM base integer atomic type", &offset, SCALARINT);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "count", "The number of ENUM values", &offset, SCALARINT);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    strcpy(field.name, "enummember");
    field.atomictype = UDA_TYPE_UNKNOWN;
    strcpy(field.type, "ENUMMEMBER60");
    strcpy(field.desc, "The ENUM list members: labels and value");
    field.pointer = 1;
    field.count = 1;
    field.rank = 0;
    field.shape = nullptr;
    field.size = sizeof(ENUMMEMBER60*);
    field.offset = offsetof(ENUMLIST60, enummember); // Different to newoffset
    offset = field.offset + field.size;
    field.offpad = padding(offset, field.type);
    field.alignment = getalignmentof(field.type);
    addCompoundField(&usertype, field);

    initCompoundField(&field);
    defineField(&field, "arraydata", "Data with this enumerated type", &offset,
                ARRAYULONG64); // Data need to be converted to this type
    addCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "arraydata_rank", "The rank of arraydata", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "arraydata_count", "The count of arraydata", &offset, SCALARINT);
    addCompoundField(&usertype, field);
    initCompoundField(&field);
    defineField(&field, "arraydata_shape", "The shape of arraydata", &offset, ARRAYINT);
    addCompoundField(&usertype, field);

    addUserDefinedType(userdefinedtypelist, usertype);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = UDA_TYPE_COMPOUND;
    strcpy(data_block->data_desc, "testplugins:test61 = ENUM Values");

    data_block->data = (char*)enumlist;

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(userdefinedtypelist, "ENUMLIST60", 0);

    return 0;
}

int TestPlugin::test62(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto* enumlist = (ENUMLIST*)malloc(sizeof(ENUMLIST));
    strcpy(enumlist->name, "TEST62 ENUM of type unsigned long long");
    enumlist->type = UDA_TYPE_UNSIGNED_SHORT;
    enumlist->count = 3;
    enumlist->enummember = (ENUMMEMBER*)malloc(enumlist->count * sizeof(ENUMMEMBER));
    strcpy(enumlist->enummember[0].name, "ENUM Value 1");
    strcpy(enumlist->enummember[1].name, "ENUM Value 2");
    strcpy(enumlist->enummember[2].name, "ENUM Value 3");
    enumlist->enummember[0].value = (long long)1;
    enumlist->enummember[1].value = (long long)2;
    enumlist->enummember[2].value = (long long)3;
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist, 1, sizeof(ENUMLIST), "ENUMLIST");
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER),
              "ENUMMEMBER");

    int count = 10;
    auto data = (unsigned long long*)malloc(count * sizeof(unsigned long long));
    data[0] = 3;
    data[1] = 2;
    data[2] = 1;
    data[3] = 2;
    data[4] = 3;
    data[5] = 2;
    data[6] = 1;
    data[7] = 2;
    data[8] = 3;
    data[9] = 2;
    enumlist->enumarray = data;
    enumlist->enumarray_rank = 1;
    enumlist->enumarray_count = count;
    enumlist->enumarray_shape = (int*)malloc(sizeof(int));
    enumlist->enumarray_shape[0] = count;

    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->enumarray, count, sizeof(unsigned long long),
              "unsigned long long");

    // count = 1;
    // int rank = 1;
    // int shape[] = {1};        // Shape of the shape array!
    addMalloc(plugin_interface->logmalloclist, (void*)enumlist->enumarray_shape, 1, sizeof(int), "int");

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = UDA_TYPE_COMPOUND;
    strcpy(data_block->data_desc, "testplugins:test62 = ENUM Values");

    data_block->data = (char*)enumlist;

    data_block->opaque_type = UDA_OPAQUE_TYPE_STRUCTURES;
    data_block->opaque_count = 1;
    data_block->opaque_block = (void*)findUserDefinedType(plugin_interface->userdefinedtypelist, "ENUMLIST", 0);

    /*
    int id = findUserDefinedTypeId(userdefinedtypelist, "ENUMLIST");
    changeUserDefinedTypeElementProperty(userdefinedtypelist, id, "data", "name", (void *)"arraydata");
    int value = UDA_TYPE_UNSIGNED_LONG64;
    changeUserDefinedTypeElementProperty(userdefinedtypelist, id, "arraydata", "atomictype", (void *)&value);
    changeUserDefinedTypeElementProperty(userdefinedtypelist, id, "arraydata", "type", (void *)"unsigned long long");
    */
    return 0;
}

//======================================================================================
// Test direct calling of plugins from this plugin
// A plugin only has a single instance on a server. For multiple instances, multiple servers are needed.
// Plugins can maintain state so recursive calls (on the same server) must respect this.
// If the housekeeping action is requested, this must be also applied to all plugins called.
// A list must be maintained to register these plugin calls to manage housekeeping.
// Calls to plugins must also respect access policy and user authentication policy
int TestPlugin::plugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    REQUEST_DATA* request = plugin_interface->request_data;

    int err = 0;

    UDA_PLUGIN_INTERFACE next_plugin_interface;
    REQUEST_DATA next_request = {0};

    const PLUGINLIST* pluginList = plugin_interface->pluginList;

    if (pluginList == nullptr) {
        RAISE_PLUGIN_ERROR("No plugins available for this data request");
    }

    // Test specifics

    const char* signal = nullptr;
    const char* source = nullptr;

    FIND_REQUIRED_STRING_VALUE(request->nameValueList, signal);
    FIND_REQUIRED_STRING_VALUE(request->nameValueList, source);

    if (signal != nullptr || source != nullptr) { // Identify the plugin to test

        next_plugin_interface = *plugin_interface; // New plugin interface

        next_plugin_interface.request_data = &next_request;
        strcpy(next_request.api_delim, request->api_delim);

        strcpy(next_request.signal, signal);
        strcpy(next_request.source, source);

        makeRequestData(&next_request, *pluginList, plugin_interface->environment);

        for (int i = 0; i < pluginList->count; i++) {
            if (next_request.request == pluginList->plugin[i].request) {
                if (pluginList->plugin[i].idamPlugin != nullptr) {
                    err = pluginList->plugin[i].idamPlugin(&next_plugin_interface); // Call the data reader
                } else {
                    err = 999;
                    addIdamError(UDA_CODE_ERROR_TYPE, "No Data Access plugin available for this data request", err, "");
                }
                break;
            }
        }

        freeNameValueList(&next_request.nameValueList);
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
// To maintain consistency with existing legacy code, use a local structure with a global scope (plugin library only)
// A final necessary step is to concatenate this local structure with the server structure before returning.
// When testing the plugin, errors are doubled (tripled!) up as both stacks are the same.
int TestPlugin::errortest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    int err = 0;
    int test = 0;

    initUdaErrorStack();
    REQUEST_DATA* request = plugin_interface->request_data;

    FIND_REQUIRED_INT_VALUE(request->nameValueList, test);

    switch (test) {
        case 1:
            testError1();
            concatUdaError(&plugin_interface->error_stack);
            return err;
        case 2:
            testError2();
            concatUdaError(&plugin_interface->error_stack);
            return err;
        case 3: {
            const char* p = "crash!"; // force a server crash! (write to read-only memory)
            *const_cast<char*>(p) = '*';

            p = nullptr;
            free(const_cast<void*>(reinterpret_cast<const void*>(p)));

            int* p2 = nullptr;
            *p2 = 1;
        }
    }

    UDA_THROW_ERROR(9990 + test, "Test of Error State Management");
}

int TestPlugin::scalartest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);

    auto p = (int*)malloc(sizeof(int));
    *p = 10;
    data_block->data = (char*)p;
    data_block->data_n = 1;
    data_block->data_type = UDA_TYPE_INT;

    return 0;
}

int TestPlugin::array1dtest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;

    initDataBlock(data_block);

    constexpr int N = 100;
    auto p = (double*)malloc(N * sizeof(double));
    for (int i = 0; i < N; ++i) {
        p[i] = i;
    }
    data_block->data = (char*)p;
    data_block->data_n = 100;
    data_block->data_type = UDA_TYPE_DOUBLE;
    data_block->rank = 1;
    data_block->dims = (DIMS*)malloc(sizeof(DIMS));

    initDimBlock(data_block->dims);
    data_block->dims[0].dim_n = 100;
    data_block->dims[0].data_type = UDA_TYPE_INT;
    data_block->dims[0].compressed = 1;
    data_block->dims[0].dim0 = 0;
    data_block->dims[0].diff = 1;

    return 0;
}

int TestPlugin::emptytest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);
    return 0;
}

int TestPlugin::call_plugin_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return callPlugin(plugin_interface->pluginList, "TESTPLUGIN::array1dtest()", plugin_interface);
}

int TestPlugin::call_plugin_test_index(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return callPlugin(plugin_interface->pluginList, "TESTPLUGIN::array1dtest()[25]", plugin_interface);
}

int TestPlugin::call_plugin_test_slice(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return callPlugin(plugin_interface->pluginList, "TESTPLUGIN::array1dtest()[10:20]", plugin_interface);
}

int TestPlugin::call_plugin_test_stride(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return callPlugin(plugin_interface->pluginList, "TESTPLUGIN::array1dtest()[::2]", plugin_interface);
}

#ifdef CAPNP_ENABLED
int TestPlugin::capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto tree = uda_capnp_new_tree();
    auto root = uda_capnp_get_root(tree);
    uda_capnp_set_node_name(root, "root");
    uda_capnp_add_children(root, 3);

    auto child = uda_capnp_get_child(tree, root, 0);
    uda_capnp_set_node_name(child, "double_array");

    std::vector<double> vec(30);
    for (int i = 0; i < 30; ++i) {
        vec[i] = i / 10.0;
    }
    uda_capnp_add_array_f64(child, vec.data(), vec.size());

    child = uda_capnp_get_child(tree, root, 1);
    uda_capnp_set_node_name(child, "i32_array");

    std::vector<int32_t> ivec(100);
    for (int i = 0; i < 100; ++i) {
        ivec[i] = i;
    }
    uda_capnp_add_array_i32(child, ivec.data(), ivec.size());

    child = uda_capnp_get_child(tree, root, 2);
    uda_capnp_set_node_name(child, "i64_scalar");

    uda_capnp_add_i64(child, 999);

    auto buffer = uda_capnp_serialise(tree);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_n = static_cast<int>(buffer.size);
    data_block->data = buffer.data;
    data_block->dims = nullptr;
    data_block->data_type = UDA_TYPE_CAPNP;

    return 0;
}

int TestPlugin::nested_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto tree = uda_capnp_new_tree();
    auto root = uda_capnp_get_root(tree);
    uda_capnp_set_node_name(root, "a");
    uda_capnp_add_children(root, 2);

    auto child_b = uda_capnp_get_child(tree, root, 0);
    uda_capnp_set_node_name(child_b, "b");

    uda_capnp_add_children(child_b, 4);

    auto child_c = uda_capnp_get_child(tree, root, 1);
    uda_capnp_set_node_name(child_c, "c");

    std::vector<double> vec(30);
    for (int i = 0; i < 30; ++i) {
        vec[i] = i / 10.0;
    }
    uda_capnp_add_array_f64(child_c, vec.data(), vec.size());

    auto child_d = uda_capnp_get_child(tree, child_b, 0);
    uda_capnp_set_node_name(child_d, "d");

    auto child_e1 = uda_capnp_get_child(tree, child_b, 1);
    uda_capnp_set_node_name(child_e1, "e");

    auto child_e2 = uda_capnp_get_child(tree, child_b, 2);
    uda_capnp_set_node_name(child_e2, "e");

    auto child_e3 = uda_capnp_get_child(tree, child_b, 3);
    uda_capnp_set_node_name(child_e3, "e");

    std::vector<int32_t> ivec(100);
    for (int i = 0; i < 100; ++i) {
        ivec[i] = i;
    }
    uda_capnp_add_array_i32(child_d, ivec.data(), ivec.size());

    uda_capnp_add_i64(child_e1, 100);
    uda_capnp_add_i64(child_e2, 200);
    uda_capnp_add_i64(child_e3, 300);

    auto buffer = uda_capnp_serialise(tree);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_n = static_cast<int>(buffer.size);
    data_block->data = buffer.data;
    data_block->dims = nullptr;
    data_block->data_type = UDA_TYPE_CAPNP;

    return 0;
}

int TestPlugin::long_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto tree = uda_capnp_new_tree();
    auto root = uda_capnp_get_root(tree);
    uda_capnp_set_node_name(root, "root");

    constexpr int N = 1000;
    uda_capnp_add_children(root, N);

    for (int i = 0; i < N; ++i) {
        std::string name = "node/" + std::to_string(i);
        auto child = uda_capnp_get_child(tree, root, i);
        uda_capnp_set_node_name(child, name.c_str());

        uda_capnp_add_children(child, 2);

        auto shape = uda_capnp_get_child(tree, child, 0);
        uda_capnp_set_node_name(shape, "shape");
        std::vector<int> vals(0);
        uda_capnp_add_array_i32(shape, vals.data(), vals.size());

        auto data = uda_capnp_get_child(tree, child, 1);
        uda_capnp_set_node_name(data, "data");
        uda_capnp_add_i32(data, i + 1);
    }

    auto buffer = uda_capnp_serialise(tree);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_n = static_cast<int>(buffer.size);
    data_block->data = buffer.data;
    data_block->dims = nullptr;
    data_block->data_type = UDA_TYPE_CAPNP;

    return 0;
}

int TestPlugin::large_capnp_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto tree = uda_capnp_new_tree();
    auto root = uda_capnp_get_root(tree);
    uda_capnp_set_node_name(root, "root");
    uda_capnp_add_children(root, 1);

    auto child = uda_capnp_get_child(tree, root, 0);
    uda_capnp_set_node_name(child, "double_array");

    constexpr size_t N = 134'217'728; // 1GB worth of doubles
    std::vector<double> vec(N);
    for (size_t i = 0; i < N; ++i) {
        vec[i] = i / 10.0;
    }
    uda_capnp_add_array_f64(child, vec.data(), vec.size());

    auto buffer = uda_capnp_serialise(tree);

    DATA_BLOCK* data_block = plugin_interface->data_block;
    initDataBlock(data_block);

    data_block->data_n = static_cast<int>(buffer.size);
    data_block->data = buffer.data;
    data_block->dims = nullptr;
    data_block->data_type = UDA_TYPE_CAPNP;

    return 0;
}
#endif // CAPNP_ENABLED

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

    if (0 != getaddrinfo(nullptr, service, &hints, &res)) {
        int err = 9991;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createUDTSocket", err, "Illegal port number or port is busy");
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createUDTSocket", err, (char*)udt_getlasterror_desc());
        return -1;
    }

    *usock = udt_socket(res->ai_family, res->ai_socktype,
                        res->ai_protocol); // AF_INET, SOCK_STREAM, default protocol

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

    if (0 != getaddrinfo(nullptr, service, &hints, &res)) {
        int err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createTCPSocket", err, "Illegal port number or port is busy");
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createTCPSocket", err, (char*)udt_getlasterror_desc());
        return -1;
    }

    *ssock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (bind(*ssock, res->ai_addr, res->ai_addrlen) != 0) {
        int err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createTCPSocket", err, "Socket Bind error");
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:createTCPSocket", err, (char*)udt_getlasterror_desc());
        return -1;
    }

    freeaddrinfo(res);
    return 0;
}

// connect conflicts with system function
int c_connect(UDTSOCKET* usock, int port)
{
    struct addrinfo hints, *peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char buffer[16];
    sprintf(buffer, "%d", port);

    if (0 != getaddrinfo(g_Localhost, buffer, &hints, &peer)) {
        int err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:c_connect", err, "Socket Connect error");
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:c_connect", err, (char*)udt_getlasterror_desc());
        return -1;
    }

    udt_connect(*usock, peer->ai_addr, peer->ai_addrlen);

    freeaddrinfo(peer);
    return 0;
}

int tcp_connect(SYSSOCKET* ssock, int port)
{
    struct addrinfo hints, *peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = g_IP_Version;
    hints.ai_socktype = g_Socket_Type;

    char buffer[16];
    sprintf(buffer, "%d", port);

    if (0 != getaddrinfo(g_Localhost, buffer, &hints, &peer)) {
        int err = 999;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:tcp_connect", err, "Socket Connect error");
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:tcp_connect", err, (char*)udt_getlasterror_desc());
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
int TestPlugin::testudt(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Start a mini server loop and create a separate communiation channel with the client bye-passing the TCP socket

    int client; // listening socket id
    int false = 0;
    int err = 0;

    // Create a UDT socket without specifying the port number

    if (createUDTSocket(&client, 0, false) < 0) {
        ;
        err = 9990;
        addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:udt", err, "Unable to create a UDT Socket");
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
    gettimeofday(&tm1, nullptr);

    time_t ticks = time(nullptr);
    char sendBuff[1025];
    snprintf(sendBuff, sizeof(sendBuff), "%.24s", ctime(&ticks));
    int tosend = strlen(sendBuff) + 1;
    int sent = udt_send(client, sendBuff, tosend, 0);

    tosend = g_TotalNum * sizeof(int32_t);
    while (tosend > 0) {
        int sent = udt_send(client, (char*)buffer + g_TotalNum * sizeof(int32_t) - tosend, tosend, 0);
        if (sent < 0) {
            err = 9990;
            addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:udt", err, "Unable to Send Data");
            addIdamError(UDA_CODE_ERROR_TYPE, "testplugin:udt", err, (char*)udt_getlasterror_desc());
            break;
        }
        tosend -= sent;
    }

    gettimeofday(&tm2, nullptr);
    int dsecs = (int)(tm2.tv_sec - tm1.tv_sec);
    int dmics = (int)(tm2.tv_usec - tm1.tv_usec);

    buffer[0] = dsecs;
    buffer[1] = dmics;
    tosend = 2 * sizeof(int32_t);
    sent = udt_send(client, (char*)buffer, tosend, 0);

    ticks = time(nullptr);
    snprintf(sendBuff, sizeof(sendBuff), "%.24s", ctime(&ticks));
    tosend = (int)strlen(sendBuff) + 1;
    sent = udt_send(client, sendBuff, tosend, 0);

    // Close the connection

    udt_close(client);

    // Return IDAM status

    initDataBlock(data_block);

    data_block->rank = 0;
    data_block->data_n = 1;
    data_block->data_type = UDA_TYPE_INT;

    int* status = (int*)malloc(sizeof(int));
    status[0] = 0;
    data_block->data = (char*)status;

    strcpy(data_block->data_desc, "testplugins:udt status");
    strcpy(data_block->data_label, "");
    strcpy(data_block->data_units, "");

    return err;
}

#endif // TESTUDT
