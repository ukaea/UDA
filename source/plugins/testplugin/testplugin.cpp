#include "testplugin.h"

#include <cstddef>
#include <cstdlib>
#include <vector>

#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <uda/client.h>
#include <uda/plugins.h>
#include <uda/uda_plugin_base.hpp>
#include <uda/serialisation/capnp_serialisation.h>

#include "teststructs.h"

#define MAXELEMENTNAME 1024

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

UDA_PLUGIN_INFO UDA_PLUGIN_INFO_FUNCTION_NAME()
{
    UDA_PLUGIN_INFO info;
    info.name = "TESTPLUGIN";
    info.version = "1.0";
    info.entry_function = "testPlugin";
    info.type = UDA_PLUGIN_CLASS_FUNCTION;
    info.extension = "";
    info.default_method = "help";
    info.description = "Generate Test Data";
    info.cache_mode = UDA_PLUGIN_CACHE_MODE_OK;
    info.is_private = false;
    info.interface_version = 1;
    return info;
}

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
    register_method("test1", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test0));
    register_method("test2", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test2));
    register_method("test3", static_cast<UDAPluginBase::plugin_member_type>(&TestPlugin::test2));
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

extern "C" int testPlugin(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    static TestPlugin plugin = {};
    return plugin.call(plugin_interface);
}

void testError1(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Test of Error Management within Plugins
    int err = 9991;
    udaAddPluginError(plugin_interface, "testplugin", err, "Test #1 of Error State Management");
}

void testError2(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Test of Error Management within Plugins
    int err = 9992;
    udaAddPluginError(plugin_interface, "testplugin", err, "Test #2 of Error State Management");
}

int TestPlugin::test0(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    const char* help = "Hello World!";

    if (get_function(plugin_interface) == "test0") {
        size_t shape[] = {strlen(help)};
        udaPluginReturnDataCharArray(plugin_interface, help, 1, shape,
                                     "testplugins: test0 = single string as a char array");
    } else {
        udaPluginReturnDataStringScalar(plugin_interface, help, "testplugins: test1 = single string");
    }

    return 0;
}

int TestPlugin::test2(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // An array of strings can be formed in two distinct ways.
    // 1> A fixed block of contiguous memory with each string beginning at a well-defined regular location - as if each
    //    string were the same length.
    // 2> An array of string pointers: each string has its own length. Memory is not contiguous. This is the normal
    //    representation of string arrays.

    // To pass back the data as a block of chars/bytes or as type STRING, model 1 must be adopted - its how the
    // middleware operates. By labeling the type as STRING, we can convert the data within the client to the correct
    // type

    // create original data using model 2

    std::vector<std::string> strings = {
        "Hello World!",
        "Qwerty keyboard",
        "MAST Upgrade",
    };

    size_t max_len = 0;
    for (const auto& string : strings) {
        max_len = std::max(max_len, string.size() + 1);
    }

    // Create a block of contigous memory and assign all bytes to nullptr character
    auto data = std::make_unique<char[]>(strings.size() * max_len);
    for (size_t i = 0; i < strings.size(); i++) {
        strcpy(&data[i * max_len], strings[i].data());
    }

    if (get_function(plugin_interface) == "test2") {
        size_t shape[] = {max_len, strings.size()};
        udaPluginReturnDataCharArray(plugin_interface, data.get(), 2, shape, "testplugins: test2 = 2D array of chars");
    } else {
        size_t shape[] = {strings.size()};
        const char* ptrs[3];
        ptrs[0] = strings[0].data();
        ptrs[1] = strings[1].data();
        ptrs[2] = strings[2].data();
        udaPluginReturnDataStringArray(plugin_interface, ptrs, 1, shape, "testplugins: test3 = array of strings");
    }

    return 0;
}

int TestPlugin::test4(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test4 {
        char value[56];
    } TEST4;

    TEST4* data;

    int offset = 0;

    COMPOUNDFIELD* fields[1] = {nullptr};
    const auto plugin_version = udaPluginGetVersion(plugin_interface);
    printf("%d", plugin_version);

    int shape[1] = {56};
    fields[0] = udaNewCompoundFixedStringField("value", "string structure element", &offset, 1, shape);

    USERDEFINEDTYPE* test4_type = udaNewUserType("TEST4", "Test #4", 0, 0, nullptr, sizeof(TEST4), 1, fields);
    udaAddUserType(plugin_interface, test4_type);

    // Create Data

    data = (TEST4*)malloc(sizeof(TEST4)); // Structured Data Must be a heap variable
    strcpy(data->value, "012345678901234567890");
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST4), "TEST4");

    // Pass Data
    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST4", " Structure Data Test #4");
    udaPluginReturnDataLabel(plugin_interface, "Values: 012345678901234567890");

    return 0;
}

int TestPlugin::test5(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test5 {
        char value[3][56];
    } TEST5;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    int shape[2] = {56, 3};
    fields[0] = udaNewCompoundFixedStringField("value", "string structure element", &offset, 2, shape);

    USERDEFINEDTYPE* test5_type = udaNewUserType("TEST5", "Test #5", 0, 0, nullptr, sizeof(TEST5), 1, fields);
    udaAddUserType(plugin_interface, test5_type);

    TEST5* data;

    // Create Data

    data = (TEST5*)malloc(sizeof(TEST5)); // Structured Data Must be a heap variable
    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST5), "TEST5");

    // Pass Data
    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST5", "Structure Data Test #5");
    udaPluginReturnDataLabel(plugin_interface, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");

    return 0;
}

int TestPlugin::test6(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test6 {
        char* value;
    } TEST6;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundVarStringField("value", "string structure element", &offset);

    USERDEFINEDTYPE* test6_type = udaNewUserType("TEST6", "Test #6", 0, 0, nullptr, sizeof(TEST6), 1, fields);
    udaAddUserType(plugin_interface, test6_type);

    // Create Data

    TEST6* data = (TEST6*)malloc(sizeof(TEST6)); // Structured Data Must be a heap variable
    data->value = (char*)malloc(56 * sizeof(char));
    strcpy(data->value, "PI=3.1415927");
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST6), "TEST6");
    udaRegisterMalloc(plugin_interface, (void*)data->value, 1, 56 * sizeof(char), "char");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST6", "Structure Data Test #6");
    udaPluginReturnDataLabel(plugin_interface, "Value: PI=3.1415927");

    return 0;
}

