#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <uda.h>
// #include <serialisation/capnp_serialisation.h>

#include "test_config.hpp"

using Catch::Matchers::StartsWith;
using Catch::Approx;

TEST_CASE( "Test help function", "[plugins][TESTPLUGIN]" )
{
    udaLoadConfig(ConfigFilePath);
    int handle = udaGetAPI("TESTPLUGIN::help()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );
    
    const char* data = udaGetData(handle);
    int data_type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( data_type == UDA_TYPE_STRING );

    std::string expected = "TESTPLUGIN: Plugin which generates test data that is used in UDA tests\n\n"
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
            "\ttest50: Passing parameters into plugins via the souhandlee argument\n"
            "\ttest60-62: ENUMLIST structures\n\n"

            "plugin: test calling other plugins\n\n"

            "error: Error reporting and server termination tests";

    std::string start = "TESTPLUGIN: Plugin which generates test data that is used in UDA tests\n\n";
    std::string end = "error: Error reporting and server termination tests";

    std::string string = data;
    REQUIRE( string.substr(0, start.size()) == start );
    REQUIRE( string.substr(string.size() - end.size()) == end );

    auto num_lines = std::count(string.begin(), string.end(), '\n');
    REQUIRE( num_lines == 46 );
}

TEST_CASE( "Run test0 - pass string as char array", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test0()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    int data_type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( data_type == UDA_TYPE_CHAR );

    int rank = udaGetRank(handle);
    REQUIRE( rank == 1 );

    std::string str = data;
    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test1 - pass string as string scalar", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test1()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    int data_type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( data_type == UDA_TYPE_STRING );

    int rank = udaGetRank(handle);
    REQUIRE( rank == 1 );

    std::string str = data;
    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test2 - pass string list as 2D char array", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test2()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    int data_type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( data_type == UDA_TYPE_CHAR );

    int rank = udaGetRank(handle);
    REQUIRE( rank == 2 );

    int dim0 = udaGetDimNum(handle, 0);
    int dim1 = udaGetDimNum(handle, 1);

    REQUIRE( dim0 == 16 );
    REQUIRE( dim1 == 3 );

    std::vector<std::string> strings;
    strings.emplace_back(std::string{ data + 0 * dim0 });
    strings.emplace_back(std::string{ data + 1 * dim0 });
    strings.emplace_back(std::string{ data + 2 * dim0 });

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test3 - pass string list as array of strings", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test3()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    int data_type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( data_type == UDA_TYPE_STRING );

    int rank = udaGetRank(handle);
    REQUIRE( rank == 2 );

    int dim0 = udaGetDimNum(handle, 0);
    int dim1 = udaGetDimNum(handle, 1);

    REQUIRE( dim0 == 16 );
    REQUIRE( dim1 == 3 );

    std::vector<std::string> strings;
    strings.emplace_back(std::string{ data + 0 * dim0 });
    strings.emplace_back(std::string{ data + 1 * dim0 });
    strings.emplace_back(std::string{ data + 2 * dim0 });

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test4 - pass struct containing char array", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test4()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);
    int** atomic_shape = udaGetNodeAtomicShape(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "STRING" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shape[0][0] == 56 );

    auto* data = static_cast<const char*>(udaGetNodeStructureComponentData(scalar, "value"));

    std::string str{ data };
    REQUIRE( str == "012345678901234567890" );
}

TEST_CASE( "Run test5 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test5()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);
    
    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "STRING" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 2 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 56 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shape = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "STRING" );
    REQUIRE( atomic_rank[0] == 2 );
    REQUIRE( atomic_shape[0][0] == 56 );
    REQUIRE( atomic_shape[0][1] == 3 );

    auto* data = static_cast<const char*>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( std::string{ &data[0 * 56] } == "012345678901234567890" );
    REQUIRE( std::string{ &data[1 * 56] } == "QWERTY KEYBOARD" );
    REQUIRE( std::string{ &data[2 * 56] } == "MAST TOKAMAK" );
}

TEST_CASE( "Run test6 - pass struct containing string", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test6()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);
    
    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "STRING" );
    REQUIRE( atomic_rank[0] == 0 );

    auto* data = static_cast<const char*>(udaGetNodeStructureComponentData(scalar, "value"));

    REQUIRE( std::string{ data } == "PI=3.1415927" );
}

TEST_CASE( "Run test7 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test7()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "STRING *" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shape = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "STRING *" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shape[0][0] == 3 );

    const auto data = static_cast<char* const* const>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( std::string{ data[0] } == "012345678901234567890" );
    REQUIRE( std::string{ data[1] } == "QWERTY KEYBOARD" );
    REQUIRE( std::string{ data[2] } == "MAST TOKAMAK" );
}

TEST_CASE( "Run test8 - pass struct containing array of string pointers", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test8()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "STRING *" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "STRING *" );
    REQUIRE( atomic_rank[0] == 0 );

    const auto data = static_cast<char* const* const>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( std::string{ data[0] } == "012345678901234567890" );
    REQUIRE( std::string{ data[1] } == "QWERTY KEYBOARD" );
    REQUIRE( std::string{ data[2] } == "MAST TOKAMAK" );
}

