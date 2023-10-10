#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>
#include <serialisation/capnp_serialisation.h>

TEST_CASE( "Test help function", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

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
            "\ttest50: Passing parameters into plugins via the source argument\n"
            "\ttest60-62: ENUMLIST structures\n\n"

            "plugin: test calling other plugins\n"

            "error: Error reporting and server termination tests\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Run test0 - pass string as char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test0()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    std::string str = array->as<char>().data();

    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test1 - pass string as string scalar", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test1()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected = "Hello World!";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Run test2 - pass string list as 2D char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test2()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    std::vector<uda::Dim> dims = array->dims();

    REQUIRE( dims.size() == 2 );

    REQUIRE( dims[0].size() == 16 );
    REQUIRE( dims[1].size() == 3 );

    std::vector<char> vec = array->as<char>();

    std::vector<std::string> strings;
    strings.emplace_back(std::string(vec.data() + 0 * dims[0].size(), strlen(vec.data() + 0 * dims[0].size())));
    strings.emplace_back(std::string(vec.data() + 1 * dims[0].size(), strlen(vec.data() + 1 * dims[0].size())));
    strings.emplace_back(std::string(vec.data() + 2 * dims[0].size(), strlen(vec.data() + 2 * dims[0].size())));

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test3 - pass string list as array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test3()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( std::string(data->type().name()) == typeid(std::string).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE(array != nullptr );

    std::vector<uda::Dim> dims = array->dims();

    REQUIRE( dims.size() == 1 );

    REQUIRE( dims[0].size() == 3 );

    std::vector<std::string> strings = array->as<std::string>();

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test4 - pass struct containing char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test4()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
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

TEST_CASE( "Run test5 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test5()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "STRING" );
    REQUIRE( child.atomicRank()[0] == 2 );
    std::vector<size_t> shape;
    shape.push_back(56);
    shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test6 - pass struct containing string", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test6()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
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
    REQUIRE( str == "PI=3.1415927" );
}

TEST_CASE( "Run test7 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test7()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "STRING *" );
    REQUIRE( child.atomicRank()[0] == 1 );
    std::vector<size_t> shape;
    shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test8 - pass struct containing array of string pointers", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test8()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "STRING *" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test9 - pass 4 structs containing multiple types of string arrays", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test9()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 4 );

    for (int i = 0; i < 4; ++i)
    {
        uda::TreeNode child = tree.child(i);

        REQUIRE( child.name() == "data" );
        REQUIRE( child.numChildren() == 0 );
        REQUIRE( child.atomicCount() == 5 );

        std::vector<std::string> expected_names;
        expected_names.emplace_back("v1");
        expected_names.emplace_back("v2");
        expected_names.emplace_back("v3");
        expected_names.emplace_back("v4");
        expected_names.emplace_back("v5");
        REQUIRE( child.atomicNames() == expected_names );

        std::vector<bool> expected_ptrs;
        expected_ptrs.push_back(false);
        expected_ptrs.push_back(false);
        expected_ptrs.push_back(true);
        expected_ptrs.push_back(false);
        expected_ptrs.push_back(true);
        REQUIRE( child.atomicPointers() == expected_ptrs );

        std::vector<std::string> expected_types;
        expected_types.emplace_back("STRING");
        expected_types.emplace_back("STRING");
        expected_types.emplace_back("STRING");
        expected_types.emplace_back("STRING *");
        expected_types.emplace_back("STRING *");
        REQUIRE( child.atomicTypes() == expected_types );

        std::vector<size_t> expected_ranks;
        expected_ranks.push_back(1);
        expected_ranks.push_back(2);
        expected_ranks.push_back(0);
        expected_ranks.push_back(1);
        expected_ranks.push_back(0);
        REQUIRE( child.atomicRank() == expected_ranks );

        // v1
        {
            uda::Scalar value = child.atomicScalar("v1");
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( std::string(value.as<char*>()) == "123212321232123212321" );
        }

        // v2
        {
            uda::Vector value = child.atomicVector("v2");
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }

        // v3
        {
            uda::Scalar value = child.atomicScalar("v3");
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( std::string(value.as<char*>()) == "PI=3.1415927" );
        }

        // v4
        {
            uda::Vector value = child.atomicVector("v4");
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }

        // v5
        {
            uda::Vector value = child.atomicVector("v5");
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }
    }
}

TEST_CASE( "Run test10 - pass single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test10()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto* value = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( value != nullptr );

    REQUIRE( value->as<int>() == 7 );
}

TEST_CASE( "Run test11 - pass struct containing single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test11()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<int>() == 11 );
}

TEST_CASE( "Run test12 - pass struct containing 1D array of ints", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test12()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 1 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 3 );

    std::vector<int> expected;
    expected.push_back(10);
    expected.push_back(11);
    expected.push_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test13 - pass struct containing 2D array of ints", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test13()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(2);
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 2 );
    REQUIRE( value.dims()[1].size() == 3 );

    std::vector<int> expected;
    expected.push_back(0);
    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(10);
    expected.push_back(11);
    expected.push_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test14 - pass struct containing single int passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test14()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<int>() == 14 );
}