int TestPlugin::test7(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test7 {
        char* value[3]; // 3 strings of arbitrary length
    } TEST7;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    int shape[1] = {3};
    fields[0] = udaNewCompoundVarStringArrayField("value", "string structure element", &offset, 1, shape);

    USERDEFINEDTYPE* test7_type = udaNewUserType("TEST7", "Test #7", 0, 0, nullptr, sizeof(TEST7), 1, fields);
    udaAddUserType(plugin_interface, test7_type);

    // Create Data

    TEST7* data = (TEST7*)malloc(sizeof(TEST7)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST7), "TEST7");

    data->value[0] = (char*)malloc(56 * sizeof(char));
    data->value[1] = (char*)malloc(55 * sizeof(char));
    data->value[2] = (char*)malloc(54 * sizeof(char));

    udaRegisterMalloc(plugin_interface, (void*)data->value[0], 56, sizeof(char), "char");
    udaRegisterMalloc(plugin_interface, (void*)data->value[1], 55, sizeof(char), "char");
    udaRegisterMalloc(plugin_interface, (void*)data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST7", "Structure Data Test #7");
    udaPluginReturnDataLabel(plugin_interface, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");

    return 0;
}

int TestPlugin::test8(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test8 {
        char** value; // arbitrary number of strings of arbitrary length
    } TEST8;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundVarStringArrayField("value", "string structure element", &offset, 0, nullptr);

    USERDEFINEDTYPE* test8_type = udaNewUserType("TEST8", "Test #8", 0, 0, nullptr, sizeof(TEST8), 1, fields);
    udaAddUserType(plugin_interface, test8_type);

    // Create Data

    TEST8* data = (TEST8*)malloc(sizeof(TEST8)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST8), "TEST8");

    data->value = (char**)malloc(3 * sizeof(char*));
    udaRegisterMalloc(plugin_interface, (void*)data->value, 3, sizeof(char*), "STRING *");

    data->value[0] = (char*)malloc(56 * sizeof(char));
    data->value[1] = (char*)malloc(55 * sizeof(char));
    data->value[2] = (char*)malloc(54 * sizeof(char));

    udaRegisterMalloc(plugin_interface, (void*)data->value[0], 56, sizeof(char), "char");
    udaRegisterMalloc(plugin_interface, (void*)data->value[1], 55, sizeof(char), "char");
    udaRegisterMalloc(plugin_interface, (void*)data->value[2], 54, sizeof(char), "char");

    strcpy(data->value[0], "012345678901234567890");
    strcpy(data->value[1], "QWERTY KEYBOARD");
    strcpy(data->value[2], "MAST TOKAMAK");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST8", "Structure Data Test #8");
    udaPluginReturnDataLabel(plugin_interface, "Values: 012345678901234567890, QWERTY KEYBOARD, MAST TOKAMAK");

    return 0;
}

int TestPlugin::test9(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    init_structure_definitions(plugin_interface);

    // Create Data

    TEST9* data = (TEST9*)malloc(4 * sizeof(TEST9)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 4, sizeof(TEST9), "TEST9");

    {
        for (int i = 0; i < 4; i++) {
            strcpy(data[i].v1, "123212321232123212321");
            strcpy(data[i].v2[0], "012345678901234567890");
            strcpy(data[i].v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v2[2], "MAST TOKAMAK");

            data[i].v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v3, "PI=3.1415927");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v3, 1, 56 * sizeof(char), "char");

            data[i].v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v4[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v4[0], "012345678901234567890");
            strcpy(data[i].v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v4[2], "MAST TOKAMAK");

            data[i].v5 = (char**)malloc(3 * sizeof(char*));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5, 3, sizeof(char*), "STRING *");
            data[i].v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v5[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v5[0], "012345678901234567890");
            strcpy(data[i].v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v5[2], "MAST TOKAMAK");
        }
    }

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST9", "Structure Data Test #9");
    udaPluginReturnDataLabel(plugin_interface, "Multiple test results");

    return 0;
}

int TestPlugin::test9A(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Create Data

    TEST9A* data = (TEST9A*)malloc(4 * sizeof(TEST9A)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 4, sizeof(TEST9A), "TEST9A");

    {
        for (int i = 0; i < 4; i++) {
            strcpy(data[i].v1, "123212321232123212321");
            strcpy(data[i].v2[0], "012345678901234567890");
            strcpy(data[i].v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v2[2], "MAST TOKAMAK");

            data[i].v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v3, "PI=3.1415927");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v3, 1, 56 * sizeof(char), "char");

            data[i].v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v4[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v4[0], "012345678901234567890");
            strcpy(data[i].v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v4[2], "MAST TOKAMAK");

            data[i].v5 = (char**)malloc(3 * sizeof(char*));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5, 3, sizeof(char*), "STRING *");
            data[i].v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v5[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v5[0], "012345678901234567890");
            strcpy(data[i].v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v5[2], "MAST TOKAMAK");

            strcpy(data[i].v6.v1, "123212321232123212321");
            strcpy(data[i].v6.v2[0], "012345678901234567890");
            strcpy(data[i].v6.v2[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v2[2], "MAST TOKAMAK");

            data[i].v6.v3 = (char*)malloc(56 * sizeof(char));
            strcpy(data[i].v6.v3, "PI=3.1415927");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v3, 1, 56 * sizeof(char), "char");

            data[i].v6.v4[0] = (char*)malloc(56 * sizeof(char));
            data[i].v6.v4[1] = (char*)malloc(55 * sizeof(char));
            data[i].v6.v4[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v4[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v4[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v4[2], 54, sizeof(char), "char");
            strcpy(data[i].v6.v4[0], "012345678901234567890");
            strcpy(data[i].v6.v4[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v4[2], "MAST TOKAMAK");

            data[i].v6.v5 = (char**)malloc(3 * sizeof(char*));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v5, 3, sizeof(char*), "STRING *");
            data[i].v6.v5[0] = (char*)malloc(56 * sizeof(char));
            data[i].v6.v5[1] = (char*)malloc(55 * sizeof(char));
            data[i].v6.v5[2] = (char*)malloc(54 * sizeof(char));
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v5[0], 56, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v5[1], 55, sizeof(char), "char");
            udaRegisterMalloc(plugin_interface, (void*)data[i].v6.v5[2], 54, sizeof(char), "char");
            strcpy(data[i].v6.v5[0], "012345678901234567890");
            strcpy(data[i].v6.v5[1], "QWERTY KEYBOARD");
            strcpy(data[i].v6.v5[2], "MAST TOKAMAK");
        }
    }

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST9A", "Structure Data Test #9A");
    udaPluginReturnDataLabel(plugin_interface, "Multiple test results");

    return 0;
}

int TestPlugin::test10(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Create Data

    int* data = (int*)malloc(sizeof(int));
    data[0] = 7;

    // Pass Data
    udaPluginReturnDataIntScalar(plugin_interface, 7, "Structure Data Test #10");
    udaPluginReturnDataLabel(plugin_interface, "Value: 7");

    return 0;
}

int TestPlugin::test11(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test11 {
        int value;
    } TEST11;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundField("value", "single integer structure element", &offset, UDA_TYPE_INT, 0, nullptr);

    USERDEFINEDTYPE* test11_type = udaNewUserType("TEST11", "Test #11", 0, 0, nullptr, sizeof(TEST11), 1, fields);
    udaAddUserType(plugin_interface, test11_type);

    // Create Data

    TEST11* data = (TEST11*)malloc(sizeof(TEST11)); // Structured Data Must be a heap variable
    data[0].value = 11;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST11), "TEST11");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST11", "Structure Data Test #11");
    udaPluginReturnDataLabel(plugin_interface, "Value: 11");

    return 0;
}

