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

TEST_CASE( "Test getActiveLimit function with subtype and coil", "[plugins][PARAMSDB][getActiveLimit]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip', coil='TFP1')", "");

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

TEST_CASE( "Test getActiveLimit function with subtype", "[plugins][PARAMSDB][getActiveLimit]" ) {

#ifdef FATCLIENT
#  include "setupEnvironment.inc"
#endif

    uda::Client client;

    const uda::Result& result = client.get("PARAMSDB::getActiveLimit(system='RTP', subtype='Current_Threshold_Trip')", "");

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