TEST_CASE( "Run test15 - pass struct containing 1D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test15()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 1 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 3 );

    std::vector<int> expected;
    expected.push_back(13);
    expected.push_back(14);
    expected.push_back(15);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test16 - pass struct containing 2D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test16()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(2);
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 2 );
    REQUIRE( value.dims()[1].size() == 3 );

    std::vector<int> expected;
    expected.push_back(0);
    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(10);
    expected.push_back(11);
    expected.push_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test18 - pass large number of structs containing single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test18()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 100000 );

    for (int i = 0; i < 100000; ++i) {
        uda::TreeNode child = tree.child(i);

        REQUIRE( child.name() == "data" );
        REQUIRE( child.numChildren() == 0 );
        REQUIRE( child.atomicCount() == 1 );
        REQUIRE( child.atomicNames()[0] == "value" );
        REQUIRE( child.atomicPointers()[0] == false );
        REQUIRE( child.atomicTypes()[0] == "int" );
        REQUIRE( child.atomicRank()[0] == 0 );

        uda::Scalar value = child.atomicScalar("value");
        REQUIRE( !value.isNull() );

        REQUIRE( value.type().name() == typeid(int).name() );
        REQUIRE( value.size() == 0 );

        REQUIRE( value.as<int>() == i );
    }
}

TEST_CASE( "Run test19 - pass 3 structs containing array of structs", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test19()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 3 );

    for (int i = 0; i < 3; ++i) {
        uda::TreeNode child = tree.child(i);

        REQUIRE(child.name() == "data");
        REQUIRE(child.numChildren() == 7);
        REQUIRE(child.atomicCount() == 1);
        REQUIRE(child.atomicNames()[0] == "value");
        REQUIRE(child.atomicPointers()[0] == false);
        REQUIRE(child.atomicTypes()[0] == "int");
        REQUIRE(child.atomicRank()[0] == 0);

        uda::Scalar value = child.atomicScalar("value");
        REQUIRE(!value.isNull());

        REQUIRE(value.type().name() == typeid(int).name());
        REQUIRE(value.size() == 0);

        REQUIRE(value.as<int>() == 3 + i);

        for (int j = 0; j < 7; ++j) {
            uda::TreeNode subchild = child.child(j);

            REQUIRE(subchild.name() == "vals");
            REQUIRE(subchild.numChildren() == 0);
            REQUIRE(subchild.atomicCount() == 1);
            REQUIRE(subchild.atomicNames()[0] == "value");
            REQUIRE(subchild.atomicPointers()[0] == false);
            REQUIRE(subchild.atomicTypes()[0] == "int");
            REQUIRE(subchild.atomicRank()[0] == 0);

            value = subchild.atomicScalar("value");
            REQUIRE(!value.isNull());

            REQUIRE(value.type().name() == typeid(int).name());
            REQUIRE(value.size() == 0);

            REQUIRE(value.as<int>() == 10 * i + j);
        }
    }
}

