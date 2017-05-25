#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test help function", "[plugins][PARAMSDB]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

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

    std::string expected = "\nPARAMSDB: get - Read data from a PARAMSDB file\n\n";

    REQUIRE( str->str() == expected );
}

TEST_CASE( "Test getActiveLimit function with subtype and coil", "[plugins][PARAMSDB][getActiveLimit]" )
{
#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip', coil='TFP1')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 5 );

    std::string exp_names[] = { "system", "subtype", "coils", "upper_lowers", "values" };
    bool exp_pointers[] = { true, true, true, true, true };
    std::string exp_types[] = { "STRING", "STRING", "STRING *", "STRING *", "double *" };
    size_t exp_ranks[] = { 0, 0, 0, 0, 0 };

    REQUIRE( child.atomicNames() == std::vector<std::string>(&exp_names[0], &exp_names[5]) );
    REQUIRE( child.atomicPointers() == std::vector<bool>(&exp_pointers[0], &exp_pointers[5]) );
    REQUIRE( child.atomicTypes() == std::vector<std::string>(&exp_types[0], &exp_types[5]) );
    REQUIRE( child.atomicRank() == std::vector<size_t>(&exp_ranks[0], &exp_ranks[5]) );

    uda::Scalar system = child.atomicScalar("system");
    REQUIRE( !system.isNull() );

    REQUIRE( system.type().name() == typeid(char*).name() );
    REQUIRE( std::string(system.as<char*>()) == "RTP" );

    uda::Scalar subtype = child.atomicScalar("subtype");
    REQUIRE( !subtype.isNull() );

    REQUIRE( subtype.type().name() == typeid(char*).name() );
    REQUIRE( std::string(subtype.as<char*>()) == "Current_Threshold_Trip" );

    uda::Vector coils = child.atomicVector("coils");
    REQUIRE( !coils.isNull() );

    REQUIRE( coils.type().name() == typeid(char*).name() );
    REQUIRE( std::string(coils.as<char*>().at(0)) == "TFP1" );

    uda::Vector upper_lowers = child.atomicVector("upper_lowers");
    REQUIRE( !upper_lowers.isNull() );

    REQUIRE( upper_lowers.type().name() == typeid(char*).name() );
    REQUIRE( std::string(upper_lowers.as<char*>().at(0)) == "ONLY" );

    uda::Vector values = child.atomicVector("values");
    REQUIRE( !values.isNull() );

    REQUIRE( values.type().name() == typeid(double).name() );
    REQUIRE( values.as<double>().at(0) == Approx(1.23) );
}

TEST_CASE( "Test getActiveLimit function with subtype", "[plugins][PARAMSDB][getActiveLimit]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );
    REQUIRE( result.isTree() );

    uda::TreeNode tree = result.tree();

    REQUIRE( tree.numChildren() == 1 );

    uda::TreeNode child = tree.child(0);

    REQUIRE( child.name() == "data" );
    REQUIRE( child.numChildren() == 0 );
    REQUIRE( child.atomicCount() == 5 );

    std::string exp_names[] = { "system", "subtype", "coils", "upper_lowers", "values" };
    bool exp_pointers[] = { true, true, true, true, true };
    std::string exp_types[] = { "STRING", "STRING", "STRING *", "STRING *", "double *" };
    size_t exp_ranks[] = { 0, 0, 0, 0, 0 };

    REQUIRE( child.atomicNames() == std::vector<std::string>(&exp_names[0], &exp_names[5]) );
    REQUIRE( child.atomicPointers() == std::vector<bool>(&exp_pointers[0], &exp_pointers[5]) );
    REQUIRE( child.atomicTypes() == std::vector<std::string>(&exp_types[0], &exp_types[5]) );
    REQUIRE( child.atomicRank() == std::vector<size_t>(&exp_ranks[0], &exp_ranks[5]) );

    uda::Scalar system = child.atomicScalar("system");
    REQUIRE( !system.isNull() );

    REQUIRE( system.type().name() == typeid(char*).name() );
    REQUIRE( std::string(system.as<char*>()) == "RTP" );

    uda::Scalar subtype = child.atomicScalar("subtype");
    REQUIRE( !subtype.isNull() );

    REQUIRE( subtype.type().name() == typeid(char*).name() );
    REQUIRE( std::string(subtype.as<char*>()) == "Current_Threshold_Trip" );

    uda::Vector coils = child.atomicVector("coils");
    REQUIRE( !coils.isNull() );

    REQUIRE( coils.size() == 2 );
    REQUIRE( coils.type().name() == typeid(char*).name() );
    REQUIRE( std::string(coils.as<char*>().at(0)) == "TFP1" );
    REQUIRE( std::string(coils.as<char*>().at(1)) == "TF" );

    uda::Vector upper_lowers = child.atomicVector("upper_lowers");
    REQUIRE( !upper_lowers.isNull() );

    REQUIRE( upper_lowers.size() == 2 );
    REQUIRE( upper_lowers.type().name() == typeid(char*).name() );
    REQUIRE( std::string(upper_lowers.as<char*>().at(0)) == "ONLY" );
    REQUIRE( std::string(upper_lowers.as<char*>().at(1)) == "ONLY" );

    uda::Vector values = child.atomicVector("values");
    REQUIRE( !values.isNull() );

    REQUIRE( values.size() == 2 );
    REQUIRE( values.type().name() == typeid(double).name() );
    REQUIRE( values.as<double>().at(0) == Approx(1.23) );
    REQUIRE( values.as<double>().at(0) == Approx(2.13) );
}

TEST_CASE( "Test getActiveLimit function with only system", "[plugins][PARAMSDB][getActiveLimit]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP')", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nPARAMSDB: get - Read data from a PARAMSDB file\n\n";

    REQUIRE( str->str() == expected );
}