TEST_CASE( "Run test9 - pass 4 structs containing multiple types of string arrays", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test9()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 4 );

    for (int i = 0; i < 4; ++i)
    {
        auto* child = udaGetNodeChild(node, i);

        REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
        REQUIRE( udaGetNodeChildrenCount(child) == 0 );
        REQUIRE( udaGetNodeAtomicCount(child) == 5 );

        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "v1" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[1] } == "v2" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[2] } == "v3" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[3] } == "v4" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[4] } == "v5" );

        REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
        REQUIRE( udaGetNodeAtomicPointers(child)[1] == false );
        REQUIRE( udaGetNodeAtomicPointers(child)[2] == true );
        REQUIRE( udaGetNodeAtomicPointers(child)[3] == false );
        REQUIRE( udaGetNodeAtomicPointers(child)[4] == true );

        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "STRING" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[1] } == "STRING" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[2] } == "STRING" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[3] } == "STRING *" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[4] } == "STRING *" );

        REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );
        REQUIRE( udaGetNodeAtomicRank(child)[1] == 2 );
        REQUIRE( udaGetNodeAtomicRank(child)[2] == 0 );
        REQUIRE( udaGetNodeAtomicRank(child)[3] == 1 );
        REQUIRE( udaGetNodeAtomicRank(child)[4] == 0 );

        // v1
        {
            auto* scalar = udaFindNTreeStructureComponent(child, "v1");

            char** atomic_names = udaGetNodeAtomicNames(scalar);
            char** atomic_types = udaGetNodeAtomicTypes(scalar);
            int* atomic_rank = udaGetNodeAtomicRank(scalar);
            int** atomic_shape = udaGetNodeAtomicShape(scalar);

            REQUIRE( std::string{ atomic_names[0] } == "v1" );
            REQUIRE( std::string{ atomic_types[0] } == "STRING" );
            REQUIRE( atomic_rank[0] == 1 );
            REQUIRE( atomic_shape[0][0] == 56 );

            auto* data = static_cast<const char*>(udaGetNodeStructureComponentData(scalar, "v1"));

            REQUIRE( std::string{ data } == "123212321232123212321" );
        }

        // v2
        {
            auto* vector = udaFindNTreeStructureComponent(child, "v2");

            char** atomic_names = udaGetNodeAtomicNames(vector);
            char** atomic_types = udaGetNodeAtomicTypes(vector);
            int* atomic_rank = udaGetNodeAtomicRank(vector);
            int** atomic_shape = udaGetNodeAtomicShape(vector);

            REQUIRE( std::string{ atomic_names[1] } == "v2" );
            REQUIRE( std::string{ atomic_types[1] } == "STRING" );
            REQUIRE( atomic_rank[1] == 2 );
            REQUIRE( atomic_shape[1][0] == 56 );
            REQUIRE( atomic_shape[1][1] == 3 );

            auto* data = static_cast<char* const>(udaGetNodeStructureComponentData(vector, "v2"));

            REQUIRE( std::string{ &data[0 * 56] } == "012345678901234567890" );
            REQUIRE( std::string{ &data[1 * 56] } == "QWERTY KEYBOARD" );
            REQUIRE( std::string{ &data[2 * 56] } == "MAST TOKAMAK" );
        }

        // v3
        {
            auto* scalar = udaFindNTreeStructureComponent(child, "v3");

            char** atomic_names = udaGetNodeAtomicNames(scalar);
            char** atomic_types = udaGetNodeAtomicTypes(scalar);
            int* atomic_rank = udaGetNodeAtomicRank(scalar);

            REQUIRE( std::string{ atomic_names[2] } == "v3" );
            REQUIRE( std::string{ atomic_types[2] } == "STRING" );
            REQUIRE( atomic_rank[2] == 0 );

            auto* data = static_cast<const char*>(udaGetNodeStructureComponentData(scalar, "v3"));

            REQUIRE( std::string{ data } == "PI=3.1415927" );
        }

        // v4
        {
            auto* vector = udaFindNTreeStructureComponent(child, "v4");

            char** atomic_names = udaGetNodeAtomicNames(vector);
            char** atomic_types = udaGetNodeAtomicTypes(vector);
            int* atomic_rank = udaGetNodeAtomicRank(vector);
            int** atomic_shape = udaGetNodeAtomicShape(vector);

            REQUIRE( std::string{ atomic_names[3] } == "v4" );
            REQUIRE( std::string{ atomic_types[3] } == "STRING *" );
            REQUIRE( atomic_rank[3] == 1 );
            REQUIRE( atomic_shape[3][0] == 3 );

            auto* data = static_cast<char* const* const>(udaGetNodeStructureComponentData(vector, "v4"));

            REQUIRE( std::string{ data[0] } == "012345678901234567890" );
            REQUIRE( std::string{ data[1] } == "QWERTY KEYBOARD" );
            REQUIRE( std::string{ data[2] } == "MAST TOKAMAK" );
        }

        // v5
        {
            auto* vector = udaFindNTreeStructureComponent(child, "v5");

            char** atomic_names = udaGetNodeAtomicNames(vector);
            char** atomic_types = udaGetNodeAtomicTypes(vector);
            int* atomic_rank = udaGetNodeAtomicRank(vector);

            REQUIRE( std::string{ atomic_names[4] } == "v5" );
            REQUIRE( std::string{ atomic_types[4] } == "STRING *" );
            REQUIRE( atomic_rank[4] == 0 );

            auto* data = static_cast<char* const* const>(udaGetNodeStructureComponentData(vector, "v5"));

            REQUIRE( std::string{ data[0] } == "012345678901234567890" );
            REQUIRE( std::string{ data[1] } == "QWERTY KEYBOARD" );
            REQUIRE( std::string{ data[2] } == "MAST TOKAMAK" );
        }
    }
}

TEST_CASE( "Run test10 - pass single int", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test10()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    const auto* scalar = reinterpret_cast<int*>(udaGetData(handle));
    REQUIRE( scalar[0] == 7 );
}

TEST_CASE( "Run test11 - pass struct containing single int", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test11()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 0 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(scalar, "value"));

    REQUIRE( *data == 11 );
}

TEST_CASE( "Run test12 - pass struct containing 1D array of ints", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test12()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shape = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shape[0][0] == 3 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( data[0] == 10 );
    REQUIRE( data[1] == 11 );
    REQUIRE( data[2] == 12 );
}