TEST_CASE( "Run test20 - pass single short", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test20()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(short).name() );

    auto* value = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( value != nullptr );

    REQUIRE( value->as<short>() == 7 );
}

TEST_CASE( "Run test21 - pass struct containing single short", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test21()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<short>() == 21 );
}

TEST_CASE( "Run test22 - pass struct containing 1D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test22()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 1 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 3 );

    std::vector<short> expected;
    expected.push_back(20);
    expected.push_back(21);
    expected.push_back(22);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test23 - pass struct containing 2D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test23()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    expected_shape.push_back(2);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 3 );
    REQUIRE( value.dims()[1].size() == 2 );

    std::vector<short> expected;
    expected.push_back(0);
    expected.push_back(1);
    expected.push_back(2);
    expected.push_back(10);
    expected.push_back(11);
    expected.push_back(12);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test24 - pass struct containing single short passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test24()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar value = child.atomicScalar("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<short>() == 24 );
}

TEST_CASE( "Run test25 - pass struct containing 1D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test25()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 1 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Vector value = child.atomicVector("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 3 );

    std::vector<short> expected;
    expected.push_back(13);
    expected.push_back(14);
    expected.push_back(15);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test26 - pass struct containing 2D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test26()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    expected_shape.push_back(2);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 3 );
    REQUIRE( value.dims()[1].size() == 2 );

    std::vector<short> expected;
    expected.push_back(13);
    expected.push_back(14);
    expected.push_back(15);
    expected.push_back(23);
    expected.push_back(24);
    expected.push_back(25);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test27 - pass struct containing 3D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test27()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 3 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(4);
    expected_shape.push_back(3);
    expected_shape.push_back(2);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 24 );

    REQUIRE( value.dims().size() == 3 );
    REQUIRE( value.dims()[0].size() == 4 );
    REQUIRE( value.dims()[1].size() == 3 );
    REQUIRE( value.dims()[2].size() == 2 );

    short exp[] = { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 100, 101, 102, 103, 110, 111,
                    112, 113, 120, 121, 122, 123 };
    std::vector<short> expected(exp, exp + sizeof(exp)/sizeof(exp[0]));

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test28 - pass struct containing 3D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test28()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "value" );
    REQUIRE( child.atomicPointers()[0] == true );
    REQUIRE( child.atomicTypes()[0] == "short" );
    REQUIRE( child.atomicRank()[0] == 3 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(4);
    expected_shape.push_back(3);
    expected_shape.push_back(2);
    REQUIRE( child.atomicShape()[0] == expected_shape );

    uda::Array value = child.atomicArray("value");
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 24 );

    REQUIRE( value.dims().size() == 3 );
    REQUIRE( value.dims()[0].size() == 4 );
    REQUIRE( value.dims()[1].size() == 3 );
    REQUIRE( value.dims()[2].size() == 2 );

    short exp[] = { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 100, 101, 102, 103, 110, 111,
                    112, 113, 120, 121, 122, 123 };
    std::vector<short> expected(exp, exp + sizeof(exp)/sizeof(exp[0]));

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test30 - pass struct containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test30()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 2 );
    REQUIRE( child.atomicNames()[0] == "R" );
    REQUIRE( child.atomicNames()[1] == "Z" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicPointers()[1] == false );
    REQUIRE( child.atomicTypes()[0] == "double" );
    REQUIRE( child.atomicTypes()[1] == "double" );
    REQUIRE( child.atomicRank()[0] == 0 );
    REQUIRE( child.atomicRank()[1] == 0 );

    uda::Scalar R = child.atomicScalar("R");
    REQUIRE( !R.isNull() );

    REQUIRE( R.type().name() == typeid(double).name() );
    REQUIRE( R.as<double>() == Approx(1.0) );

    uda::Scalar Z = child.atomicScalar("Z");
    REQUIRE( !Z.isNull() );

    REQUIRE( Z.type().name() == typeid(double).name() );
    REQUIRE( Z.as<double>() == Approx(2.0) );
}

