#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <c++/UDA.hpp>

TEST_CASE( "Test EQUIMAP::help() function", "[EQUIMAP][plugins]" )
{
#include "setup.inc"

    uda::Client client;

    const uda::Result& result = client.get("EQUIMAP::help()", "");

    REQUIRE( result.errorCode() == 0 );
    REQUIRE( result.errorMessage().empty() );

    uda::Data* data = result.data();

    REQUIRE( data != NULL );
    REQUIRE( !data->isNull() );
    REQUIRE( data->type().name() == typeid(char*).name() );

    auto str = dynamic_cast<uda::String*>(data);

    REQUIRE( str != NULL );

    std::string expected = "psiRZBox Enabled!";

    REQUIRE( str->str() == expected );
}