TEST_CASE( "Run test13 - pass struct containing 2D array of ints", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test13()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 2 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 2 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 3 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shape = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 2 );
    REQUIRE( atomic_shape[0][0] == 2 );
    REQUIRE( atomic_shape[0][1] == 3 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[2] == 2 );
    REQUIRE( data[3] == 10 );
    REQUIRE( data[4] == 11 );
    REQUIRE( data[5] == 12 );
}

TEST_CASE( "Run test14 - pass struct containing single int passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test14()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 0 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(scalar, "value"));

    REQUIRE( *data == 14 );
}

TEST_CASE( "Run test15 - pass struct containing 1D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test15()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shapes = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shapes[0][0] == 3 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( data[0] == 13 );
    REQUIRE( data[1] == 14 );
    REQUIRE( data[2] == 15 );
}

TEST_CASE( "Run test16 - pass struct containing 2D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test16()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(2);
    expected_shape.push_back(3);
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 2 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 3 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shapes = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "int" );
    REQUIRE( atomic_rank[0] == 2 );
    REQUIRE( atomic_shapes[0][0] == 2 );
    REQUIRE( atomic_shapes[0][1] == 3 );

    auto* data = static_cast<int*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[2] == 2 );
    REQUIRE( data[3] == 10 );
    REQUIRE( data[4] == 11 );
    REQUIRE( data[5] == 12 );
}

TEST_CASE( "Run test18 - pass large number of structs containing single int", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test18()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 100000 );

    for (int i = 0; i < 100000; ++i) {
        auto* child = udaGetNodeChild(node, i);

        REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
        REQUIRE( udaGetNodeChildrenCount(child) == 0 );
        REQUIRE( udaGetNodeAtomicCount(child) == 1 );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
        REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
        REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

        auto* scalar = udaFindNTreeStructureComponent(child, "value");

        char** atomic_names = udaGetNodeAtomicNames(scalar);
        char** atomic_types = udaGetNodeAtomicTypes(scalar);
        int* atomic_rank = udaGetNodeAtomicRank(scalar);

        REQUIRE( std::string{ atomic_names[0] } == "value" );
        REQUIRE( std::string{ atomic_types[0] } == "int" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<int*>(udaGetNodeStructureComponentData(scalar, "value"));

        REQUIRE( *data == i );
    }
}

TEST_CASE( "Run test19 - pass 3 structs containing array of structs", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test19()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 3 );

    for (int i = 0; i < 3; ++i) {
        auto* child = udaGetNodeChild(node, i);

        REQUIRE(std::string{ udaGetNodeStructureName(child) } == "data");
        REQUIRE(udaGetNodeChildrenCount(child) == 7);
        REQUIRE(udaGetNodeAtomicCount(child) == 1);
        REQUIRE(std::string{ udaGetNodeAtomicNames(child)[0] } == "value");
        REQUIRE(udaGetNodeAtomicPointers(child)[0] == false);
        REQUIRE(std::string{ udaGetNodeAtomicTypes(child)[0] } == "int");
        REQUIRE(udaGetNodeAtomicRank(child)[0] == 0);

        auto* scalar = udaFindNTreeStructureComponent(child, "value");

        char** atomic_names = udaGetNodeAtomicNames(scalar);
        char** atomic_types = udaGetNodeAtomicTypes(scalar);
        int* atomic_rank = udaGetNodeAtomicRank(scalar);

        REQUIRE( std::string{ atomic_names[0] } == "value" );
        REQUIRE( std::string{ atomic_types[0] } == "int" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<int*>(udaGetNodeStructureComponentData(scalar, "value"));

        REQUIRE( *data == 3 + i );

        for (int j = 0; j < 7; ++j) {
            auto* sub_child = udaGetNodeChild(child, j);

            REQUIRE( std::string{ udaGetNodeStructureName(sub_child) } == "vals" );
            REQUIRE( udaGetNodeChildrenCount(sub_child) == 0 );
            REQUIRE( udaGetNodeAtomicCount(sub_child) == 1 );
            REQUIRE( std::string{ udaGetNodeAtomicNames(sub_child)[0] } == "value" );
            REQUIRE( udaGetNodeAtomicPointers(sub_child)[0] == false );
            REQUIRE( std::string{ udaGetNodeAtomicTypes(sub_child)[0] } == "int" );
            REQUIRE( udaGetNodeAtomicRank(sub_child)[0] == 0 );

            auto* sub_scalar = udaFindNTreeStructureComponent(sub_child, "value");

            atomic_names = udaGetNodeAtomicNames(sub_scalar);
            atomic_types = udaGetNodeAtomicTypes(sub_scalar);
            atomic_rank = udaGetNodeAtomicRank(sub_scalar);

            REQUIRE( std::string{ atomic_names[0] } == "value" );
            REQUIRE( std::string{ atomic_types[0] } == "int" );
            REQUIRE( atomic_rank[0] == 0 );

            data = static_cast<int*>(udaGetNodeStructureComponentData(sub_scalar, "value"));

            REQUIRE( *data == 10 * i + j );
        }
    }
}

TEST_CASE( "Run test20 - pass single short", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test20()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    REQUIRE( udaGetRank(handle) == 0 );

    const auto* data = reinterpret_cast<short*>(udaGetData(handle));
    REQUIRE( *data == 7 );
}

TEST_CASE( "Run test21 - pass struct containing single short", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test21()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 0 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(scalar, "value"));

    REQUIRE( *data == 21 );
}

TEST_CASE( "Run test22 - pass struct containing 1D array of shorts", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test22()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );;
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shape = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shape[0][0] == 3 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( data[0] == 20 );
    REQUIRE( data[1] == 21 );
    REQUIRE( data[2] == 22 );
}

