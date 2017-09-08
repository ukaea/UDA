#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test ANBCORRECTIONS::help() function", "[ANBCORRECTIONS][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("ANBCORRECTIONS::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.error() == "" );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    uda::String* str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nanbCorrections: Functions Names and Test Descriptions\n\n";

    REQUIRE( str->str() == expected );
}