TEST_CASE( "Run test31 - pass 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test31()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 100 );

    for (int i = 0; i < 100; ++i) {
        uda::TreeNode child = tree.child(i);

        REQUIRE( child.name() == "data" );
        REQUIRE( child.numChildren() == 0 );
        REQUIRE( child.atomicCount() == 2 );
        REQUIRE( child.atomicNames()[0] == "R" );
        REQUIRE( child.atomicNames()[1] == "Z" );
        REQUIRE( child.atomicPointers()[0] == false );
        REQUIRE( child.atomicPointers()[1] == false );
        REQUIRE( child.atomicTypes()[0] == "double" );
        REQUIRE( child.atomicTypes()[1] == "double" );
        REQUIRE( child.atomicRank()[0] == 0 );
        REQUIRE( child.atomicRank()[1] == 0 );

        uda::Scalar R = child.atomicScalar("R");
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = child.atomicScalar("Z");
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test32 - pass struct containing array of 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test32()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 100 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "count" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar count = child.atomicScalar("count");
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        uda::TreeNode coord = child.child(i);

        REQUIRE( coord.name() == "coords" );
        REQUIRE( coord.numChildren() == 0 );
        REQUIRE( coord.atomicCount() == 2 );
        REQUIRE( coord.atomicNames()[0] == "R" );
        REQUIRE( coord.atomicNames()[1] == "Z" );
        REQUIRE( coord.atomicPointers()[0] == false );
        REQUIRE( coord.atomicPointers()[1] == false );
        REQUIRE( coord.atomicTypes()[0] == "double" );
        REQUIRE( coord.atomicTypes()[1] == "double" );
        REQUIRE( coord.atomicRank()[0] == 0 );
        REQUIRE( coord.atomicRank()[1] == 0 );

        uda::Scalar R = coord.atomicScalar("R");
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = coord.atomicScalar("Z");
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test33 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test33()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 100 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "count" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar count = child.atomicScalar("count");
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        uda::TreeNode coord = child.child(i);

        REQUIRE( coord.name() == "coords" );
        REQUIRE( coord.numChildren() == 0 );
        REQUIRE( coord.atomicCount() == 2 );
        REQUIRE( coord.atomicNames()[0] == "R" );
        REQUIRE( coord.atomicNames()[1] == "Z" );
        REQUIRE( coord.atomicPointers()[0] == false );
        REQUIRE( coord.atomicPointers()[1] == false );
        REQUIRE( coord.atomicTypes()[0] == "double" );
        REQUIRE( coord.atomicTypes()[1] == "double" );
        REQUIRE( coord.atomicRank()[0] == 0 );
        REQUIRE( coord.atomicRank()[1] == 0 );

        uda::Scalar R = coord.atomicScalar("R");
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = coord.atomicScalar("Z");
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test34 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test34()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 100 );
    REQUIRE( child.atomicCount() == 1 );
    REQUIRE( child.atomicNames()[0] == "count" );
    REQUIRE( child.atomicPointers()[0] == false );
    REQUIRE( child.atomicTypes()[0] == "int" );
    REQUIRE( child.atomicRank()[0] == 0 );

    uda::Scalar count = child.atomicScalar("count");
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        uda::TreeNode coord = child.child(i);

        REQUIRE( coord.name() == "coords" );
        REQUIRE( coord.numChildren() == 0 );
        REQUIRE( coord.atomicCount() == 2 );
        REQUIRE( coord.atomicNames()[0] == "R" );
        REQUIRE( coord.atomicNames()[1] == "Z" );
        REQUIRE( coord.atomicPointers()[0] == true );
        REQUIRE( coord.atomicPointers()[1] == true );
        REQUIRE( coord.atomicTypes()[0] == "unsigned char *" );
        REQUIRE( coord.atomicTypes()[1] == "unsigned char *" );
        REQUIRE( coord.atomicRank()[0] == 0 );
        REQUIRE( coord.atomicRank()[1] == 0 );
        REQUIRE( coord.atomicRank()[0] == 0 );
        REQUIRE( coord.atomicRank()[1] == 0 );
        std::vector<size_t> exp_shape = { 10 };
        REQUIRE( coord.atomicShape()[0] == exp_shape );
        REQUIRE( coord.atomicShape()[1] == exp_shape );

        uda::Vector R = coord.atomicVector("R");
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(unsigned char).name() );
//        std::vector<char> exp = { 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i };
        unsigned char val = 1 * i;
        std::vector<unsigned char> exp = { val, val, val, val, val, val, val, val, val, val };
        REQUIRE( R.as<unsigned char>() == exp );

        uda::Vector Z = coord.atomicVector("Z");
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(unsigned char).name() );
//        exp = { 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i };
        val = 10 * i;
        exp = { val, val, val, val, val, val, val, val, val, val };
        REQUIRE( Z.as<unsigned char>() == exp );
    }
}

