#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test VIEWPORT::help() function", "[VIEWPORT][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("VIEWPORT::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "\nviewport: Add Functions Names, Syntax, and Descriptions\n\n";

    REQUIRE( str->str() == expected );
}