int TestPlugin::test12(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test12 {
        int value[3];
    } TEST12;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    int shape[1] = {3};
    fields[0] = udaNewCompoundField("value", "array integer structure element", &offset, UDA_TYPE_INT, 1, shape);

    USERDEFINEDTYPE* test12_type = udaNewUserType("TEST12", "Test #12", 0, 0, nullptr, sizeof(TEST12), 1, fields);
    udaAddUserType(plugin_interface, test12_type);

    TEST12* data = (TEST12*)malloc(sizeof(TEST12)); // Structured Data Must be a heap variable
    data[0].value[0] = 10;
    data[0].value[1] = 11;
    data[0].value[2] = 12;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST12), "TEST12");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST12", "Structure Data Test #12");
    udaPluginReturnDataLabel(plugin_interface, "Value: 10,11,12");

    return 0;
}

int TestPlugin::test13(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test13 {
        int value[2][3];
    } TEST13;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    int shape[2] = {2, 3};
    fields[0] = udaNewCompoundField("value", "2d array integer structure element", &offset, UDA_TYPE_INT, 2, shape);

    USERDEFINEDTYPE* test13_type = udaNewUserType("TEST13", "Test #13", 0, 0, nullptr, sizeof(TEST13), 1, fields);
    udaAddUserType(plugin_interface, test13_type);

    // Create Data

    TEST13* data = (TEST13*)malloc(sizeof(TEST13)); // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST13), "TEST13");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST13", "Structure Data Test #13");
    udaPluginReturnDataLabel(plugin_interface, "Value: {0,1,2},{10,11,12}");

    return 0;
}

int TestPlugin::test14(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test14 {
        int* value;
    } TEST14;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundPointerField("value", "single integer structure element", &offset, UDA_TYPE_INT, true);

    USERDEFINEDTYPE* test14_type = udaNewUserType("TEST14", "Test #14", 0, 0, nullptr, sizeof(TEST14), 1, fields);
    udaAddUserType(plugin_interface, test14_type);

    // Create Data

    TEST14* data = (TEST14*)malloc(sizeof(TEST14)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST14), "TEST14");

    data[0].value = (int*)malloc(sizeof(int));
    udaRegisterMalloc(plugin_interface, (void*)data[0].value, 1, sizeof(int), "int");

    data[0].value[0] = 14;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST14", "Structure Data Test #14");
    udaPluginReturnDataLabel(plugin_interface, "int *value: 14");

    return 0;
}

int TestPlugin::test15(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test15 {
        int* value;
    } TEST15;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundPointerField("value", "single integer structure element", &offset, UDA_TYPE_INT, true);

    USERDEFINEDTYPE* test15_type = udaNewUserType("TEST15", "Test #15", 0, 0, nullptr, sizeof(TEST15), 1, fields);
    udaAddUserType(plugin_interface, test15_type);

    // Create Data

    TEST15* data = (TEST15*)malloc(sizeof(TEST15)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST15), "TEST15");

    data[0].value = (int*)malloc(3 * sizeof(int));
    int shape[1] = {3};
    udaRegisterMallocArray(plugin_interface, (void*)data[0].value, 3, sizeof(int), "int", 1, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST15", "Structure Data Test #15");
    udaPluginReturnDataLabel(plugin_interface, "Values: 13,14,15");

    return 0;
}

int TestPlugin::test16(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test16 {
        int* value;
    } TEST16;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundPointerField("value", "single integer structure element", &offset, UDA_TYPE_INT, true);

    USERDEFINEDTYPE* test16_type = udaNewUserType("TEST16", "Test #16", 0, 0, nullptr, sizeof(TEST16), 1, fields);
    udaAddUserType(plugin_interface, test16_type);

    // Create Data

    TEST16* data = (TEST16*)malloc(sizeof(TEST16)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST16), "TEST16");

    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 2;
    shape[1] = 3;
    int count = shape[0] * shape[1];
    int rank = 2;
    data[0].value = (int*)malloc(count * sizeof(int));
    udaRegisterMallocArray(plugin_interface, (void*)data[0].value, count, sizeof(int), "int", rank, shape);

    data[0].value[0] = 0;
    data[0].value[1] = 1;
    data[0].value[2] = 2;
    data[0].value[3] = 10;
    data[0].value[4] = 11;
    data[0].value[5] = 12;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST16", "Structure Data Test #16");
    udaPluginReturnDataLabel(plugin_interface, "Values: {0,1,2},{10,11,12}");

    return 0;
}

int TestPlugin::test18(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test18 {
        int value;
    } TEST18;

    COMPOUNDFIELD* fields[1] = {nullptr};

    int offset = 0;

    fields[0] = udaNewCompoundField("value", "single integer structure element", &offset, UDA_TYPE_INT, 0, nullptr);

    USERDEFINEDTYPE* test18_type = udaNewUserType("TEST18", "Test #18", 0, 0, nullptr, sizeof(TEST18), 1, fields);
    udaAddUserType(plugin_interface, test18_type);

    // Create Data
    constexpr int data_n = 100000;
    TEST18* data = (TEST18*)malloc(data_n * sizeof(TEST18)); // Structured Data Must be a heap variable
    for (int i = 0; i < data_n; i++) {
        data[i].value = i;
    }
    udaRegisterMalloc(plugin_interface, (void*)data, data_n, sizeof(TEST18), "TEST18");

    // Pass Data

    int data_shape[1] = {data_n};
    udaPluginReturnCompoundArrayData(plugin_interface, (char*)data, "TEST18", "Structure Data Test #18", 1, data_shape);
    udaPluginReturnDataLabel(plugin_interface, "100000 Values: i 0, 100000");

    return 0;
}