TEST_CASE( "Run test23 - pass struct containing 2D array of shorts", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test23()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 2 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 2 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shape = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 2 );
    REQUIRE( atomic_shape[0][0] == 3 );
    REQUIRE( atomic_shape[0][1] == 2 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[2] == 2 );
    REQUIRE( data[3] == 10 );
    REQUIRE( data[4] == 11 );
    REQUIRE( data[5] == 12 );
}

TEST_CASE( "Run test24 - pass struct containing single short passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test24()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    auto* scalar = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(scalar);
    char** atomic_types = udaGetNodeAtomicTypes(scalar);
    int* atomic_rank = udaGetNodeAtomicRank(scalar);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 0 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(scalar, "value"));

    REQUIRE( *data == 24 );
}

TEST_CASE( "Run test25 - pass struct containing 1D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test25()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 1 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );

    auto* vector = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(vector);
    char** atomic_types = udaGetNodeAtomicTypes(vector);
    int* atomic_rank = udaGetNodeAtomicRank(vector);
    int** atomic_shape = udaGetNodeAtomicShape(vector);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 1 );
    REQUIRE( atomic_shape[0][0] == 3 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(vector, "value"));

    REQUIRE( data[0] == 13 );
    REQUIRE( data[1] == 14 );
    REQUIRE( data[2] == 15 );
}

TEST_CASE( "Run test26 - pass struct containing 2D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test26()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 2 );
    std::vector<size_t> expected_shape;
    expected_shape.push_back(3);
    expected_shape.push_back(2);
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 2 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shape = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 2 );
    REQUIRE( atomic_shape[0][0] == 3 );
    REQUIRE( atomic_shape[0][1] == 2 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 13 );
    REQUIRE( data[1] == 14 );
    REQUIRE( data[2] == 15 );
    REQUIRE( data[3] == 23 );
    REQUIRE( data[4] == 24 );
    REQUIRE( data[5] == 25 );
}

TEST_CASE( "Run test27 - pass struct containing 3D array of shorts", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test27()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 4 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][2] == 2 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shape = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 3 );
    REQUIRE( atomic_shape[0][0] == 4 );
    REQUIRE( atomic_shape[0][1] == 3 );
    REQUIRE( atomic_shape[0][2] == 2 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[2] == 2 );
    REQUIRE( data[3] == 3 );
    REQUIRE( data[4] == 10 );
    REQUIRE( data[5] == 11 );
    REQUIRE( data[6] == 12 );
    REQUIRE( data[7] == 13 );
    REQUIRE( data[8] == 20 );
    REQUIRE( data[9] == 21 );
    REQUIRE( data[10] == 22 );
    REQUIRE( data[11] == 23 );
    REQUIRE( data[12] == 100 );
    REQUIRE( data[13] == 101 );
    REQUIRE( data[14] == 102 );
    REQUIRE( data[15] == 103 );
    REQUIRE( data[16] == 110 );
    REQUIRE( data[17] == 111 );
    REQUIRE( data[18] == 112 );
    REQUIRE( data[19] == 113 );
    REQUIRE( data[20] == 120 );
    REQUIRE( data[21] == 121 );
    REQUIRE( data[22] == 122 );
    REQUIRE( data[23] == 123 );
}

TEST_CASE( "Run test28 - pass struct containing 3D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test28()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "value" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == true );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "short" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][0] == 4 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][1] == 3 );
    REQUIRE( udaGetNodeAtomicShape(child)[0][2] == 2 );

    auto* array = udaFindNTreeStructureComponent(child, "value");

    char** atomic_names = udaGetNodeAtomicNames(array);
    char** atomic_types = udaGetNodeAtomicTypes(array);
    int* atomic_rank = udaGetNodeAtomicRank(array);
    int** atomic_shape = udaGetNodeAtomicShape(array);

    REQUIRE( std::string{ atomic_names[0] } == "value" );
    REQUIRE( std::string{ atomic_types[0] } == "short" );
    REQUIRE( atomic_rank[0] == 3 );
    REQUIRE( atomic_shape[0][0] == 4 );
    REQUIRE( atomic_shape[0][1] == 3 );
    REQUIRE( atomic_shape[0][2] == 2 );

    auto* data = static_cast<short*>(udaGetNodeStructureComponentData(array, "value"));

    REQUIRE( data[0] == 0 );
    REQUIRE( data[1] == 1 );
    REQUIRE( data[2] == 2 );
    REQUIRE( data[3] == 3 );
    REQUIRE( data[4] == 10 );
    REQUIRE( data[5] == 11 );
    REQUIRE( data[6] == 12 );
    REQUIRE( data[7] == 13 );
    REQUIRE( data[8] == 20 );
    REQUIRE( data[9] == 21 );
    REQUIRE( data[10] == 22 );
    REQUIRE( data[11] == 23 );
    REQUIRE( data[12] == 100 );
    REQUIRE( data[13] == 101 );
    REQUIRE( data[14] == 102 );
    REQUIRE( data[15] == 103 );
    REQUIRE( data[16] == 110 );
    REQUIRE( data[17] == 111 );
    REQUIRE( data[18] == 112 );
    REQUIRE( data[19] == 113 );
    REQUIRE( data[20] == 120 );
    REQUIRE( data[21] == 121 );
    REQUIRE( data[22] == 122 );
    REQUIRE( data[23] == 123 );
}

