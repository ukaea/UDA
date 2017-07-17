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
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

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
    REQUIRE( result.error() == "" );
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
    REQUIRE( result.error() == "" );
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

TEST_CASE( "Test getActiveLimit function with system", "[plugins][PARAMSDB][getActiveLimit]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
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

TEST_CASE( "Test getForceCoefficients function with coil and upper_lower", "[plugins][PARAMSDB][getForceCoefficients]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getForceCoefficients(coil='TFP1', upper_lower='ONLY')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode coil = tree.child(0);

    REQUIRE( coil.name() == "data" );
    REQUIRE( coil.numChildren() == 1 );
    REQUIRE( coil.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_upper_lowers" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar num_subtypes = coil.atomicScalar("num_upper_lowers");
        REQUIRE( !num_subtypes.isNull() );
        REQUIRE( num_subtypes.type().name() == typeid(int).name() );
        REQUIRE( num_subtypes.as<int>() == 1 );
    }

    uda::TreeNode upper_lower = coil.child(0);

    REQUIRE( upper_lower.name() == "upper_lowers" );
    REQUIRE( upper_lower.numChildren() == 2 );
    REQUIRE( upper_lower.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_force_coeffs" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( upper_lower.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( upper_lower.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( upper_lower.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( upper_lower.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = upper_lower.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "ONLY" );

        uda::Scalar num_coils = upper_lower.atomicScalar("num_force_coeffs");
        REQUIRE( !num_coils.isNull() );
        REQUIRE( num_coils.type().name() == typeid(int).name() );
        REQUIRE( num_coils.as<int>() == 2 );
    }

    std::string exp_coil_names[] = { "TF", "P1" };
    double exp_coil_values[] = { 1.11, 2.22 };

    for (int coil_idx = 0; coil_idx < 2; ++coil_idx) {
        uda::TreeNode driven_coil = upper_lower.child(coil_idx);

        REQUIRE( driven_coil.name() == "force_coeffs" );
        REQUIRE( driven_coil.numChildren() == 0 );
        REQUIRE( driven_coil.atomicCount() == 2 );

        {
            std::string exp_names[] = { "driven_coil", "value" };
            bool exp_pointers[] = { true, false };
            std::string exp_types[] = { "STRING", "double" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( driven_coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
            REQUIRE( driven_coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
            REQUIRE( driven_coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
            REQUIRE( driven_coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

            uda::Scalar name = driven_coil.atomicScalar("driven_coil");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_coil_names[coil_idx] );

            uda::Scalar value = driven_coil.atomicScalar("value");
            REQUIRE( !value.isNull() );
            REQUIRE( value.type().name() == typeid(double).name() );
            REQUIRE( value.as<double>() == Approx(exp_coil_values[coil_idx]) );
        }
    }
}

TEST_CASE( "Test getForceCoefficients function with coil", "[plugins][PARAMSDB][getForceCoefficients]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getForceCoefficients(coil='TFP1')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode coil = tree.child(0);

    REQUIRE( coil.name() == "data" );
    REQUIRE( coil.numChildren() == 1 );
    REQUIRE( coil.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_upper_lowers" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar num_subtypes = coil.atomicScalar("num_upper_lowers");
        REQUIRE( !num_subtypes.isNull() );
        REQUIRE( num_subtypes.type().name() == typeid(int).name() );
        REQUIRE( num_subtypes.as<int>() == 1 );
    }

    uda::TreeNode upper_lower = coil.child(0);

    REQUIRE( upper_lower.name() == "upper_lowers" );
    REQUIRE( upper_lower.numChildren() == 2 );
    REQUIRE( upper_lower.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_force_coeffs" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( upper_lower.atomicNames() == ARR2VEC( std::string, exp_names ) );
        REQUIRE( upper_lower.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
        REQUIRE( upper_lower.atomicTypes() == ARR2VEC( std::string, exp_types ) );
        REQUIRE( upper_lower.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

        uda::Scalar name = upper_lower.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "ONLY" );

        uda::Scalar num_coils = upper_lower.atomicScalar("num_force_coeffs");
        REQUIRE( !num_coils.isNull() );
        REQUIRE( num_coils.type().name() == typeid(int).name() );
        REQUIRE( num_coils.as<int>() == 2 );
    }

    std::string exp_coil_names[] = { "TF", "P1" };
    double exp_coil_values[] = { 1.11, 2.22 };

    for (int coil_idx = 0; coil_idx < 2; ++coil_idx) {
        uda::TreeNode driven_coil = upper_lower.child(coil_idx);

        REQUIRE( driven_coil.name() == "force_coeffs" );
        REQUIRE( driven_coil.numChildren() == 0 );
        REQUIRE( driven_coil.atomicCount() == 2 );

        {
            std::string exp_names[] = { "driven_coil", "value" };
            bool exp_pointers[] = { true, false };
            std::string exp_types[] = { "STRING", "double" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( driven_coil.atomicNames() == ARR2VEC( std::string, exp_names ) );
            REQUIRE( driven_coil.atomicPointers() == ARR2VEC( bool, exp_pointers ) );
            REQUIRE( driven_coil.atomicTypes() == ARR2VEC( std::string, exp_types ) );
            REQUIRE( driven_coil.atomicRank() == ARR2VEC( size_t, exp_ranks ) );

            uda::Scalar name = driven_coil.atomicScalar("driven_coil");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_coil_names[coil_idx] );

            uda::Scalar value = driven_coil.atomicScalar("value");
            REQUIRE( !value.isNull() );
            REQUIRE( value.type().name() == typeid(double).name() );
            REQUIRE( value.as<double>() == Approx(exp_coil_values[coil_idx]) );
        }
    }
}

TEST_CASE( "Test getFilterCoefficients function with filter", "[plugins][PARAMSDB][getFilterCoefficients]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getFilterCoefficients(filter=0)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode filter = tree.child(0);

    REQUIRE( filter.name() == "data" );
    REQUIRE( filter.numChildren() == 5 );
    REQUIRE( filter.atomicCount() == 2 );

    {
        std::string exp_names[] = { "filter", "num_coefficients" };
        bool exp_pointers[] = { false, false };
        std::string exp_types[] = { "int", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( filter.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( filter.atomicPointers() == ARR2VEC(bool, exp_pointers) );
        REQUIRE( filter.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( filter.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = filter.atomicScalar("filter");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(int).name() );
        REQUIRE( name.as<int>() == 0 );

        uda::Scalar num_subtypes = filter.atomicScalar("num_coefficients");
        REQUIRE( !num_subtypes.isNull() );
        REQUIRE( num_subtypes.type().name() == typeid(int).name() );
        REQUIRE( num_subtypes.as<int>() == 5 );
    }

    int exp_coeffs[] = { 0, 1, 2, 3, 4 };
    double exp_coeff_values[] = { 1.0, 1.1, 1.2, 1.3, 1.4 };

    for (int coeff_idx = 0; coeff_idx < 5; ++coeff_idx) {
        uda::TreeNode coeff = filter.child(coeff_idx);

        REQUIRE( coeff.name() == "coefficients" );
        REQUIRE( coeff.numChildren() == 0 );
        REQUIRE( coeff.atomicCount() == 2 );

        {
            std::string exp_names[] = { "coefficient", "value" };
            bool exp_pointers[] = { false, false };
            std::string exp_types[] = { "int", "double" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( coeff.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( coeff.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( coeff.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( coeff.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = coeff.atomicScalar("coefficient");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(int).name() );
            REQUIRE( name.as<int>() == exp_coeffs[coeff_idx] );

            uda::Scalar value = coeff.atomicScalar("value");
            REQUIRE( !value.isNull() );
            REQUIRE( value.type().name() == typeid(double).name() );
            REQUIRE( value.as<double>() == Approx(exp_coeff_values[coeff_idx]) );
        }
    }
}

TEST_CASE( "Test getFilterCoefficients function with no args", "[plugins][PARAMSDB][getFilterCoefficients]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getFilterCoefficients()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 2);

    for (int tree_idx = 0; tree_idx < 2; ++tree_idx) {
        uda::TreeNode filter = tree.child(tree_idx);

        REQUIRE( filter.name() == "data" );
        REQUIRE( filter.numChildren() == 5 );
        REQUIRE( filter.atomicCount() == 2 );

        {
            std::string exp_names[] = { "filter", "num_coefficients" };
            bool exp_pointers[] = { false, false };
            std::string exp_types[] = { "int", "int" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( filter.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( filter.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( filter.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( filter.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = filter.atomicScalar("filter");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(int).name() );
            REQUIRE( name.as<int>() == tree_idx );

            uda::Scalar num_subtypes = filter.atomicScalar("num_coefficients");
            REQUIRE( !num_subtypes.isNull() );
            REQUIRE( num_subtypes.type().name() == typeid(int).name() );
            REQUIRE( num_subtypes.as<int>() == 5 );
        }

        int exp_coeffs[] = { 0, 1, 2, 3, 4 };
        double exp_coeff_values[] = { 1.0, 1.1, 1.2, 1.3, 1.4 };

        for (int coeff_idx = 0; coeff_idx < 5; ++coeff_idx) {
            uda::TreeNode coeff = filter.child(coeff_idx);

            REQUIRE( coeff.name() == "coefficients" );
            REQUIRE( coeff.numChildren() == 0 );
            REQUIRE( coeff.atomicCount() == 2 );

            {
                std::string exp_names[] = { "coefficient", "value" };
                bool exp_pointers[] = { false, false };
                std::string exp_types[] = { "int", "double" };
                size_t exp_ranks[] = { 0, 0 };

                REQUIRE( coeff.atomicNames() == ARR2VEC(std::string, exp_names) );
                REQUIRE( coeff.atomicPointers() == ARR2VEC(bool, exp_pointers) );
                REQUIRE( coeff.atomicTypes() == ARR2VEC(std::string, exp_types) );
                REQUIRE( coeff.atomicRank() == ARR2VEC(size_t, exp_ranks) );

                uda::Scalar name = coeff.atomicScalar("coefficient");
                REQUIRE( !name.isNull() );
                REQUIRE( name.type().name() == typeid(int).name() );
                REQUIRE( name.as<int>() == exp_coeffs[coeff_idx] );

                uda::Scalar value = coeff.atomicScalar("value");
                REQUIRE( !value.isNull() );
                REQUIRE( value.type().name() == typeid(double).name() );
                REQUIRE( value.as<double>() == Approx( tree_idx + exp_coeff_values[coeff_idx] ) );
            }
        }
    }
}

TEST_CASE( "Test getBoardCalibrations function with board", "[plugins][PARAMSDB][getBoardCalibrations]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getBoardCalibrations(board=0)", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode board = tree.child(0);

    REQUIRE( board.name() == "data" );
    REQUIRE( board.numChildren() == 3 );
    REQUIRE( board.atomicCount() == 2 );

    {
        std::string exp_names[] = { "board", "num_channels" };
        bool exp_pointers[] = { false, false };
        std::string exp_types[] = { "int", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( board.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( board.atomicPointers() == ARR2VEC(bool, exp_pointers) );
        REQUIRE( board.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( board.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = board.atomicScalar("board");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(int).name() );
        REQUIRE( name.as<int>() == 0 );

        uda::Scalar num_channels = board.atomicScalar("num_channels");
        REQUIRE( !num_channels.isNull() );
        REQUIRE( num_channels.type().name() == typeid(int).name() );
        REQUIRE( num_channels.as<int>() == 3 );
    }

    int exp_channels[] = { 0, 1, 2 };
    double exp_gains[] = { 1.0, 1.1, 1.2 };
    double exp_cal_offsets[] = { 0.01, 0.02, 0.03 };

    for (int channel_idx = 0; channel_idx < 3; ++channel_idx) {
        uda::TreeNode channel = board.child(channel_idx);

        REQUIRE( channel.name() == "channels" );
        REQUIRE( channel.numChildren() == 0 );
        REQUIRE( channel.atomicCount() == 3 );

        {
            std::string exp_names[] = { "channel", "gain", "cal_offset" };
            bool exp_pointers[] = { false, false, false };
            std::string exp_types[] = { "int", "double", "double" };
            size_t exp_ranks[] = { 0, 0, 0 };

            REQUIRE( channel.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( channel.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( channel.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( channel.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = channel.atomicScalar("channel");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(int).name() );
            REQUIRE( name.as<int>() == exp_channels[channel_idx] );

            uda::Scalar gain = channel.atomicScalar("gain");
            REQUIRE( !gain.isNull() );
            REQUIRE( gain.type().name() == typeid(double).name() );
            REQUIRE( gain.as<double>() == Approx(exp_gains[channel_idx]) );

            uda::Scalar cal_offset = channel.atomicScalar("cal_offset");
            REQUIRE( !cal_offset.isNull() );
            REQUIRE( cal_offset.type().name() == typeid(double).name() );
            REQUIRE( cal_offset.as<double>() == Approx(exp_cal_offsets[channel_idx]) );
        }
    }
}

TEST_CASE( "Test getBoardCalibrations function with no args", "[plugins][PARAMSDB][getBoardCalibrations]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getBoardCalibrations()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 2 );

    for (int board_idx = 0; board_idx < 2; ++board_idx) {
        uda::TreeNode board = tree.child(board_idx);

        REQUIRE( board.name() == "data" );
        REQUIRE( board.numChildren() == 3 );
        REQUIRE( board.atomicCount() == 2 );

        {
            std::string exp_names[] = { "board", "num_channels" };
            bool exp_pointers[] = { false, false };
            std::string exp_types[] = { "int", "int" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( board.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( board.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( board.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( board.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = board.atomicScalar("board");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(int).name() );
            REQUIRE( name.as<int>() == board_idx );

            uda::Scalar num_channels = board.atomicScalar("num_channels");
            REQUIRE( !num_channels.isNull() );
            REQUIRE( num_channels.type().name() == typeid(int).name() );
            REQUIRE( num_channels.as<int>() == 3 );
        }

        int exp_channels[] = { 0, 1, 2 };
        double exp_gains[] = { 1.0, 1.1, 1.2 };
        double exp_cal_offsets[] = { 0.01, 0.02, 0.03 };

        for (int channel_idx = 0; channel_idx < 3; ++channel_idx) {
            uda::TreeNode channel = board.child(channel_idx);

            REQUIRE( channel.name() == "channels" );
            REQUIRE( channel.numChildren() == 0 );
            REQUIRE( channel.atomicCount() == 3 );

            {
                std::string exp_names[] = { "channel", "gain", "cal_offset" };
                bool exp_pointers[] = { false, false, false };
                std::string exp_types[] = { "int", "double", "double" };
                size_t exp_ranks[] = { 0, 0, 0 };

                REQUIRE( channel.atomicNames() == ARR2VEC(std::string, exp_names) );
                REQUIRE( channel.atomicPointers() == ARR2VEC(bool, exp_pointers) );
                REQUIRE( channel.atomicTypes() == ARR2VEC(std::string, exp_types) );
                REQUIRE( channel.atomicRank() == ARR2VEC(size_t, exp_ranks) );

                uda::Scalar name = channel.atomicScalar("channel");
                REQUIRE( !name.isNull() );
                REQUIRE( name.type().name() == typeid(int).name() );
                REQUIRE( name.as<int>() == exp_channels[channel_idx] );

                uda::Scalar gain = channel.atomicScalar("gain");
                REQUIRE( !gain.isNull() );
                REQUIRE( gain.type().name() == typeid(double).name() );
                REQUIRE( gain.as<double>() == Approx(board_idx + exp_gains[channel_idx]) );

                uda::Scalar cal_offset = channel.atomicScalar("cal_offset");
                REQUIRE( !cal_offset.isNull() );
                REQUIRE( cal_offset.type().name() == typeid(double).name() );
                REQUIRE( cal_offset.as<double>() == Approx(exp_cal_offsets[channel_idx]) );
            }
        }
    }
}

TEST_CASE( "Test getCoilParameters function with coil, upper_lower and parameter", "[plugins][PARAMSDB][getCoilParameters]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get(
            "PARAMSDB::getCoilParameters(coil='TFP1', upper_lower='ONLY', parameter='TestParam1')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode coil = tree.child(0);

    REQUIRE( coil.name() == "data" );
    REQUIRE( coil.numChildren() == 1 );
    REQUIRE( coil.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_upper_lowers" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( coil.atomicPointers() == ARR2VEC(bool, exp_pointers) );
        REQUIRE( coil.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( coil.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar num_upper_lowers = coil.atomicScalar("num_upper_lowers");
        REQUIRE( !num_upper_lowers.isNull() );
        REQUIRE( num_upper_lowers.type().name() == typeid(int).name() );
        REQUIRE( num_upper_lowers.as<int>() == 1 );
    }

    uda::TreeNode upper_lower = coil.child(0);

    REQUIRE( upper_lower.name() == "upper_lowers" );
    REQUIRE( upper_lower.numChildren() == 1 );
    REQUIRE( upper_lower.atomicCount() == 2 );

    {
        std::string exp_names[] = { "upper_lower", "num_parameters" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( upper_lower.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( upper_lower.atomicPointers() == ARR2VEC(bool, exp_pointers)) ;
        REQUIRE( upper_lower.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( upper_lower.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = upper_lower.atomicScalar("upper_lower");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "ONLY" );

        uda::Scalar num_parameters = upper_lower.atomicScalar("num_parameters");
        REQUIRE( !num_parameters.isNull() );
        REQUIRE( num_parameters.type().name() == typeid(int).name() );
        REQUIRE( num_parameters.as<int>() == 1 );
    }

    uda::TreeNode parameter = upper_lower.child(0);

    REQUIRE( parameter.name() == "parameters" );
    REQUIRE( parameter.numChildren() == 0 );
    REQUIRE( parameter.atomicCount() == 2 );

    {
        std::string exp_names[] = { "parameter", "value" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "double" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( parameter.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( parameter.atomicPointers() == ARR2VEC(bool, exp_pointers)) ;
        REQUIRE( parameter.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( parameter.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = parameter.atomicScalar("parameter");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TestParam1" );

        uda::Scalar value = parameter.atomicScalar("value");
        REQUIRE( !value.isNull() );
        REQUIRE( value.type().name() == typeid(double).name() );
        REQUIRE( value.as<double>() == Approx(10.0) );
    }
}

TEST_CASE( "Test getCoilParameters function with coil and upper_lower", "[plugins][PARAMSDB][getCoilParameters]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getCoilParameters(coil='TFP1', upper_lower='ONLY')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode coil = tree.child(0);

    REQUIRE( coil.name() == "data" );
    REQUIRE( coil.numChildren() == 1 );
    REQUIRE( coil.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_upper_lowers" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( coil.atomicPointers() == ARR2VEC(bool, exp_pointers) );
        REQUIRE( coil.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( coil.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar num_upper_lowers = coil.atomicScalar("num_upper_lowers");
        REQUIRE( !num_upper_lowers.isNull() );
        REQUIRE( num_upper_lowers.type().name() == typeid(int).name() );
        REQUIRE( num_upper_lowers.as<int>() == 1 );
    }

    uda::TreeNode upper_lower = coil.child(0);

    REQUIRE( upper_lower.name() == "upper_lowers" );
    REQUIRE( upper_lower.numChildren() == 3 );
    REQUIRE( upper_lower.atomicCount() == 2 );

    {
        std::string exp_names[] = { "upper_lower", "num_parameters" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( upper_lower.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( upper_lower.atomicPointers() == ARR2VEC(bool, exp_pointers)) ;
        REQUIRE( upper_lower.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( upper_lower.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = upper_lower.atomicScalar("upper_lower");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "ONLY" );

        uda::Scalar num_parameters = upper_lower.atomicScalar("num_parameters");
        REQUIRE( !num_parameters.isNull() );
        REQUIRE( num_parameters.type().name() == typeid(int).name() );
        REQUIRE( num_parameters.as<int>() == 3 );
    }

    std::string exp_param_names[] = { "TestParam1", "TestParam2", "TestParam3" };
    double exp_param_values[] = { 10.0, 20.0, 30.0 };

    for (int param_idx = 0; param_idx < 3; ++param_idx) {
        uda::TreeNode parameter = upper_lower.child(param_idx);

        REQUIRE( parameter.name() == "parameters" );
        REQUIRE( parameter.numChildren() == 0 );
        REQUIRE( parameter.atomicCount() == 2 );

        {
            std::string exp_names[] = { "parameter", "value" };
            bool exp_pointers[] = { true, false };
            std::string exp_types[] = { "STRING", "double" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( parameter.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( parameter.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( parameter.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( parameter.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = parameter.atomicScalar("parameter");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_param_names[param_idx] );

            uda::Scalar value = parameter.atomicScalar("value");
            REQUIRE( !value.isNull() );
            REQUIRE( value.type().name() == typeid(double).name() );
            REQUIRE( value.as<double>() == Approx(exp_param_values[param_idx]) );
        }
    }
}

TEST_CASE( "Test getCoilParameters function with coil", "[plugins][PARAMSDB][getCoilParameters]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getCoilParameters(coil='TFP1')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode coil = tree.child(0);

    REQUIRE( coil.name() == "data" );
    REQUIRE( coil.numChildren() == 2 );
    REQUIRE( coil.atomicCount() == 2 );

    {
        std::string exp_names[] = { "name", "num_upper_lowers" };
        bool exp_pointers[] = { true, false };
        std::string exp_types[] = { "STRING", "int" };
        size_t exp_ranks[] = { 0, 0 };

        REQUIRE( coil.atomicNames() == ARR2VEC(std::string, exp_names) );
        REQUIRE( coil.atomicPointers() == ARR2VEC(bool, exp_pointers) );
        REQUIRE( coil.atomicTypes() == ARR2VEC(std::string, exp_types) );
        REQUIRE( coil.atomicRank() == ARR2VEC(size_t, exp_ranks) );

        uda::Scalar name = coil.atomicScalar("name");
        REQUIRE( !name.isNull() );
        REQUIRE( name.type().name() == typeid(char*).name() );
        REQUIRE( std::string(name.as<char*>()) == "TFP1" );

        uda::Scalar num_upper_lowers = coil.atomicScalar("num_upper_lowers");
        REQUIRE( !num_upper_lowers.isNull() );
        REQUIRE( num_upper_lowers.type().name() == typeid(int).name() );
        REQUIRE( num_upper_lowers.as<int>() == 2 );
    }

    std::string exp_upper_lower_names[] = { "ONLY", "BOTH" };
    double exp_param_values[][3] = { { 10.0, 20.0, 30.0 }, { 40.0, 50.0, 60.0 } };

    for (int upper_lower_idx = 0; upper_lower_idx < 2; ++upper_lower_idx) {
        uda::TreeNode upper_lower = coil.child(upper_lower_idx);

        REQUIRE (upper_lower.name() == "upper_lowers" );
        REQUIRE( upper_lower.numChildren() == 3 );
        REQUIRE( upper_lower.atomicCount() == 2 );

        {
            std::string exp_names[] = { "upper_lower", "num_parameters" };
            bool exp_pointers[] = { true, false };
            std::string exp_types[] = { "STRING", "int" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( upper_lower.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( upper_lower.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( upper_lower.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( upper_lower.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = upper_lower.atomicScalar("upper_lower");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_upper_lower_names[upper_lower_idx] );

            uda::Scalar num_parameters = upper_lower.atomicScalar("num_parameters" );
            REQUIRE( !num_parameters.isNull() );
            REQUIRE( num_parameters.type().name() == typeid(int).name() );
            REQUIRE( num_parameters.as<int>() == 3 );
        }

        std::string exp_param_names[] = { "TestParam1", "TestParam2", "TestParam3" };

        for (int param_idx = 0; param_idx < 3; ++param_idx) {
            uda::TreeNode parameter = upper_lower.child(param_idx);

            REQUIRE( parameter.name() == "parameters" );
            REQUIRE( parameter.numChildren() == 0 );
            REQUIRE( parameter.atomicCount() == 2 );

            {
                std::string exp_names[] = { "parameter", "value" };
                bool exp_pointers[] = { true, false };
                std::string exp_types[] = { "STRING", "double" };
                size_t exp_ranks[] = { 0, 0 };

                REQUIRE( parameter.atomicNames() == ARR2VEC(std::string, exp_names));
                REQUIRE( parameter.atomicPointers() == ARR2VEC(bool, exp_pointers));
                REQUIRE( parameter.atomicTypes() == ARR2VEC(std::string, exp_types));
                REQUIRE( parameter.atomicRank() == ARR2VEC(size_t, exp_ranks));

                uda::Scalar name = parameter.atomicScalar("parameter");
                REQUIRE( !name.isNull() );
                REQUIRE( name.type().name() == typeid(char*).name() );
                REQUIRE( std::string(name.as<char*>()) == exp_param_names[param_idx] );

                uda::Scalar value = parameter.atomicScalar("value");
                REQUIRE( !value.isNull() );
                REQUIRE( value.type().name() == typeid(double).name() );
                REQUIRE( value.as<double>() == Approx(exp_param_values[upper_lower_idx][param_idx]) );
            }
        }
    }
}

TEST_CASE( "Test getCoilParameters function with no args", "[plugins][PARAMSDB][getCoilParameters]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getCoilParameters()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 2 );

    std::string exp_coil_names[] = { "TFP1", "TF" };

    for (int coil_idx = 0; coil_idx < 2; ++coil_idx) {
        uda::TreeNode coil = tree.child(coil_idx);

        REQUIRE( coil.name() == "data" );
        REQUIRE( coil.numChildren() == 2 );
        REQUIRE( coil.atomicCount() == 2 );

        {
            std::string exp_names[] = { "name", "num_upper_lowers" };
            bool exp_pointers[] = { true, false };
            std::string exp_types[] = { "STRING", "int" };
            size_t exp_ranks[] = { 0, 0 };

            REQUIRE( coil.atomicNames() == ARR2VEC(std::string, exp_names) );
            REQUIRE( coil.atomicPointers() == ARR2VEC(bool, exp_pointers) );
            REQUIRE( coil.atomicTypes() == ARR2VEC(std::string, exp_types) );
            REQUIRE( coil.atomicRank() == ARR2VEC(size_t, exp_ranks) );

            uda::Scalar name = coil.atomicScalar("name");
            REQUIRE( !name.isNull() );
            REQUIRE( name.type().name() == typeid(char*).name() );
            REQUIRE( std::string(name.as<char*>()) == exp_coil_names[coil_idx] );

            uda::Scalar num_upper_lowers = coil.atomicScalar("num_upper_lowers");
            REQUIRE( !num_upper_lowers.isNull() );
            REQUIRE( num_upper_lowers.type().name() == typeid(int).name() );
            REQUIRE( num_upper_lowers.as<int>() == 2 );
        }

        std::string exp_upper_lower_names[] = { "ONLY", "BOTH" };
        double exp_param_values[][2][3] = {
                {{ 10.0, 20.0, 30.0 }, { 40.0, 50.0, 60.0 }},       // Coil 1
                {{ 100.0, 200.0, 300.0 }, { 400.0, 500.0, 600.0 }}  // Coil 2
        };

        for (int upper_lower_idx = 0; upper_lower_idx < 2; ++upper_lower_idx) {
            uda::TreeNode upper_lower = coil.child(upper_lower_idx);

            REQUIRE (upper_lower.name() == "upper_lowers");
            REQUIRE( upper_lower.numChildren() == 3 );
            REQUIRE( upper_lower.atomicCount() == 2 );

            {
                std::string exp_names[] = { "upper_lower", "num_parameters" };
                bool exp_pointers[] = { true, false };
                std::string exp_types[] = { "STRING", "int" };
                size_t exp_ranks[] = { 0, 0 };

                REQUIRE( upper_lower.atomicNames() == ARR2VEC(std::string, exp_names) );
                REQUIRE( upper_lower.atomicPointers() == ARR2VEC(bool, exp_pointers) );
                REQUIRE( upper_lower.atomicTypes() == ARR2VEC(std::string, exp_types) );
                REQUIRE( upper_lower.atomicRank() == ARR2VEC(size_t, exp_ranks) );

                uda::Scalar name = upper_lower.atomicScalar("upper_lower");
                REQUIRE( !name.isNull() );
                REQUIRE( name.type().name() == typeid(char*).name() );
                REQUIRE( std::string(name.as<char*>()) == exp_upper_lower_names[upper_lower_idx] );

                uda::Scalar num_parameters = upper_lower.atomicScalar("num_parameters");
                REQUIRE( !num_parameters.isNull() );
                REQUIRE( num_parameters.type().name() == typeid(int).name() );
                REQUIRE( num_parameters.as<int>() == 3 );
            }

            std::string exp_param_names[] = { "TestParam1", "TestParam2", "TestParam3" };

            for (int param_idx = 0; param_idx < 3; ++param_idx) {
                uda::TreeNode parameter = upper_lower.child(param_idx);

                REQUIRE( parameter.name() == "parameters" );
                REQUIRE( parameter.numChildren() == 0 );
                REQUIRE( parameter.atomicCount() == 2 );

                {
                    std::string exp_names[] = { "parameter", "value" };
                    bool exp_pointers[] = { true, false };
                    std::string exp_types[] = { "STRING", "double" };
                    size_t exp_ranks[] = { 0, 0 };

                    REQUIRE( parameter.atomicNames() == ARR2VEC(std::string, exp_names) );
                    REQUIRE( parameter.atomicPointers() == ARR2VEC(bool, exp_pointers) );
                    REQUIRE( parameter.atomicTypes() == ARR2VEC(std::string, exp_types) );
                    REQUIRE( parameter.atomicRank() == ARR2VEC(size_t, exp_ranks) );

                    uda::Scalar name = parameter.atomicScalar("parameter");
                    REQUIRE( !name.isNull() );
                    REQUIRE( name.type().name() == typeid(char*).name() );
                    REQUIRE( std::string(name.as<char*>()) == exp_param_names[param_idx] );

                    uda::Scalar value = parameter.atomicScalar("value");
                    REQUIRE( !value.isNull() );
                    REQUIRE( value.type().name() == typeid(double).name() );
                    REQUIRE( value.as<double>() == Approx(exp_param_values[coil_idx][upper_lower_idx][param_idx]) );
                }
            }
        }
    }
}