int TestPlugin::test19(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test19A {
        int value;
    } TEST19A;

    COMPOUNDFIELD* test19a_fields[1] = {nullptr};

    int offset = 0;

    test19a_fields[0] = udaNewCompoundField("value", "single integer structure element", &offset, UDA_TYPE_INT, 0,
                                            nullptr);

    USERDEFINEDTYPE* test19a_type =
        udaNewUserType("TEST19A", "Test #19", 0, 0, nullptr, sizeof(TEST19A), 1, test19a_fields);
    udaAddUserType(plugin_interface, test19a_type);

    typedef struct Test19 {
        int value;
        TEST19A vals[7];
    } TEST19;

    COMPOUNDFIELD* test19_fields[2] = {nullptr};

    offset = 0;

    test19_fields[0] = udaNewCompoundField("value", "single integer structure element", &offset, UDA_TYPE_INT, 0, nullptr);
    int shape[1] = {7};
    test19_fields[1] =
        udaNewCompoundUserTypeArrayField("vals", "single integer structure element", &offset, test19a_type, 1, shape);

    USERDEFINEDTYPE* test19_type =
        udaNewUserType("TEST19", "Test #19", 0, 0, nullptr, sizeof(TEST19), 2, test19_fields);
    udaAddUserType(plugin_interface, test19_type);

    // Create Data

    constexpr int data_n = 3;
    TEST19* data = (TEST19*)malloc(data_n * sizeof(TEST19)); // Structured Data Must be a heap variable
    for (int i = 0; i < data_n; i++) {
        data[i].value = 3 + i;
        for (int j = 0; j < 7; j++) {
            data[i].vals[j].value = 10 * i + j;
        }
    }
    udaRegisterMalloc(plugin_interface, (void*)data, data_n, sizeof(TEST19), "TEST19");

    // Pass Data

    int data_shape[1] = {data_n};
    udaPluginReturnCompoundArrayData(plugin_interface, (char*)data, "TEST19", "Structure Data Test #19", 1, data_shape);
    udaPluginReturnDataLabel(plugin_interface, "Values: ");

    return 0;
}

int TestPlugin::test20(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    // Create Data

    short* data = (short*)malloc(sizeof(short));
    data[0] = 7;

    // Pass Data

    udaPluginReturnDataShortScalar(plugin_interface, 7, "Structure Data Test #20");
    udaPluginReturnDataLabel(plugin_interface, "Short Value: 7");

    return 0;
}

int TestPlugin::test21(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test21 {
        short value;
    } TEST21;

    COMPOUNDFIELD* test21_fields[1] = {nullptr};

    int offset = 0;

    test21_fields[0] = udaNewCompoundField("value", "single short structure element", &offset, UDA_TYPE_SHORT, 0, nullptr);

    USERDEFINEDTYPE* test21_type =
        udaNewUserType("TEST21", "Test #21", 0, 0, nullptr, sizeof(TEST21), 1, test21_fields);
    udaAddUserType(plugin_interface, test21_type);

    // Create Data

    TEST21* data = (TEST21*)malloc(sizeof(TEST21)); // Structured Data Must be a heap variable
    data[0].value = 21;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST21), "TEST21");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST21", "Structure Data Test #21");
    udaPluginReturnDataLabel(plugin_interface, "Short Value: 21");

    return 0;
}

int TestPlugin::test22(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test22 {
        short value[3];
    } TEST22;

    COMPOUNDFIELD* test22_fields[1] = {nullptr};

    int offset = 0;
    int shape[1] = {3};
    test22_fields[0] = udaNewCompoundField("value", "single short structure element", &offset, UDA_TYPE_SHORT, 1, shape);

    USERDEFINEDTYPE* test22_type =
        udaNewUserType("TEST22", "Test #22", 0, 0, nullptr, sizeof(TEST22), 1, test22_fields);
    udaAddUserType(plugin_interface, test22_type);

    // Create Data

    TEST22* data = (TEST22*)malloc(sizeof(TEST22)); // Structured Data Must be a heap variable
    data[0].value[0] = 20;
    data[0].value[1] = 21;
    data[0].value[2] = 22;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST22), "TEST22");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST22", "Structure Data Test #22");
    udaPluginReturnDataLabel(plugin_interface, "Short Array Values: 20,21,22");

    return 0;
}

int TestPlugin::test23(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test23 {
        short value[2][3];
    } TEST23;

    COMPOUNDFIELD* test23_fields[1] = {nullptr};

    int offset = 0;
    int shape[2] = {3, 2};
    test23_fields[0] = udaNewCompoundField("value", "single short structure element", &offset, UDA_TYPE_SHORT, 2, shape);

    USERDEFINEDTYPE* test23_type =
        udaNewUserType("TEST23", "Test #23", 0, 0, nullptr, sizeof(TEST23), 1, test23_fields);
    udaAddUserType(plugin_interface, test23_type);

    // Create Data

    TEST23* data = (TEST23*)malloc(sizeof(TEST23)); // Structured Data Must be a heap variable
    data[0].value[0][0] = 0;
    data[0].value[0][1] = 1;
    data[0].value[0][2] = 2;
    data[0].value[1][0] = 10;
    data[0].value[1][1] = 11;
    data[0].value[1][2] = 12;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST23), "TEST23");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST23", "Structure Data Test #23");
    udaPluginReturnDataLabel(plugin_interface, "Values: {0,1,2},{10,11,12}");

    return 0;
}

int TestPlugin::test24(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test24 {
        short* value;
    } TEST24;

    COMPOUNDFIELD* test24_fields[1] = {nullptr};
    int offset = 0;

    test24_fields[0] = udaNewCompoundPointerField("value", "single short structure element", &offset, UDA_TYPE_SHORT, true);

    USERDEFINEDTYPE* test24_type =
        udaNewUserType("TEST24", "Test #24", 0, 0, nullptr, sizeof(TEST24), 1, test24_fields);
    udaAddUserType(plugin_interface, test24_type);

    // Create Data

    TEST24* data = (TEST24*)malloc(sizeof(TEST24)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST24), "TEST24");

    data[0].value = (short*)malloc(sizeof(short));
    udaRegisterMalloc(plugin_interface, (void*)data[0].value, 1, sizeof(short), "short");

    data[0].value[0] = 24;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST24", "Structure Data Test #24");
    udaPluginReturnDataLabel(plugin_interface, "short *value: 14");

    return 0;
}

int TestPlugin::test25(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test25 {
        short* value;
    } TEST25;

    COMPOUNDFIELD* test25_fields[1] = {nullptr};
    int offset = 0;

    test25_fields[0] = udaNewCompoundPointerField("value", "single short structure element", &offset, UDA_TYPE_SHORT, true);

    USERDEFINEDTYPE* test25_type =
        udaNewUserType("TEST25", "Test #25", 0, 0, nullptr, sizeof(TEST25), 1, test25_fields);
    udaAddUserType(plugin_interface, test25_type);
    // Create Data

    TEST25* data = (TEST25*)malloc(sizeof(TEST25)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST25), "TEST25");

    data[0].value = (short*)malloc(3 * sizeof(short));
    int shape[] = {3};
    udaRegisterMallocArray(plugin_interface, (void*)data[0].value, 3, sizeof(short), "short", 1, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST25", "Structure Data Test #25");
    udaPluginReturnDataLabel(plugin_interface, "Short Values: 13,14,15");

    return 0;
}