TEST_CASE( "Run test30 - pass struct containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test30()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 0 );
    REQUIRE( udaGetNodeAtomicCount(child) == 2 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "R" );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[1] } == "Z" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( udaGetNodeAtomicPointers(child)[1] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "double" );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[1] } == "double" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );
    REQUIRE( udaGetNodeAtomicRank(child)[1] == 0 );

    {
        auto* R = udaFindNTreeStructureComponent(child, "R");

        char** atomic_names = udaGetNodeAtomicNames(R);
        char** atomic_types = udaGetNodeAtomicTypes(R);
        int* atomic_rank = udaGetNodeAtomicRank(R);

        REQUIRE( std::string{ atomic_names[0] } == "R" );
        REQUIRE( std::string{ atomic_types[0] } == "double" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<double*>(udaGetNodeStructureComponentData(R, "R"));

        REQUIRE( *data == Approx(1.0) );
    }

    {
        auto* Z = udaFindNTreeStructureComponent(child, "Z");

        char** atomic_names = udaGetNodeAtomicNames(Z);
        char** atomic_types = udaGetNodeAtomicTypes(Z);
        int* atomic_rank = udaGetNodeAtomicRank(Z);

        REQUIRE( std::string{ atomic_names[1] } == "Z" );
        REQUIRE( std::string{ atomic_types[1] } == "double" );
        REQUIRE( atomic_rank[1] == 0 );

        auto* data = static_cast<double*>(udaGetNodeStructureComponentData(Z, "Z"));

        REQUIRE( *data == Approx(2.0) );
    }
}

TEST_CASE( "Run test31 - pass 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test31()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 100 );

    for (int i = 0; i < 100; ++i) {
        auto* child = udaGetNodeChild(node, i);

        REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
        REQUIRE( udaGetNodeChildrenCount(child) == 0 );
        REQUIRE( udaGetNodeAtomicCount(child) == 2 );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "R" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(child)[1] } == "Z" );
        REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
        REQUIRE( udaGetNodeAtomicPointers(child)[1] == false );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "double" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[1] } == "double" );
        REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );
        REQUIRE( udaGetNodeAtomicRank(child)[1] == 0 );

        {
            auto* R = udaFindNTreeStructureComponent(child, "R");

            char** atomic_names = udaGetNodeAtomicNames(R);
            char** atomic_types = udaGetNodeAtomicTypes(R);
            int* atomic_rank = udaGetNodeAtomicRank(R);

            REQUIRE( std::string{ atomic_names[0] } == "R" );
            REQUIRE( std::string{ atomic_types[0] } == "double" );
            REQUIRE( atomic_rank[0] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(R, "R"));

            REQUIRE( *data == Approx(1.0 * i) );
        }

        {
            auto* Z = udaFindNTreeStructureComponent(child, "Z");

            char** atomic_names = udaGetNodeAtomicNames(Z);
            char** atomic_types = udaGetNodeAtomicTypes(Z);
            int* atomic_rank = udaGetNodeAtomicRank(Z);

            REQUIRE( std::string{ atomic_names[1] } == "Z" );
            REQUIRE( std::string{ atomic_types[1] } == "double" );
            REQUIRE( atomic_rank[1] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(Z, "Z"));

            REQUIRE( *data == Approx(10.0 * i) );
        }
    }
}

TEST_CASE( "Run test32 - pass struct containing array of 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test32()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 100 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "count" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    {
        auto* count = udaFindNTreeStructureComponent(child, "count");

        char** atomic_names = udaGetNodeAtomicNames(count);
        char** atomic_types = udaGetNodeAtomicTypes(count);
        int* atomic_rank = udaGetNodeAtomicRank(count);

        REQUIRE( std::string{ atomic_names[0] } == "count" );
        REQUIRE( std::string{ atomic_types[0] } == "int" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<int*>(udaGetNodeStructureComponentData(count, "count"));

        REQUIRE( *data == 100 );
    }

    for (int i = 0; i < 100; ++i) {
        auto* coord = udaGetNodeChild(child, i);

        REQUIRE( std::string{ udaGetNodeStructureName(coord) } == "coords" );
        REQUIRE( udaGetNodeChildrenCount(coord) == 0 );
        REQUIRE( udaGetNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[0] } == "R" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[1] } == "Z" );
        REQUIRE( udaGetNodeAtomicPointers(coord)[0] == false );
        REQUIRE( udaGetNodeAtomicPointers(coord)[1] == false );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[0] } == "double" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[1] } == "double" );
        REQUIRE( udaGetNodeAtomicRank(coord)[0] == 0 );
        REQUIRE( udaGetNodeAtomicRank(coord)[1] == 0 );

        {
            auto* R = udaFindNTreeStructureComponent(coord, "R");

            char** atomic_names = udaGetNodeAtomicNames(R);
            char** atomic_types = udaGetNodeAtomicTypes(R);
            int* atomic_rank = udaGetNodeAtomicRank(R);

            REQUIRE( std::string{ atomic_names[0] } == "R" );
            REQUIRE( std::string{ atomic_types[0] } == "double" );
            REQUIRE( atomic_rank[0] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(R, "R"));

            REQUIRE( *data == Approx((double)(1.0 * i)) );
        }

        {
            auto* Z = udaFindNTreeStructureComponent(coord, "Z");

            char** atomic_names = udaGetNodeAtomicNames(Z);
            char** atomic_types = udaGetNodeAtomicTypes(Z);
            int* atomic_rank = udaGetNodeAtomicRank(Z);

            REQUIRE( std::string{ atomic_names[1] } == "Z" );
            REQUIRE( std::string{ atomic_types[1] } == "double" );
            REQUIRE( atomic_rank[1] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(Z, "Z"));

            REQUIRE( *data == Approx((double)(10.0 * i)) );
        }
    }
}