TEST_CASE( "Run plugin - call a plugin", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::plugin(signal='HELP::HELP()', source='')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != nullptr );

    std::string expected = "\n"
            "Help\tList of HELP plugin functions:\n"
            "\n"
            "services()\tReturns a list of available services with descriptions\n"
            "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
            "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Run errortest - test error reporting", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=1)", ""), "[testplugin]: Test #1 of Error State Management\n" );
    REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=2)", ""), "[testplugin]: Test #2 of Error State Management\n" );
#ifndef FATCLIENT
    // This test hard crashes the server code so can't be run in fat-client mode
    REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=3)", ""), "[idamClient]:  Protocol 11 Error (Server Block #2)\n" );
#endif
}

TEST_CASE( "Run scalartest - return a simple scalar value", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::scalartest()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto* value = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( value != nullptr );

    REQUIRE( value->as<int>() == 10 );
}

TEST_CASE( "Run array1dtest - return a simple 1d array value", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::array1dtest()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 100 );
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - take first 10 values", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

//    const uda::Result& result = client.get("SS::SUBSET(\"TESTPLUGIN::array1dtest()\", [0:10])", "");
    const uda::Result& result = client.get("TESTPLUGIN::array1dtest()[0:10]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 10 );
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - take last 10 values", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

//    const uda::Result& result = client.get("SS::SUBSET(\"TESTPLUGIN::array1dtest()\", [-10:])", "");
    const uda::Result& result = client.get("TESTPLUGIN::array1dtest()[-10:]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 10 );
    REQUIRE( vec[0] == Approx(90.0) );
    REQUIRE( vec[1] == Approx(91.0) );
    REQUIRE( vec[2] == Approx(92.0) );
    REQUIRE( vec[3] == Approx(93.0) );
    REQUIRE( vec[4] == Approx(94.0) );
}

TEST_CASE( "Test array subsetting - take every 5th value", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

//    const uda::Result& result = client.get("SS::SUBSET(\"TESTPLUGIN::array1dtest()\", [::5])", "");
    const uda::Result& result = client.get("TESTPLUGIN::array1dtest()[::5]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 20 );
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(5.0) );
    REQUIRE( vec[2] == Approx(10.0) );
    REQUIRE( vec[3] == Approx(15.0) );
    REQUIRE( vec[4] == Approx(20.0) );
}

TEST_CASE( "Test array subsetting - reverse elements", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::array1dtest()[::-1]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 100 );
    REQUIRE( vec[0] == Approx(99.0) );
    REQUIRE( vec[1] == Approx(98.0) );
    REQUIRE( vec[2] == Approx(97.0) );
    REQUIRE( vec[3] == Approx(96.0) );
    REQUIRE( vec[4] == Approx(95.0) );
}

TEST_CASE( "Test array subsetting with argument with square brackets", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::array1dtest(args=[0;0])[0:10]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 10 );
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - index scalar with [0]", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::test10()[0]", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(int).name() );

    auto* value = dynamic_cast<uda::Scalar*>(data);

    REQUIRE( value != nullptr );

    REQUIRE( value->as<int>() == 7 );
}