int TestPlugin::test26(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test26 {
        short* value;
    } TEST26;

    COMPOUNDFIELD* test26_fields[1] = {nullptr};
    int offset = 0;

    test26_fields[0] = udaNewCompoundPointerField("value", "single short structure element", &offset, UDA_TYPE_SHORT, true);

    USERDEFINEDTYPE* test26_type =
        udaNewUserType("TEST26", "Test #26", 0, 0, nullptr, sizeof(TEST26), 1, test26_fields);
    udaAddUserType(plugin_interface, test26_type);

    // Create Data Structure

    TEST26* data = (TEST26*)malloc(sizeof(TEST26)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST26), "TEST26");

    // Data is a compact Fortran like rank 2 array

    data[0].value = (short*)malloc(6 * sizeof(short));
    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 3;
    shape[1] = 2;
    udaRegisterMallocArray(plugin_interface, (void*)data[0].value, 6, sizeof(short), "short", 2, shape);

    data[0].value[0] = 13;
    data[0].value[1] = 14;
    data[0].value[2] = 15;

    data[0].value[3] = 23;
    data[0].value[4] = 24;
    data[0].value[5] = 25;

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST26", "Structure Data Test #26");
    udaPluginReturnDataLabel(plugin_interface, "Short Values: 13,14,15   23,24,25");

    return 0;
}

int TestPlugin::test27(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test27 {
        short value[2][3][4];
    } TEST27;

    COMPOUNDFIELD* test27_fields[1] = {nullptr};
    int offset = 0;

    int shape[3] = {4, 3, 2};
    test27_fields[0] =
        udaNewCompoundField("value", "single short structure element", &offset, UDA_TYPE_SHORT, 3, shape);

    USERDEFINEDTYPE* test27_type =
        udaNewUserType("TEST27", "Test #27", 0, 0, nullptr, sizeof(TEST27), 1, test27_fields);
    udaAddUserType(plugin_interface, test27_type);

    // Create Data

    TEST27* data = (TEST27*)malloc(sizeof(TEST27)); // Structured Data Must be a heap variable

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

    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST27), "TEST27");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST27", "Structure Data Test #27");
    udaPluginReturnDataLabel(plugin_interface, "Values: {0,1,2,3},{10,11,12,13},...");

    return 0;
}

int TestPlugin::test28(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test28 {
        short* value;
    } TEST28;

    COMPOUNDFIELD* test28_fields[1] = {nullptr};
    int offset = 0;

    test28_fields[0] = udaNewCompoundPointerField("value", "single short structure element", &offset, UDA_TYPE_SHORT, true);

    USERDEFINEDTYPE* test28_type =
        udaNewUserType("TEST28", "Test #28", 0, 0, nullptr, sizeof(TEST28), 1, test28_fields);
    udaAddUserType(plugin_interface, test28_type);

    // Create Data Structure

    TEST28* data = (TEST28*)malloc(sizeof(TEST28)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST28), "TEST28");

    // Data is a compact Fortran like rank 3 array

    data[0].value = (short*)malloc(24 * sizeof(short));
    int* shape = (int*)malloc(3 * sizeof(int));
    shape[0] = 4;
    shape[1] = 3;
    shape[2] = 2;
    udaRegisterMallocArray(plugin_interface, (void*)data[0].value, 24, sizeof(short), "short", 3, shape);

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

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST28", "Structure Data Test #28");
    udaPluginReturnDataLabel(plugin_interface, "Short Values: 13,14,15   23,24,25");

    return 0;
}

int TestPlugin::test30(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test30 {
        double R;
        double Z;
    } TEST30;

    COMPOUNDFIELD* test30_fields[2] = {nullptr};
    int offset = 0;

    test30_fields[0] = udaNewCompoundField("R", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);
    test30_fields[1] = udaNewCompoundField("Z", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);

    USERDEFINEDTYPE* test30_type =
        udaNewUserType("TEST30", "Test #30", 0, 0, nullptr, sizeof(TEST30), 2, test30_fields);
    udaAddUserType(plugin_interface, test30_type);

    // Create Data

    TEST30* data = (TEST30*)malloc(sizeof(TEST30)); // Structured Data Must be a heap variable
    data[0].R = 1.0;
    data[0].Z = 2.0;
    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST30), "TEST30");

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST30", "Structure Data Test #30");
    udaPluginReturnDataLabel(plugin_interface, "Double Values: (1, 2)");

    return 0;
}

int TestPlugin::test31(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test31 {
        double R;
        double Z;
    } TEST31;

    COMPOUNDFIELD* test31_fields[2] = {nullptr};
    int offset = 0;

    test31_fields[0] = udaNewCompoundField("R", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);
    test31_fields[1] = udaNewCompoundField("Z", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);

    USERDEFINEDTYPE* test31_type =
        udaNewUserType("TEST31", "Test #31", 0, 0, nullptr, sizeof(TEST31), 2, test31_fields);
    udaAddUserType(plugin_interface, test31_type);

    // Create Data

    constexpr int count = 100;
    TEST31* data = (TEST31*)malloc(count * sizeof(TEST31)); // Structured Data Must be a heap variable

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

    udaRegisterMallocArray(plugin_interface, (void*)data, count, sizeof(TEST31), "TEST31", rank, shape);

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST31", "Structure Data Test #31");
    udaPluginReturnDataLabel(plugin_interface, "Double Values [5, 20] : (1*, 10*)");

    return 0;
}

int TestPlugin::test32(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test32A {
        double R;
        double Z;
    } TEST32A;

    COMPOUNDFIELD* test32a_fields[2] = {nullptr};
    int offset = 0;

    test32a_fields[0] = udaNewCompoundField("R", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);
    test32a_fields[1] = udaNewCompoundField("Z", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);

    USERDEFINEDTYPE* test32a_type =
        udaNewUserType("TEST32A", "Test #32", 0, 0, nullptr, sizeof(TEST32A), 2, test32a_fields);
    udaAddUserType(plugin_interface, test32a_type);

    typedef struct Test32 {
        int count;
        TEST32A coords[100];
    } TEST32;

    COMPOUNDFIELD* test32_fields[2] = {nullptr};
    offset = 0;

    test32_fields[0] = udaNewCompoundField("count", "int structure element", &offset, UDA_TYPE_INT, 0, nullptr);
    int shape[1] = {100};
    offset = offsetof(TEST32, coords);
    test32_fields[1] = udaNewCompoundUserTypeArrayField("coords", "structure TEST32A", &offset, test32a_type, 1, shape);

    USERDEFINEDTYPE* test32_type =
        udaNewUserType("TEST32", "Test #32", 0, 0, nullptr, sizeof(TEST32), 2, test32_fields);
    udaAddUserType(plugin_interface, test32_type);

    // Create Data

    constexpr int data_n = 1;
    TEST32* data = (TEST32*)malloc(data_n * sizeof(TEST32)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, data_n, sizeof(TEST32), "TEST32");

    constexpr int field_count = 100;

    memset(data, '\0', sizeof(TEST32));
    data->count = field_count;

    for (int i = 0; i < field_count; i++) {
        data->coords[i].R = 1.0 * i;
        data->coords[i].Z = 10.0 * i;
    }

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST32", "Structure Data Test #32");
    udaPluginReturnDataLabel(plugin_interface, "Double Values [5, 20] : (1*, 10*)");

    return 0;
}