TEST_CASE( "Run test33 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test33()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 100 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "count" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    {
        auto* count = udaFindNTreeStructureComponent(child, "count");

        char** atomic_names = udaGetNodeAtomicNames(count);
        char** atomic_types = udaGetNodeAtomicTypes(count);
        int* atomic_rank = udaGetNodeAtomicRank(count);

        REQUIRE( std::string{ atomic_names[0] } == "count" );
        REQUIRE( std::string{ atomic_types[0] } == "int" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<int*>(udaGetNodeStructureComponentData(count, "count"));

        REQUIRE( *data == 100 );
    }

    for (int i = 0; i < 100; ++i) {
        auto coord = udaGetNodeChild(child, i);

        REQUIRE( std::string{ udaGetNodeStructureName(coord) } == "coords" );
        REQUIRE( udaGetNodeChildrenCount(coord) == 0 );
        REQUIRE( udaGetNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[0] } == "R" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[1] } == "Z" );
        REQUIRE( udaGetNodeAtomicPointers(coord)[0] == false );
        REQUIRE( udaGetNodeAtomicPointers(coord)[1] == false );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[0] } == "double" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[1] } == "double" );
        REQUIRE( udaGetNodeAtomicRank(coord)[0] == 0 );
        REQUIRE( udaGetNodeAtomicRank(coord)[1] == 0 );

        {
            auto* R = udaFindNTreeStructureComponent(coord, "R");

            char** atomic_names = udaGetNodeAtomicNames(R);
            char** atomic_types = udaGetNodeAtomicTypes(R);
            int* atomic_rank = udaGetNodeAtomicRank(R);

            REQUIRE( std::string{ atomic_names[0] } == "R" );
            REQUIRE( std::string{ atomic_types[0] } == "double" );
            REQUIRE( atomic_rank[0] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(R, "R"));

            REQUIRE( *data == Approx((double)(1.0 * i)) );
        }

        {
            auto* Z = udaFindNTreeStructureComponent(coord, "Z");

            char** atomic_names = udaGetNodeAtomicNames(Z);
            char** atomic_types = udaGetNodeAtomicTypes(Z);
            int* atomic_rank = udaGetNodeAtomicRank(Z);

            REQUIRE( std::string{ atomic_names[1] } == "Z" );
            REQUIRE( std::string{ atomic_types[1] } == "double" );
            REQUIRE( atomic_rank[1] == 0 );

            auto* data = static_cast<double*>(udaGetNodeStructureComponentData(Z, "Z"));

            REQUIRE( *data == Approx((double)(10.0 * i)) );
        }
    }
}

TEST_CASE( "Run test34 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::test34()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    bool is_tree = udaSetDataTree(handle) != 0;
    REQUIRE( is_tree );

    auto* node = udaGetDataTree(handle);

    int num_children = udaGetNodeChildrenCount(node);
    REQUIRE( num_children == 1 );

    auto* child = udaGetNodeChild(node, 0);

    REQUIRE( std::string{ udaGetNodeStructureName(child) } == "data" );
    REQUIRE( udaGetNodeChildrenCount(child) == 100 );
    REQUIRE( udaGetNodeAtomicCount(child) == 1 );
    REQUIRE( std::string{ udaGetNodeAtomicNames(child)[0] } == "count" );
    REQUIRE( udaGetNodeAtomicPointers(child)[0] == false );
    REQUIRE( std::string{ udaGetNodeAtomicTypes(child)[0] } == "int" );
    REQUIRE( udaGetNodeAtomicRank(child)[0] == 0 );

    {
        auto* count = udaFindNTreeStructureComponent(child, "count");

        char** atomic_names = udaGetNodeAtomicNames(count);
        char** atomic_types = udaGetNodeAtomicTypes(count);
        int* atomic_rank = udaGetNodeAtomicRank(count);

        REQUIRE( std::string{ atomic_names[0] } == "count" );
        REQUIRE( std::string{ atomic_types[0] } == "int" );
        REQUIRE( atomic_rank[0] == 0 );

        auto* data = static_cast<int*>(udaGetNodeStructureComponentData(count, "count"));

        REQUIRE( *data == 100 );
    }

    for (int i = 0; i < 100; ++i) {
        auto* coord = udaGetNodeChild(child, i);

        REQUIRE( std::string{ udaGetNodeStructureName(coord) } == "coords" );
        REQUIRE( udaGetNodeChildrenCount(coord) == 0 );
        REQUIRE( udaGetNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[0] } == "R" );
        REQUIRE( std::string{ udaGetNodeAtomicNames(coord)[1] } == "Z" );
        REQUIRE( udaGetNodeAtomicPointers(coord)[0] == true );
        REQUIRE( udaGetNodeAtomicPointers(coord)[1] == true );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[0] } == "unsigned char *" );
        REQUIRE( std::string{ udaGetNodeAtomicTypes(coord)[1] } == "unsigned char *" );
        REQUIRE( udaGetNodeAtomicRank(coord)[0] == 0 );
        REQUIRE( udaGetNodeAtomicRank(coord)[1] == 0 );

        {
            auto* R = udaFindNTreeStructureComponent(coord, "R");

            char** atomic_names = udaGetNodeAtomicNames(R);
            char** atomic_types = udaGetNodeAtomicTypes(R);
            int* atomic_rank = udaGetNodeAtomicRank(R);

            REQUIRE( std::string{ atomic_names[0] } == "R" );
            REQUIRE( std::string{ atomic_types[0] } == "unsigned char *" );
            REQUIRE( atomic_rank[0] == 0 );

            auto* data = static_cast<unsigned char*>(udaGetNodeStructureComponentData(R, "R"));

            REQUIRE( data[0] == (unsigned char)(1 * i) );
        }

        {
            auto* Z = udaFindNTreeStructureComponent(coord, "Z");

            char** atomic_names = udaGetNodeAtomicNames(Z);
            char** atomic_types = udaGetNodeAtomicTypes(Z);
            int* atomic_rank = udaGetNodeAtomicRank(Z);

            REQUIRE( std::string{ atomic_names[1] } == "Z" );
            REQUIRE( std::string{ atomic_types[1] } == "unsigned char *" );
            REQUIRE( atomic_rank[1] == 0 );

            auto* data = static_cast<unsigned char*>(udaGetNodeStructureComponentData(Z, "Z"));

            REQUIRE( data[0] == (unsigned char)(10 * i) );
        }
    }
}

