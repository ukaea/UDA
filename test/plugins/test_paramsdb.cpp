#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

#define ARR_LEN(NAME) sizeof(NAME) / sizeof(NAME[0])
#define ARR2VEC(TYPE, NAME) std::vector<TYPE>(&NAME[0], &NAME[ARR_LEN(NAME)])

TEST_CASE( "Test help function", "[plugins][PARAMSDB]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nParamsDB: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test getActiveLimit function with system, subtype and coil", "[plugins][PARAMSDB][getActiveLimit]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip', coil='TFP1')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode system = tree.child(0);

    REQUIRE( system.name() == "data" );
    REQUIRE( system.numChildren() == 1 );
    REQUIRE( system.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_subtypes" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( system.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( system.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( system.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( system.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = system.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "RTP" );

        uda::Scalar num_subtypes = system.atomicScalar("num_subtypes");
        REQUIRE( !num_subtypes.isNull() );
        REQUIRE( num_subtypes.type().name() == typeid(int).name() );
        REQUIRE( num_subtypes.as<int>() == 1 );
    }

    uda::TreeNode subtype = system.child(0);

    REQUIRE( subtype.name() == "subtypes" );
    REQUIRE( subtype.numChildren() == 1 );
    REQUIRE( subtype.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_coils" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( subtype.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( subtype.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( subtype.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( subtype.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = subtype.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "Current_Threshold_Trip" );

        uda::Scalar num_coils = subtype.atomicScalar("num_coils");
        REQUIRE( !num_coils.isNull() );
        REQUIRE( num_coils.type().name() == typeid(int).name() );
        REQUIRE( num_coils.as<int>() == 1 );
    }

    uda::TreeNode coil = subtype.child(0);

    REQUIRE( coil.name() == "coils" );
    REQUIRE( coil.numChildren() == 0 );
    REQUIRE( coil.atomicCount() == 3 );

    {
        std::string exp_names[] = { "name", "upper_lower", "value" };
        bool exp_pointers[] = { true, true, false };
        std::string exp_types[] = { "STRING", "STRING", "double" };
        size_t exp_ranks[] = { 0, 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar upper_lower = coil.atomicScalar("upper_lower");
        REQUIRE( !upper_lower.isNull() );
        REQUIRE( upper_lower.type().name() == typeid(char*).name() );
        REQUIRE( std::string(upper_lower.as<char*>()) == "ONLY" );

        uda::Scalar value = coil.atomicScalar("value");
        REQUIRE( !value.isNull() );
        REQUIRE( value.type().name() == typeid(double).name() );
        REQUIRE( value.as<double>() == Approx(1.23) );
    }
}

TEST_CASE( "Test getActiveLimit function with system and subtype", "[plugins][PARAMSDB][getActiveLimit]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode system = tree.child(0);

    REQUIRE( system.name() == "data" );
    REQUIRE( system.numChildren() == 1 );
    REQUIRE( system.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_subtypes" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( system.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( system.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( system.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( system.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = system.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "RTP" );

        uda::Scalar num_subtypes = system.atomicScalar("num_subtypes");
        REQUIRE( !num_subtypes.isNull() );
        REQUIRE( num_subtypes.type().name() == typeid(int).name() );
        REQUIRE( num_subtypes.as<int>() == 1 );
    }

    uda::TreeNode subtype = system.child(0);

    REQUIRE( subtype.name() == "subtypes" );
    REQUIRE( subtype.numChildren() == 2 );
    REQUIRE( subtype.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_coils" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( subtype.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( subtype.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( subtype.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( subtype.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = subtype.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "Current_Threshold_Trip" );

        uda::Scalar num_coils = subtype.atomicScalar("num_coils");
        REQUIRE( !num_coils.isNull() );
        REQUIRE( num_coils.type().name() == typeid(int).name() );
        REQUIRE( num_coils.as<int>() == 2 );
    }

    std::string exp_coil_names[] = { "TFP1", "TF" };
    std::string exp_coil_upper_lowers[] = { "ONLY", "ONLY" };
    double exp_coil_values[] = { 1.23, 2.13 };

    for (int coil_idx = 0; coil_idx < 2; ++coil_idx) {
        uda::TreeNode coil = subtype.child(coil_idx);

        REQUIRE( coil.name() == "coils" );
        REQUIRE( coil.numChildren() == 0 );
        REQUIRE( coil.atomicCount() == 3 );

        {
            std::string exp_names[] = { "name", "upper_lower", "value" };
            bool exp_pointers[] = { true, true, false };
            std::string exp_types[] = { "STRING", "STRING", "double" };
            size_t exp_ranks[] = { 0, 0, 0 };

            REQUIRE( coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
            REQUIRE( coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
            REQUIRE( coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
            REQUIRE( coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

            uda::Scalar name = coil.atomicScalar("name");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_coil_names[coil_idx] );

            uda::Scalar upper_lower = coil.atomicScalar("upper_lower");
            REQUIRE( !upper_lower.isNull() );
            REQUIRE( upper_lower.type().name() == typeid(char*).name() );
            REQUIRE( std::string(upper_lower.as<char*>()) == exp_coil_upper_lowers[coil_idx] );

            uda::Scalar value = coil.atomicScalar("value");
            REQUIRE( !value.isNull() );
            REQUIRE( value.type().name() == typeid(double).name() );
            REQUIRE( value.as<double>() == Approx(exp_coil_values[coil_idx]) );
        }
    }
}

//TEST_CASE( "Test getActiveLimit function with only system", "[plugins][PARAMSDB][getActiveLimit]" ) {
//
//#ifdef FATCLIENT
//#  include "setupEnvironment.inc"
//#endif
//
//    uda::Client client;
//
//    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP')", "");
//
//    REQUIRE( result.errorCode() == 0 );
//    REQUIRE( result.errorMessage().empty() );
//
//    uda::Data* data = result.data();
//
//    REQUIRE( data != NULL );
//    REQUIRE( !data->isNull() );
//    REQUIRE( data->type().name() == typeid(char*).name() );
//
//    auto str = dynamic_cast<uda::String*>(data);
//
//    REQUIRE( str != NULL );
//
//    std::string expected = "\nPARAMSDB: get - Read data from a PARAMSDB file\n\n";
//
//    REQUIRE( str->str() == expected );
//}