int TestPlugin::test33(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test33A {
        double R;
        double Z;
    } TEST33A;

    COMPOUNDFIELD* test33a_fields[2] = {nullptr};
    int offset = 0;

    test33a_fields[0] = udaNewCompoundField("R", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);
    test33a_fields[1] = udaNewCompoundField("Z", "double structure element", &offset, UDA_TYPE_DOUBLE, 0, nullptr);

    USERDEFINEDTYPE* test33a_type =
        udaNewUserType("TEST33A", "Test #33", 0, 0, nullptr, sizeof(TEST33A), 2, test33a_fields);
    udaAddUserType(plugin_interface, test33a_type);

    typedef struct Test33 {
        int count;
        TEST33A* coords;
    } TEST33;

    COMPOUNDFIELD* test33_fields[2] = {nullptr};
    offset = 0;

    test33_fields[0] = udaNewCompoundField("count", "int structure element", &offset, UDA_TYPE_INT, 0, nullptr);
//    int shape[1] = {100};
//    test33_fields[1] = udaNewCompoundUserTypeArrayField("coords", "structure TEST33A", &offset, test33a_type, 1, shape);
    offset = offsetof(TEST33, coords);
    test33_fields[1] = udaNewCompoundUserTypePointerField("coords", "structure TEST33A", &offset, test33a_type);

    USERDEFINEDTYPE* test33_type =
        udaNewUserType("TEST33", "Test #33", 0, 0, nullptr, sizeof(TEST33), 2, test33_fields);
    udaAddUserType(plugin_interface, test33_type);

    // Create Data

    constexpr int data_n = 1;
    TEST33* data = (TEST33*)malloc(data_n * sizeof(TEST33)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, data_n, sizeof(TEST33), "TEST33");

    data->count = 100;
    data->coords = (TEST33A*)malloc(data->count * sizeof(TEST33A));

    int malloc_shape[2] = {0};
    malloc_shape[0] = 5;
    malloc_shape[1] = 20;

    udaRegisterMallocArray(plugin_interface, (void*)data->coords, data->count, sizeof(TEST33A), "TEST33A", 2,
                           malloc_shape);

    for (int i = 0; i < data->count; i++) {
        data->coords[i].R = 1.0 * i;
        data->coords[i].Z = 10.0 * i;
    }

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST33", "Structure Data Test #33");
    udaPluginReturnDataLabel(plugin_interface, "Double Values [5, 20] : (1*, 10*)");

    return 0;
}