TEST_CASE( "Run plugin - call a plugin", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::plugin(signal='HELP::HELP()', source='')", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    std::string str{ data };

    std::string expected =
            "HELP: Plugin to provide server help and available services\n"
            "\n"
            "Functions:\n"
            "\n"
            "services()      Returns a list of available services with descriptions\n"
            "ping()          Return the Local Server Time in seconds and microseonds\n"
            "servertime()    Return the Local Server Time in seconds and microseonds\n";

    REQUIRE( str == expected );
}

// TEST_CASE( "Run errortest - test error reporting", "[plugins][TESTPLUGIN]" )
// {
//     REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=1)", ""), StartsWith("Test #1 of Error State Management") );
//     REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=2)", ""), StartsWith("Test #2 of Error State Management") );
//
// #ifndef FATCLIENT
//     // This test hard crashes the server code so can't be run in fat-client mode
// //    REQUIRE_THROWS_WITH( client.get("TESTPLUGIN::errortest(test=3)", ""), StartsWith("No Data waiting at Socket when Data Expected!") );
// #endif
// }

TEST_CASE( "Run scalartest - return a simple scalar value", "[plugins][TESTPLUGIN]" )
{
    const int handle = udaGetAPI("TESTPLUGIN::scalartest()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    auto* value = reinterpret_cast<const int*>(data);
    REQUIRE( *value == 10 );
}

TEST_CASE( "Run array1dtest - return a simple 1d array value", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::array1dtest()", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 100 );

    auto* vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - take first 10 values", "[plugins][TESTPLUGIN]" )
{
//    int handle = udaGetAPI("SS::Subset(\"TESTPLUGIN::array1dtest()\", [0:10])", "");
    int handle = udaGetAPI("TESTPLUGIN::array1dtest()[0:10]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 10 );

    auto vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - take last 10 values", "[plugins][TESTPLUGIN]" )
{
//    int handle = udaGetAPI("SS::Subset(\"TESTPLUGIN::array1dtest()\", [-10:])", "");
    int handle = udaGetAPI("TESTPLUGIN::array1dtest()[-10:]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 10 );

    auto vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(90.0) );
    REQUIRE( vec[1] == Approx(91.0) );
    REQUIRE( vec[2] == Approx(92.0) );
    REQUIRE( vec[3] == Approx(93.0) );
    REQUIRE( vec[4] == Approx(94.0) );
}

TEST_CASE( "Test array subsetting - take every 5th value", "[plugins][TESTPLUGIN]" )
{
//    int handle = udaGetAPI("SS::Subset(\"TESTPLUGIN::array1dtest()\", [::5])", "");
    int handle = udaGetAPI("TESTPLUGIN::array1dtest()[::5]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 20 );

    auto vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(5.0) );
    REQUIRE( vec[2] == Approx(10.0) );
    REQUIRE( vec[3] == Approx(15.0) );
    REQUIRE( vec[4] == Approx(20.0) );
}

TEST_CASE( "Test array subsetting - reverse elements", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::array1dtest()[::-1]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 100 );

    auto vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(99.0) );
    REQUIRE( vec[1] == Approx(98.0) );
    REQUIRE( vec[2] == Approx(97.0) );
    REQUIRE( vec[3] == Approx(96.0) );
    REQUIRE( vec[4] == Approx(95.0) );
}

TEST_CASE( "Test array subsetting with argument with square brackets", "[plugins][TESTPLUGIN]" )
{
    int handle = udaGetAPI("TESTPLUGIN::array1dtest(args=[0;0])[0:10]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    REQUIRE( udaGetDataNum(handle) == 10 );

    auto vec = reinterpret_cast<const double*>(data);
    REQUIRE( vec[0] == Approx(0.0) );
    REQUIRE( vec[1] == Approx(1.0) );
    REQUIRE( vec[2] == Approx(2.0) );
    REQUIRE( vec[3] == Approx(3.0) );
    REQUIRE( vec[4] == Approx(4.0) );
}

TEST_CASE( "Test array subsetting - index scalar with [0]", "[plugins][TESTPLUGIN]" )
{
    const int handle = udaGetAPI("TESTPLUGIN::test10()[0]", "");

    REQUIRE( handle >= 0 );
    REQUIRE( udaGetErrorCode(handle) == 0 );

    const char* data = udaGetData(handle);
    REQUIRE( data != nullptr );

    const auto value = reinterpret_cast<const int*>(data);
    REQUIRE( *value == 7 );
}

// TEST_CASE( "Run call_plugin_test - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
// {
//     int handle = udaGetAPI("TESTPLUGIN::call_plugin_test()", "");
//
//     REQUIRE( handle >= 0 );
//     REQUIRE( udaGetErrorCode(handle) == 0 );
//
//     const char* data = udaGetData(handle);
//
//     REQUIRE( data != nullptr );
//     REQUIRE( !data->isNull() );
//     REQUIRE( data_type == typeid(double).name() );
//
//     auto* array = dynamic_cast<uda::Array*>(data);
//
//     REQUIRE( array != nullptr );
//
//     auto vec = array->as<double>();
//
//     REQUIRE( vec.size() == 100 );
//     REQUIRE( vec[0] == Approx(0.0) );
//     REQUIRE( vec[1] == Approx(1.0) );
//     REQUIRE( vec[2] == Approx(2.0) );
//     REQUIRE( vec[3] == Approx(3.0) );
//     REQUIRE( vec[4] == Approx(4.0) );
// }
//
// TEST_CASE( "Run call_plugin_test_index - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
// {
//     int handle = udaGetAPI("TESTPLUGIN::call_plugin_test_index()", "");
//
//     REQUIRE( handle >= 0 );
//     REQUIRE( udaGetErrorCode(handle) == 0 );
//
//     const char* data = udaGetData(handle);
//
//     REQUIRE( data != nullptr );
//     REQUIRE( !data->isNull() );
//     REQUIRE( data_type == typeid(double).name() );
//
//     auto* array = dynamic_cast<uda::Array*>(data);
//
//     REQUIRE( array != nullptr );
//
//     auto vec = array->as<double>();
//
//     REQUIRE( vec.size() == 1 );
//     REQUIRE( vec[0] == Approx(25.0) );
// }
//
// TEST_CASE( "Run call_plugin_test_slice - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
// {
//     int handle = udaGetAPI("TESTPLUGIN::call_plugin_test_slice()", "");
//
//     REQUIRE( handle >= 0 );
//     REQUIRE( udaGetErrorCode(handle) == 0 );
//
//     const char* data = udaGetData(handle);
//
//     REQUIRE( data != nullptr );
//     REQUIRE( !data->isNull() );
//     REQUIRE( data_type == typeid(double).name() );
//
//     auto* array = dynamic_cast<uda::Array*>(data);
//
//     REQUIRE( array != nullptr );
//
//     auto vec = array->as<double>();
//
//     REQUIRE( vec.size() == 10 );
//     REQUIRE( vec[0] == Approx(10.0) );
//     REQUIRE( vec[1] == Approx(11.0) );
//     REQUIRE( vec[2] == Approx(12.0) );
//     REQUIRE( vec[3] == Approx(13.0) );
//     REQUIRE( vec[4] == Approx(14.0) );
// }
//
// // TODO: stride seems to be broken
// //TEST_CASE( "Run call_plugin_test_stride - return the result of calling a plugin", "[plugins][TESTPLUGIN]" )
// //{
// ////
// //    uda::Client client;
// //
// //    int handle = udaGetAPI("TESTPLUGIN::call_plugin_test_stide()", "");
// //
// //    REQUIRE( handle >= 0 );
// //    REQUIRE( udaGetErrorCode(handle) == 0 );
// //
// //    const char* data = udaGetData(handle);
// //
// //    REQUIRE( data != nullptr );
// //    REQUIRE( !data->isNull() );
// //    REQUIRE( data_type == typeid(double).name() );
// //
// //    auto* array = dynamic_cast<uda::Array*>(data);
// //
// //    REQUIRE( array != nullptr );
// //
// //    auto vec = array->as<double>();
// //
// //    REQUIRE( vec.size() == 50 );
// //    REQUIRE( vec[0] == Approx(0.0) );
// //    REQUIRE( vec[1] == Approx(2.0) );
// //    REQUIRE( vec[2] == Approx(4.0) );
// //    REQUIRE( vec[3] == Approx(6.0) );
// //    REQUIRE( vec[4] == Approx(8.0) );
// //}
//
// TEST_CASE( "Run emptytest - return no data", "[plugins][TESTPLUGIN]" )
// {
//     int handle = udaGetAPI("TESTPLUGIN::emptytest()", "");
//
//     REQUIRE( handle >= 0 );
//     REQUIRE( udaGetErrorCode(handle) == 0 );
//
//     const char* data = udaGetData(handle);
//
//     REQUIRE( data != nullptr );
//     REQUIRE( data->isNull() );
// }
//
// #ifdef CAPNP_ENABLED
// TEST_CASE( "Test capnp serialisation", "[plugins][TESTPLUGIN]" )
// {
//
//     int handle = udaGetAPI("TESTPLUGIN::capnp_test()", "");
//     REQUIRE( handle >= 0 );
//
//     int ec = udaGetErrohandleode(handle);
//     REQUIRE( ec == 0 );
//
//     const char* error = udaGetErrorMsg(handle);
//     std::string error_string = error == nullptr ? "" : error;
//     REQUIRE( error_string.empty() );
//
//     auto data_type = udaGetDataType(handle);
//     REQUIRE( data_type == UDA_TYPE_CAPNP );
//
//     auto data = udaGetData(handle);
//     REQUIRE( data != nullptr );
//
//     auto data_n = udaGetDataNum(handle);
//     REQUIRE( data_n >= 0 );
//
//     auto tree = uda_capnp_deserialise(data, data_n);
//
// //    uda_capnp_print_tree_reader(tree);
//
//     auto root = uda_capnp_read_root(tree);
//     auto node = uda_capnp_read_child(tree, root, "double_array");
//     REQUIRE( node != nullptr );
//
//     auto maybe_rank = uda_capnp_read_rank(node);
//     REQUIRE( maybe_rank.has_value );
//     REQUIRE( maybe_rank.value == 1 );
//
//     auto rank = maybe_rank.value;
//
//     size_t shape[1];
//     bool ok = uda_capnp_read_shape(node, shape);
//     REQUIRE( ok );
//     REQUIRE( shape[0] == 30 );
//
//     size_t num_slices = uda_capnp_read_num_slices(node);
//     REQUIRE( num_slices == 1 );
//
//     double array[30];
//     ok = uda_capnp_read_data(node, 0, reinterpret_cast<char*>(&array));
//     REQUIRE( ok );
//
//     REQUIRE( array[0] == Approx(0.0) );
//     REQUIRE( array[10] == Approx(1.0) );
//     REQUIRE( array[29] == Approx(2.9) );
// }
// #endif // CAPNP_ENABLED