TEST_CASE( "Run call_plugin_test - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::call_plugin_test()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 100 );
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Run call_plugin_test_index - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::call_plugin_test_index()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 1 );
    REQUIRE( vec[0] == Approx(25.0) );
}

TEST_CASE( "Run call_plugin_test_slice - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::call_plugin_test_slice()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(double).name() );

    auto* array = dynamic_cast<uda::Array*>(data);

    REQUIRE( array != nullptr );

    auto vec = array->as<double>();

    REQUIRE( vec.size() == 10 );
    REQUIRE( vec[0] == Approx(10.0) );
    REQUIRE( vec[1] == Approx(11.0) );
    REQUIRE( vec[2] == Approx(12.0) );
    REQUIRE( vec[3] == Approx(13.0) );
    REQUIRE( vec[4] == Approx(14.0) );
}

// TODO: stride seems to be broken
//TEST_CASE( "Run call_plugin_test_stride - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
//{
//#include "setup.inc"
//
//    uda::Client client;
//
//    const uda::Result& result = client.get("TESTPLUGIN::call_plugin_test_stide()", "");
//
//    REQUIRE( result.errorCode() == 0 );
//    REQUIRE( result.errorMessage().empty() );
//
//    uda::Data* data = result.data();
//
//    REQUIRE( data != nullptr );
//    REQUIRE( !data->isNull() );
//    REQUIRE( data->type().name() == typeid(double).name() );
//
//    auto* array = dynamic_cast<uda::Array*>(data);
//
//    REQUIRE( array != nullptr );
//
//    auto vec = array->as<double>();
//
//    REQUIRE( vec.size() == 50 );
//    REQUIRE( vec[0] == Approx(0.0) );
//    REQUIRE( vec[1] == Approx(2.0) );
//    REQUIRE( vec[2] == Approx(4.0) );
//    REQUIRE( vec[3] == Approx(6.0) );
//    REQUIRE( vec[4] == Approx(8.0) );
//}

TEST_CASE( "Run emptytest - return no data", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("TESTPLUGIN::emptytest()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != nullptr );
    REQUIRE( data->isNull() );
}

#ifdef CAPNP_ENABLED
TEST_CASE( "Test capnp serialisation", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = idamGetAPI("TESTPLUGIN::capnp()", "");
    REQUIRE( handle >= 0 );

    int ec = getIdamErrorCode(handle);
    REQUIRE( ec == 0 );

    const char* error = getIdamErrorMsg(handle);
    std::string error_string = error == nullptr ? "" : error;
    REQUIRE( error_string.empty() );

    auto data_type = getIdamDataType(handle);
    REQUIRE( data_type == UDA_TYPE_CAPNP );

    auto data = getIdamData(handle);
    REQUIRE( data != nullptr );

    auto data_n = getIdamDataNum(handle);
    REQUIRE( data_n >= 0 );

    auto tree = uda_capnp_deserialise(data, data_n);

//    uda_capnp_print_tree_reader(tree);

    auto root = uda_capnp_read_root(tree);
    auto node = uda_capnp_read_child(tree, root, "double_array");
    REQUIRE( node != nullptr );

    auto maybe_rank = uda_capnp_read_rank(node);
    REQUIRE( maybe_rank.has_value );
    REQUIRE( maybe_rank.value == 1 );

    auto rank = maybe_rank.value;

    size_t shape[1];
    bool ok = uda_capnp_read_shape(node, shape);
    REQUIRE( ok );
    REQUIRE( shape[0] == 30 );

    double array[30];
    ok = uda_capnp_read_data(node, reinterpret_cast<char*>(&array));
    REQUIRE( ok );

    REQUIRE( array[0] == Approx(0.0) );
    REQUIRE( array[10] == Approx(1.0) );
    REQUIRE( array[29] == Approx(2.9) );
}
#endif // CAPNP_ENABLED