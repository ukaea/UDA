#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test help function", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nTestplugin: Functions Names and Test Descriptions/n/n"
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

            "error: Error reporting and server termination tests\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Run test0 - pass string as char array", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test0()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    uda::Array* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != NULL );

    std::string str = array->as<char>().data();

    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test1 - pass string as string scalar", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test1()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "Hello World!";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Run test2 - pass string list as 2D char array", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test2()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    uda::Array* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != NULL );

    std::vector<uda::Dim> dims = array->dims();

    REQUIRE( dims.size() == 2 );

    REQUIRE( dims[0].size() == 16 );
    REQUIRE( dims[1].size() == 3 );

    std::vector<char> vec = array->as<char>();

    std::vector<std::string> strings;
    strings.push_back(std::string(vec.data() + 0 * dims[0].size(), strlen(vec.data() + 0 * dims[0].size())));
    strings.push_back(std::string(vec.data() + 1 * dims[0].size(), strlen(vec.data() + 1 * dims[0].size())));
    strings.push_back(std::string(vec.data() + 2 * dims[0].size(), strlen(vec.data() + 2 * dims[0].size())));

    std::vector<std::string> expected;
    expected.push_back("Hello World!");
    expected.push_back("Qwerty keyboard");
    expected.push_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test3 - pass string list as array of strings", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT

#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test3()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(std::string).name() );

    uda::Array* array = dynamic_cast<uda::Array*>(data);

    REQUIRE(array != NULL );

    std::vector<uda::Dim> dims = array->dims();

    REQUIRE( dims.size() == 1 );

    REQUIRE( dims[0].size() == 3 );

    std::vector<std::string> strings = array->as<std::string>();

    std::vector<std::string> expected;
    expected.push_back("Hello World!");
    expected.push_back("Qwerty keyboard");
    expected.push_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test4 - pass struct containing char array", "[plugins][TESTPLUGIN]" )
{

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test4()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    child.printUserDefinedTypeTable();
    child.printStructureNames();

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );

    uda::Scalar value = child.atomicScalar("value");

    REQUIRE( value.type().name() == typeid(char*).name() );

    std::string str(value.as<char*>());
    REQUIRE( str == "012345678901234567890" );
}