int TestPlugin::test34(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    typedef struct Test34A {
        unsigned char* R;
        unsigned char* Z;
    } TEST34A;

    COMPOUNDFIELD* test34a_fields[2] = {nullptr};
    int offset = 0;

    offset = offsetof(TEST34A, R);
    test34a_fields[0] = udaNewCompoundPointerField("R", "unsigned char structure element", &offset, UDA_TYPE_UNSIGNED_CHAR, false);
    offset = offsetof(TEST34A, Z);
    test34a_fields[1] = udaNewCompoundPointerField("Z", "unsigned char structure element", &offset, UDA_TYPE_UNSIGNED_CHAR, false);

    USERDEFINEDTYPE* test34a_type =
        udaNewUserType("TEST34A", "Test #34", 0, 0, nullptr, sizeof(TEST34A), 2, test34a_fields);
    udaAddUserType(plugin_interface, test34a_type);

    typedef struct Test34 {
        int count;
        TEST34A* coords;
    } TEST34;

    COMPOUNDFIELD* test34_fields[2] = {nullptr};
    offset = 0;

    offset = offsetof(TEST34, count);
    test34_fields[0] = udaNewCompoundField("count", "int structure element", &offset, UDA_TYPE_INT, 0, nullptr);
    offset = offsetof(TEST34, coords);
    test34_fields[1] = udaNewCompoundUserTypePointerField("coords", "structure TEST34A", &offset, test34a_type);

    USERDEFINEDTYPE* test34_type =
        udaNewUserType("TEST34", "Test #34", 0, 0, nullptr, sizeof(TEST34), 2, test34_fields);
    udaAddUserType(plugin_interface, test34_type);

    // Create Data

    constexpr int data_n = 1;
    TEST34* data = (TEST34*)malloc(data_n * sizeof(TEST34)); // Structured Data Must be a heap variable
    udaRegisterMalloc(plugin_interface, (void*)data, data_n, sizeof(TEST34), "TEST34");

    data->count = 100;
    data->coords = (TEST34A*)malloc(data->count * sizeof(TEST34A));

    int rank = 2;
    int* shape = (int*)malloc(2 * sizeof(int));
    shape[0] = 5;
    shape[1] = 20;

    udaRegisterMallocArray(plugin_interface, (void*)data->coords, data->count, sizeof(TEST34A), "TEST34A", rank, shape);

    for (int i = 0; i < data->count; i++) {
        data->coords[i].R = (unsigned char*)malloc(10 * sizeof(unsigned char));
        data->coords[i].Z = (unsigned char*)malloc(10 * sizeof(unsigned char));

        udaRegisterMalloc(plugin_interface, (void*)data->coords[i].R, 10, sizeof(unsigned char), "unsigned char *");
        udaRegisterMalloc(plugin_interface, (void*)data->coords[i].Z, 10, sizeof(unsigned char), "unsigned char *");

        for (int j = 0; j < 10; ++j) {
            data->coords[i].R[j] = (unsigned char)(1 * i);
            data->coords[i].Z[j] = (unsigned char)(10 * i);
        }
    }

    // Pass Data

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "TEST34", "Structure Data Test #34");
    udaPluginReturnDataLabel(plugin_interface, "Double Values [5, 20] : (1*, 10*)");

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
    init_user_defined_type(&usertype); // New structure definition

    strcpy(usertype.name, "TEST40");
    strcpy(usertype.source, "Test #40");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST40); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    int offset = 0;

    COMPOUNDFIELD field;
    init_compound_field(&field);

    // The number of data blocks is given by: request_block->putDataBlockList.blockCount
    // For this test, all blocks must be of the same type: request_block->putDataBlockList.putDataBlock[0].data_type;
    // Repeat call with changing types may cause client side issues!

    UDA_LOG(UDA_LOG_DEBUG, "Number of PutData Blocks: {}", request_block->putDataBlockList.blockCount);

    if (request_block->putDataBlockList.blockCount == 0) {
        err = 999;
        addIdamError(ErrorType::Code, "testplugin", err, "No Put Data Blocks to process!");
        return err;
    }

    defineField(&field, "dataCount", "the number of data array elements", &offset, UDA_SCALAR_UINT);

    switch (request_block->putDataBlockList.putDataBlock[0].data_type) {
        case UDA_TYPE_INT:
            defineField(&field, "data", "the block data array", &offset, UDA_TYPE_INT);
            break;
        case UDA_TYPE_FLOAT:
            defineField(&field, "data", "the block data array", &offset, UDA_ARRAY_FLOAT);
            break;
        case UDA_TYPE_DOUBLE:
            defineField(&field, "data", "the block data array", &offset, UDA_ARRAY_DOUBLE);
            break;
    }

    addUserDefinedType(userdefinedtypelist, usertype);

    init_user_defined_type(&usertype); // New structure definition

    strcpy(usertype.name, "TEST41");
    strcpy(usertype.source, "Test #41");
    usertype.ref_id = 0;
    usertype.imagecount = 0; // No Structure Image data
    usertype.image = nullptr;
    usertype.size = sizeof(TEST41); // Structure size
    usertype.idamclass = UDA_TYPE_COMPOUND;

    offset = 0;
    init_compound_field(&field);

    defineField(&field, "count", "the number of data blocks", &offset, UDA_SCALAR_UINT);

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

    udaRegisterMalloc(plugin_interface, (void*)data, 1, sizeof(TEST41), "TEST41");
    udaRegisterMalloc(plugin_interface, (void*)blocks, request_block->putDataBlockList.blockCount, sizeof(TEST40),
                      "TEST40");

    data->count = request_block->putDataBlockList.blockCount;

    for (int i = 0; i < request_block->putDataBlockList.blockCount; i++) {
        blocks[i].dataCount = request_block->putDataBlockList.putDataBlock[i].count;
        blocks[i].data = (void*)request_block->putDataBlockList.putDataBlock[i].data;

        UDA_LOG(UDA_LOG_DEBUG, "data type : {}", request_block->putDataBlockList.putDataBlock[0].data_type);
        UDA_LOG(UDA_LOG_DEBUG, "data count: {}", request_block->putDataBlockList.putDataBlock[0].count);

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
    // Return an array of strings with all passed parameters and substitutions

    std::string work = "test50 passed parameters\n";

    int count = udaPluginArgumentCount(plugin_interface);

    work += fmt::format("Number of name-value pairs: {}\n", count);
    for (int i = 0; i < count; i++) {
        const char* name = udaPluginArgument(plugin_interface, i);
        const char* value;
        udaPluginFindStringArg(plugin_interface, &value, name);
        work += fmt::format("name: {}, value: {}\n", name, value);
    }

    udaPluginReturnDataStringScalar(plugin_interface, work.c_str(), nullptr);

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

int register_enumlist(UDA_PLUGIN_INTERFACE* plugin_interface, ENUMLIST60* enum_list)
{
    COMPOUNDFIELD* enummember60_fields[2] = {nullptr};
    int offset = 0;

    int shape[1] = {MAXELEMENTNAME};
    enummember60_fields[0] =
        udaNewCompoundField("name", "char array structure element", &offset, UDA_TYPE_CHAR, 1, shape);
    enummember60_fields[1] = udaNewCompoundField("value", "long long structure element", &offset, UDA_TYPE_LONG64, 0,
                                                 nullptr);

    USERDEFINEDTYPE* enummember60_type =
        udaNewUserType("ENUMMEMBER60", "Test #60", 0, 0, nullptr, sizeof(ENUMMEMBER60), 2, enummember60_fields);
    udaAddUserType(plugin_interface, enummember60_type);

    COMPOUNDFIELD* enumlist60_fields[8] = {nullptr};
    offset = 0;

    enumlist60_fields[0] =
        udaNewCompoundField("name", "char array structure element", &offset, UDA_TYPE_CHAR, 1, shape);
    enumlist60_fields[1] = udaNewCompoundField("type", "int structure element", &offset, UDA_TYPE_INT, 0, nullptr);
    enumlist60_fields[2] = udaNewCompoundField("count", "int structure element", &offset, UDA_TYPE_INT, 0, nullptr);
    enumlist60_fields[3] =
        udaNewCompoundUserTypePointerField("enummember", "ENUMMEMBER60* structure element", &offset, enummember60_type);

    enumlist60_fields[5] = udaNewCompoundField("arraydata_rank", "int structure element", &offset, UDA_TYPE_INT, 0,
                                               nullptr);
    enumlist60_fields[6] = udaNewCompoundField("arraydata_count", "int structure element", &offset, UDA_TYPE_INT, 0,
                                               nullptr);
    enumlist60_fields[7] = udaNewCompoundPointerField("arraydata_shape", "int* structure element", &offset, UDA_TYPE_INT, false);

    COMPOUNDFIELD* arraydata_field = nullptr;
    switch (enum_list->type) {
        case (UDA_TYPE_UNSIGNED_SHORT): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_UNSIGNED_SHORT, false);
            break;
        }
        case (UDA_TYPE_SHORT): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_SHORT, false);
            break;
        }
        case (UDA_TYPE_UNSIGNED_INT): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_UNSIGNED_INT, false);
            break;
        }
        case (UDA_TYPE_INT): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_INT, false);
            break;
        }
        case (UDA_TYPE_UNSIGNED_LONG64): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_UNSIGNED_LONG64, false);
            break;
        }
        case (UDA_TYPE_LONG64): {
            arraydata_field = udaNewCompoundPointerField("arraydata", "unsigned short* structure element", &offset, UDA_TYPE_LONG64, false);
            break;
        }
    }

    enumlist60_fields[4] = arraydata_field;

    USERDEFINEDTYPE* enumlist60_type =
        udaNewUserType("ENUMLIST60", "Test #60", 0, 0, nullptr, sizeof(ENUMLIST60), 2, enumlist60_fields);
    udaAddUserType(plugin_interface, enumlist60_type);

    return 0;
}

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
    udaRegisterMalloc(plugin_interface, (void*)enumlist, 1, sizeof(ENUMLIST60), "ENUMLIST60");
    udaRegisterMalloc(plugin_interface, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER60),
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

    udaRegisterMalloc(plugin_interface, (void*)enumlist->arraydata, count, sizeof(unsigned short), "unsigned short");

    count = 1;
    int rank = 1;
    int malloc_shape[] = {1}; // Shape of the shape array!
    udaRegisterMallocArray(plugin_interface, (void*)enumlist->arraydata_shape, count, sizeof(int), "int", rank,
                           malloc_shape);

    register_enumlist(plugin_interface, enumlist);

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "ENUMLIST60", "Test60 = ENUM Values");
    udaPluginReturnDataLabel(plugin_interface, "");

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
    udaRegisterMalloc(plugin_interface, (void*)enumlist, 1, sizeof(ENUMLIST60), "ENUMLIST60");
    udaRegisterMalloc(plugin_interface, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER60),
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

    udaRegisterMalloc(plugin_interface, (void*)enumlist->arraydata, count, sizeof(unsigned long long),
                      "unsigned long long");

    count = 1;
    int rank = 1;
    int shape[] = {1}; // Shape of the shape array!
    udaRegisterMallocArray(plugin_interface, (void*)enumlist->arraydata_shape, count, sizeof(int), "int", rank, shape);

    register_enumlist(plugin_interface, enumlist);

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "ENUMLIST60", "Test61 = ENUM Values");
    udaPluginReturnDataLabel(plugin_interface, "");

    return 0;
}

int TestPlugin::test62(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    auto* enumlist = (ENUMLIST60*)malloc(sizeof(ENUMLIST60));
    strcpy(enumlist->name, "TEST62 ENUM of type unsigned long long");
    enumlist->type = UDA_TYPE_UNSIGNED_SHORT;
    enumlist->count = 3;
    enumlist->enummember = (ENUMMEMBER60*)malloc(enumlist->count * sizeof(ENUMMEMBER60));
    strcpy(enumlist->enummember[0].name, "ENUM Value 1");
    strcpy(enumlist->enummember[1].name, "ENUM Value 2");
    strcpy(enumlist->enummember[2].name, "ENUM Value 3");
    enumlist->enummember[0].value = (long long)1;
    enumlist->enummember[1].value = (long long)2;
    enumlist->enummember[2].value = (long long)3;
    udaRegisterMalloc(plugin_interface, (void*)enumlist, 1, sizeof(ENUMLIST60), "ENUMLIST60");
    udaRegisterMalloc(plugin_interface, (void*)enumlist->enummember, enumlist->count, sizeof(ENUMMEMBER60),
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
    enumlist->arraydata = data;
    enumlist->arraydata_rank = 1;
    enumlist->arraydata_count = count;
    enumlist->arraydata_shape = (int*)malloc(sizeof(int));
    enumlist->arraydata_shape[0] = count;

    udaRegisterMalloc(plugin_interface, (void*)enumlist->arraydata, count, sizeof(unsigned long long),
                      "unsigned long long");

    // count = 1;
    // int rank = 1;
    // int shape[] = {1};        // Shape of the shape array!
    udaRegisterMalloc(plugin_interface, (void*)enumlist->arraydata_shape, 1, sizeof(int), "int");

    register_enumlist(plugin_interface, enumlist);

    udaPluginReturnCompoundData(plugin_interface, (char*)data, "ENUMLIST60", "Test62 = ENUM Values");
    udaPluginReturnDataLabel(plugin_interface, "");

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
    int err = 0;

    int plugin_count = udaPluginPluginsCount(plugin_interface);

    if (plugin_count == 0) {
        UDA_RAISE_PLUGIN_ERROR(plugin_interface, "No plugins available for this data request");
    }

    // Test specifics

    const char* signal = nullptr;
    const char* source = nullptr;

    UDA_FIND_REQUIRED_STRING_VALUE(plugin_interface, signal);
    UDA_FIND_REQUIRED_STRING_VALUE(plugin_interface, source);

    if (signal != nullptr || source != nullptr) { // Identify the plugin to test
        udaCallPlugin2(plugin_interface, signal, source);
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

    UDA_FIND_REQUIRED_INT_VALUE(plugin_interface, test);

    switch (test) {
        case 1:
            testError1(plugin_interface);
            return err;
        case 2:
            testError2(plugin_interface);
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

    UDA_RAISE_PLUGIN_ERROR(plugin_interface, "Test of Error State Management");
}

int TestPlugin::scalartest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    udaPluginReturnDataIntScalar(plugin_interface, 10, nullptr);

    return 0;
}

int TestPlugin::array1dtest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    constexpr int n = 100;
    auto data = std::make_unique<double[]>(n);
    for (int i = 0; i < n; ++i) {
        data[i] = i;
    }

    size_t shape[1] = {n};
    udaPluginReturnDataDoubleArray(plugin_interface, data.get(), 1, shape, nullptr);

    return 0;
}

int TestPlugin::emptytest(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return 0;
}

int TestPlugin::call_plugin_test(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaCallPlugin(plugin_interface, "TESTPLUGIN::array1dtest()");
}

int TestPlugin::call_plugin_test_index(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaCallPlugin(plugin_interface, "TESTPLUGIN::array1dtest()[25]");
}

int TestPlugin::call_plugin_test_slice(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaCallPlugin(plugin_interface, "TESTPLUGIN::array1dtest()[10:20]");
}

int TestPlugin::call_plugin_test_stride(UDA_PLUGIN_INTERFACE* plugin_interface)
{
    return udaCallPlugin(plugin_interface, "TESTPLUGIN::array1dtest()[::2]");
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

    udaPluginReturnData(plugin_interface, buffer.data, static_cast<int>(buffer.size), UDA_TYPE_CAPNP, 0, nullptr,
                        nullptr);

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

    udaPluginReturnData(plugin_interface, buffer.data, static_cast<int>(buffer.size), UDA_TYPE_CAPNP, 0, nullptr,
                        nullptr);

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

    udaPluginReturnData(plugin_interface, buffer.data, static_cast<int>(buffer.size), UDA_TYPE_CAPNP, 0, nullptr,
                        nullptr);

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

    udaPluginReturnData(plugin_interface, buffer.data, static_cast<int>(buffer.size), UDA_TYPE_CAPNP, 0, nullptr,
                        nullptr);

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
        addIdamError(ErrorType::Code, "testplugin:createUDTSocket", err, "Illegal port number or port is busy");
        addIdamError(ErrorType::Code, "testplugin:createUDTSocket", err, (char*)udt_getlasterror_desc());
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
          fprintf(stderr, "UDT bind: [{}]\n",udt_getlasterror_desc());
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
    sprintf(service, "{}", port);

    if (0 != getaddrinfo(nullptr, service, &hints, &res)) {
        int err = 999;
        addIdamError(ErrorType::Code, "testplugin:createTCPSocket", err, "Illegal port number or port is busy");
        addIdamError(ErrorType::Code, "testplugin:createTCPSocket", err, (char*)udt_getlasterror_desc());
        return -1;
    }

    *ssock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (bind(*ssock, res->ai_addr, res->ai_addrlen) != 0) {
        int err = 999;
        addIdamError(ErrorType::Code, "testplugin:createTCPSocket", err, "Socket Bind error");
        addIdamError(ErrorType::Code, "testplugin:createTCPSocket", err, (char*)udt_getlasterror_desc());
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
        addIdamError(ErrorType::Code, "testplugin:c_connect", err, "Socket Connect error");
        addIdamError(ErrorType::Code, "testplugin:c_connect", err, (char*)udt_getlasterror_desc());
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
        addIdamError(ErrorType::Code, "testplugin:tcp_connect", err, "Socket Connect error");
        addIdamError(ErrorType::Code, "testplugin:tcp_connect", err, (char*)udt_getlasterror_desc());
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
        addIdamError(ErrorType::Code, "testplugin:udt", err, "Unable to create a UDT Socket");
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
            addIdamError(ErrorType::Code, "testplugin:udt", err, "Unable to Send Data");
            addIdamError(ErrorType::Code, "testplugin:udt", err, (char*)udt_getlasterror_desc());
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

    init_data_block(data_block);

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
